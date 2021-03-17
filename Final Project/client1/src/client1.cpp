#include "examples.h"
#include <cmath>

using namespace std;
using namespace seal;

#define DOWNTOPBMP 1
  
unsigned char header[54];

void read_img (unsigned char *o_buf, const char* filename) { //{{{
  FILE* fr = fopen(filename, "rb");
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
// prepare to write files
  ofstream of_parms;
  ofstream of_sk;
  ofstream of_rk;
  ofstream of_cipher_client;
  of_parms.open("../../../../network/parms.bin", ios_base::out | ios_base::binary);
  of_sk.open("../../../../network/sk.bin", ios_base::out | ios_base::binary);
  of_rk.open("../../../../network/rk.bin", ios_base::out | ios_base::binary);
  of_cipher_client.open("../../../../network/cipherI.bin", ios_base::out | ios_base::binary);

  // image reading
  unsigned char plaintext_buf[28*28];
  unsigned char plaintext_buf1[28*28];
  read_img(plaintext_buf, "../../../image0.bmp"); 
  read_img(plaintext_buf1, "../../../image1.bmp");
  
  // parameter setting
  EncryptionParameters parms(scheme_type::ckks); // the CKKS homomorphic encryption scheme is used
  size_t poly_modulus_degree = 8192; // 4K slots are available
  parms.set_poly_modulus_degree(poly_modulus_degree);
  parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 60, 40, 40, 60 })); // less than 4 multiplications
  auto parms_size = parms.save(of_parms);

  double scale = pow(2.0, 40);
  SEALContext context(parms);

  // key generation
  KeyGenerator keygen(context);
  auto secret_key = keygen.secret_key(); // secret key generation 
  secret_key.save(of_sk);
  PublicKey public_key; 
  keygen.create_public_key(public_key); // public key generation
  RelinKeys relin_keys;
  keygen.create_relin_keys(relin_keys);
  relin_keys.save(of_rk);

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

  // encoding for data2 (input image 1)
  vector<double> input1;
  input1.reserve(slot_count);
  for (size_t i = 0; i < slot_count; i++) {
      double norm_pxl = 0.0;
      if (i < 28 * 28) { // use 28x28 slots out of 4K slots
          norm_pxl = (double)plaintext_buf1[i] / 255;
      }
      input1.push_back(norm_pxl); // put 28x28 pixels into th input1 vector
  }
  Plaintext p1;
  encoder.encode(input1, scale, p1); // encoding

  vector<double> input2;
  input2.reserve(slot_count);
  for (size_t i = 0; i < slot_count; i++) {
      double norm_pxl = 0.0;
      if (i < 28 * 28) { // use 28x28 slots out of 4K slots
          norm_pxl = 255 / ((double)plaintext_buf[i] + 1.0);
      }
      input2.push_back(norm_pxl); // put 28x28 pixels into th input0 vector
  }
  Plaintext p2;
  encoder.encode(input2, scale, p2); // encoding

  // encrypt the two plaintext polynomials
  Encryptor encryptor(context, public_key);
  auto size_c0 = encryptor.encrypt(p0).save(of_cipher_client);
  auto size_c1 = encryptor.encrypt(p1).save(of_cipher_client);
  auto size_c2 = encryptor.encrypt(p2).save(of_cipher_client);

  // close the ofstream
  of_parms.close();
  of_sk.close();
  of_rk.close();
  of_cipher_client.close();

  return 0;
}