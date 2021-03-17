rm -rf inout/result_image.bmp
rm -rf network
mkdir network
#1_client_enc
cd client1/build
./client1
mv *.bin ../../network/
#2_server
cd ../../server/build
./server
mv *.bin ../../network/
#3_client_dec
cd ../../client2/build
./client2
mv *.bmp ../../inout/
