rm -rf *.bmp
raspistill --encoding bmp -o image0.bmp -w 28 -h 28
sleep 1
raspistill --encoding bmp -o image1.bmp -w 28 -h 28
