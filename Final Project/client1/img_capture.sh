rm -rf *.bmp
raspistill --encoding bmp -o image0.bmp -w 32 -h 32
timeout /t 10 /nobreak
raspistill --encoding bmp -o image1.bmp -w 32 -h 32
