#include "examples.h"

using namespace std;
using namespace seal;

#define DOWNTOPBMP 1
  
unsigned char header[54];

void read_img (unsigned char *o_buf) { //{{{
  FILE* fr = fopen("../../test_images/test_image1.bmp", "rb");
  unsigned char i_buf[3*28*28];
  int tmpI;
  size_t result;

  // skip the 54B header
  result = fread(header, sizeof(unsigned char), 54, fr);

  // read R, G, and B pixel values
  for (int i=0; i<3; i++) 
  for (int r=0; r<28; r++) 
  for (int c=0; c<28; c++) {
    result = fread(i_buf, sizeof(unsigned char), 3*28*28, fr);
  }
  
  // crop and convert the image format from RGB to Y (monochrome)
  for (int r=0; r<28; r++) {
    for (int c=0; c<28; c++) {
      tmpI = ((int)i_buf[3*(r*28+c)] + 2*(int)i_buf[3*(r*28+c)+1] + (int)i_buf[3*(r*28+c)+2])/4;
      if (DOWNTOPBMP == 1) { // because the captured image is upside down
        o_buf[(28-r-1)*28+c] = (unsigned char)tmpI;
      } else {
        o_buf[r*28+c] = (unsigned char)tmpI;
      }
    }
  }

  fclose(fr);
} //}}}

void write_img (unsigned char *i_buf) { //{{{
  FILE* fw = fopen("result_image.bmp", "wb");
  size_t result;

  // write the 54B header (image size, format, etc.)
  result = fwrite(header, sizeof(unsigned char), 54, fw);

  // write R, G, and B pixel values
  for (int r=0; r<28; r++) 
  for (int c=0; c<28; c++) {
    result = fwrite(&i_buf[r*28+(28-c-1)], sizeof(unsigned char), 1, fw);
    result = fwrite(&i_buf[r*28+(28-c-1)], sizeof(unsigned char), 1, fw);
    result = fwrite(&i_buf[r*28+(28-c-1)], sizeof(unsigned char), 1, fw);
  }
  
  fclose(fw);
} //}}}

int main(void) {
  // image reading
  unsigned char plaintext_buf [28*28];
  read_img(plaintext_buf);
  
  // parameter setting
  EncryptionParameters parms(scheme_type::ckks); // the CKKS homomorphic encryption scheme is used
  size_t poly_modulus_degree = 8192; // 4K slots are available
  parms.set_poly_modulus_degree(poly_modulus_degree);
  parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 60, 40, 40, 60 })); // less than 4 multiplications
  double scale = pow(2.0, 40);
  SEALContext context(parms);

  // key generation
  KeyGenerator keygen(context);
  auto secret_key = keygen.secret_key(); // secret key generation 
  PublicKey public_key; 
  keygen.create_public_key(public_key); // public key generation
  
  // encryptor, evaluator, and decryptor setting
  Encryptor encryptor(context, public_key);
  Evaluator evaluator(context);
  Decryptor decryptor(context, secret_key);

  // encoding for data0 (input image)
  CKKSEncoder encoder(context);
  size_t slot_count = encoder.slot_count();
  vector<double> input0;
  input0.reserve(slot_count);
  for (size_t i=0; i<slot_count; i++) {
    double norm_pxl = 0.0; 
    if (i < 28*28) { // use 28x28 slots out of 4K slots
      norm_pxl = (double)plaintext_buf[i] / 255; 
    }
    input0.push_back(norm_pxl); // put 28x28 pixels into th input0 vector
  }
  Plaintext p0;
  encoder.encode(input0, scale, p0); // encoding

  // encoding for data1 (constant 10)
  Plaintext p1;
  encoder.encode(10, scale, p1); // encoding

  // encryption of the plaintext polynomial
  Ciphertext c0;
  encryptor.encrypt(p0, c0);

  // homomorphic evaluation (10*Enc(pixel values))
  Ciphertext cR;
  evaluator.multiply_plain(c0, p1, cR);
  evaluator.rescale_to_next_inplace(cR);

  // decryption and decoding
  Plaintext pR;
  decryptor.decrypt(cR, pR);
  vector<double> result;
  encoder.decode(pR, result);

  // writing the result image
  reverse(result.begin(), result.end());
  for (int i=0; i<28*28; i++) {
    double tmpD = 255*result.back(); // vector::back(): extracts an element from the vector
    plaintext_buf[28*28-i-1] = (tmpD > 255)? 255: tmpD;
    result.pop_back(); // vector::pop_back(): removes the element from the vector
  }
  write_img(plaintext_buf);

  return 0;
}
