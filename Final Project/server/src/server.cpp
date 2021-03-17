#include "examples.h"
#include <cmath>

using namespace std;
using namespace seal;


int main(void) {
    ifstream if_parms_server;
    ifstream if_cipher_server;
    ifstream if_relin_keys;
    ofstream of_cipher_server;

    if_parms_server.open("../../network/parms.bin", ios::binary); //"/../../../../network/parms.bin"
    if_cipher_server.open("../../network/cipherI.bin", ios::binary); //"/../../../../network/cipherI.bin"
    if_relin_keys.open("../../network/rk.bin", ios::binary); //"/../../../../network/rk.bin"
    of_cipher_server.open("cipherR.bin", ios_base::out | ios::binary);

    if (if_parms_server.bad())
    {
        std::cout << if_parms_server.rdbuf();
        return 0;
    }

    EncryptionParameters parms_server;
    parms_server.load(if_parms_server);

    SEALContext context_server(parms_server);
    Ciphertext c0, c1, c2;
    Ciphertext cR, cR1;
    c0.load(context_server, if_cipher_server);
    c1.load(context_server, if_cipher_server);
    c2.load(context_server, if_cipher_server);
    RelinKeys relin_keys;
    relin_keys.load(context_server, if_relin_keys);

    Evaluator evaluator(context_server);
    // homomorphic evaluation (Enc(image1) - Enc(image0))
    evaluator.sub(c1, c0, cR);

    // homomorphic evaluation (Enc(resImage) * Enc(1/(image0+1)))
    evaluator.multiply(cR, c2, cR1);
    evaluator.relinearize_inplace(cR1, relin_keys);
    auto size_cR_server = cR1.save(of_cipher_server);

    if_parms_server.close();
    if_cipher_server.close();
    if_relin_keys.close();
    of_cipher_server.close();


  return 0;
}
