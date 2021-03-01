/* 
	* @author	Sunwoong Sunny Kim
	* @brief	this file is for the B EE 525 class and cannot be distributed without permission of the author
	*         it is mainly based on C syntax
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>   
#include "GPIO.h"
 
#define countof(ARRAY) (sizeof(ARRAY) / sizeof*(ARRAY))
#define DOWNTOPBMP 1

using namespace exploringRPi;
using namespace std;

void read_img (unsigned char *o_buf) { //{{{
  FILE* f = fopen("../image.bmp", "rb");
  unsigned char header[54];
  unsigned char buf[3*32*32];
  int tmpI;
  size_t result;

  // skip the 54B header
  result = fread(header, sizeof(unsigned char), 54, f);

  // read R, G, and B pixel values
  for (int i=0; i<3; i++) 
  for (int r=0; r<32; r++) 
  for (int c=0; c<32; c++) {
    result = fread(buf, sizeof(unsigned char), 3*32*32, f);
  }
  
  // crop and convert the image format from RGB to Y (monochrome)
  for (int r=0; r<32; r++) {
    for (int c=0; c<32; c++) {
      if (r > 1 && r < 30) { // 32 --> 28
        if (c > 1 && c < 30) { // 32 --> 28
          tmpI = ((int)buf[3*(r*32+c)] + 2*(int)buf[3*(r*32+c)+1] + (int)buf[3*(r*32+c)+2])/4;
          if (DOWNTOPBMP == 1) { // because the captured image is upside down
            o_buf[(28-r+1)*28+(c-2)] = (unsigned char)tmpI;
          } else {
            o_buf[(r-2)*28+(c-2)] = (unsigned char)tmpI;
          }
        }
      }
    }
  }

	// for debugging
  for (int r=0; r<32; r++) {
    for (int c=0; c<32; c++) {
			printf("%3d, ", o_buf[r*28+c]);
		}
		printf("\n");
	}

  fclose(f);
} //}}}

int conv (int layer, int D, int N, int W, float *a, float *w, float *sum) { //{{{
  for (int n=0; n<N; n++)
  for (int z=0; z<D; z++)
  for (int y=0; y<W; y++)
  for (int x=0; x<W; x++) {
    for (int yy=0; yy<5; yy++)
    for (int xx=0; xx<5; xx++) {
      if (((y ==   0) && ((yy == 0) || (yy == 1))) ||  
          ((y ==   1) && ((yy == 0))) ||  
          ((y == W-2) && ((yy == 4))) ||  
          ((y == W-1) && ((yy == 3) || (yy == 4))) ||  
          ((x ==   0) && ((xx == 0) || (xx == 1))) ||  
          ((x ==   1) && ((xx == 0))) ||  
          ((x == W-2) && ((xx == 4))) ||
          ((x == W-1) && ((xx == 3) || (xx == 4)))) {
        sum[n*W*W+y*W+x] += 0; // boundary
      } else {
        sum[n*W*W+y*W+x] += (w[yy*D*5*N+xx*D*N+z*N+n] * a[z*W*W+(y-2+yy)*W+(x-2+xx)]);
      }   
    }   
  }

  return 0;
} //}}}

int square_act (int layer, int N, int W, float *i_a, float *o_a) { //{{{
  for (int n=0; n<N; n++)
  for (int y=0; y<W; y++)
  for (int x=0; x<W; x++) {
    o_a[n*W*W+y*W+x] = i_a[n*W*W+y*W+x] * i_a[n*W*W+y*W+x];
  }

  return 0;
} //}}}

int avg_pool (int layer, int N, int W, float *i_a, float *o_a) { //{{{
  for (int n=0; n<N; n++)
  for (int y=0; y<W; y+=2)
  for (int x=0; x<W; x+=2) {
    o_a[n*W/2*W/2+y/2*W/2+x/2] = (i_a[n*W*W+(y  )*W+x] 
                                + i_a[n*W*W+(y  )*W+(x+1)] 
                                + i_a[n*W*W+(y+1)*W+x] 
                                + i_a[n*W*W+(y+1)*W+(x+1)]) / 4;
  }

  return 0;
} //}}}

int fc (int layer, int W, int H, float *a, float *w, float *sum) { //{{{
  for (int j=0; j<W; j++)
  for (int i=0; i<H; i++) {
    sum[j] += (a[i] * w[i*W+j]);
  }

  return 0;
} //}}}

int fc_square_act (int layer, int W, float *i_a, float *o_a) { //{{{
  for (int x=0; x<W; x++) {
    o_a[x] = i_a[x] * i_a[x];
  }

  return 0;
} //}}}

int reshape (int layer, int N, int W, float *i_a, float *o_a) { //{{{
  for (int n=0; n<N; n++)
  for (int yy=0; yy<W; yy++)
  for (int xx=0; xx<W; xx++) {
    o_a[yy*N*W+xx*N+n] = i_a[n*W*W+yy*W+xx];
  }
  
  return 0;
} //}}}

int fc_sigmoid_act (int W, float *i_a, float *o_a) { //{{{
  for (int x=0; x<W; x++) {
    o_a[x] = 1 / (1 + exp(-i_a[x]));
  }

  return 0;
} //}}}

char classify (int W, float *o_a) { //{{{
  float max = 0;
  char idx = 0;

  for (int i=0; i<W; i++) {
    if (max < o_a[i]) {
      max = o_a[i];
      idx = (char)i;
    }
  }
  
  return idx;
} //}}}

char nn (float *i_img, float *w_conv1, float *w_conv2, float *w_fc1, float *w_fc2) { //{{{
  char ret;

  // arrays for convolution sums
  float s_conv1    [28*28*5];
  float s_conv2    [14*14*50];
  float s_fc1      [100];
  float s_fc2      [10];

  // arrays for activations
  float a_conv1    [28*28*5];
  float a_conv2    [14*14*50];
  float a_fc1      [100];
  float a_fc2      [10];

  // arrays for activations after pooling and reshaping
  float a_conv1_PL [14*14*5];
  float a_conv2_PL [7*7*50];
  float a_conv2_RS [7*7*50];
  
  // memory reset
  memset(s_conv1, 0, sizeof(s_conv1));
  memset(s_conv2, 0, sizeof(s_conv2));
  memset(s_fc1, 0, sizeof(s_fc1));
  memset(s_fc2, 0, sizeof(s_fc2));
  memset(a_conv1, 0, sizeof(a_conv1));
  memset(a_conv2, 0, sizeof(a_conv2));
  memset(a_fc1, 0, sizeof(a_fc1));
  memset(a_fc2, 0, sizeof(a_fc2));
  memset(a_conv1_PL, 0, sizeof(a_conv1_PL));
  memset(a_conv2_PL, 0, sizeof(a_conv2_PL));
  memset(a_conv2_RS, 0, sizeof(a_conv2_RS));
  
  // conv1
  conv(1, 1, 5, 28, i_img, w_conv1, s_conv1);
  // square activation
  square_act(1, 5, 28, s_conv1, a_conv1);
  // AVG pool
  avg_pool(1, 5, 28, a_conv1, a_conv1_PL);
  
  // conv2
  conv(2, 5, 50, 14, a_conv1_PL, w_conv2, s_conv2);
  // AVG pool
  avg_pool(2, 50, 14, s_conv2, a_conv2_PL);
  // reshape
  reshape(2, 50, 7, a_conv2_PL, a_conv2_RS);
  
  // FC1 (W = 100, H = 7x7x50)
  fc(3, 100, 2450, a_conv2_RS, w_fc1, s_fc1);
  // square activation
  fc_square_act(3, 100, s_fc1, a_fc1);
  
  // FC2 (W = 10, H = 100)
  fc(4, 10, 100, a_fc1, w_fc2, s_fc2);
  // sigmoid activation
  fc_sigmoid_act (10, s_fc2, a_fc2);
  
  ret = classify(10, a_fc2);

  return ret;
} //}}}
int output_gpio17(int num){
  GPIO outGPIO(17);    							// pin 11 setting
  outGPIO.setDirection(OUTPUT);    	// basic output example
  outGPIO.setValue(LOW);
  for (num; num > 0; num--){        	// flash the LED 10 times
      outGPIO.setValue(HIGH);       	// turn the LED on
      usleep(500000);               	// sleep for 0.5 seconds
      outGPIO.setValue(LOW);        	// turn the LED off
      usleep(500000);               	// sleep for 0.5 seconds
   }

  return 0;
}

int main() { //{{{
  char pred;
  unsigned char *buf = (unsigned char *)malloc(28*28*sizeof(unsigned char));
  float *buf_f       = (float *)malloc(28*28*sizeof(float));
  
  // read an image
  read_img(buf);
 
  // read parameters
  float w_conv1    [5*5*1*5];
  float w_conv2    [5*5*5*50];
  float w_fc1      [7*7*50*100];
  float w_fc2      [100*10];

  FILE *f_w_conv1 = fopen("../trained_parameter/W_conv1.ckpt.data-00000-of-00001", "rb");
  FILE *f_w_conv2 = fopen("../trained_parameter/W_conv2.ckpt.data-00000-of-00001", "rb");
  FILE *f_w_fc1   = fopen("../trained_parameter/W_fc1.ckpt.data-00000-of-00001", "rb");
  FILE *f_w_fc2   = fopen("../trained_parameter/W_fc2.ckpt.data-00000-of-00001", "rb");
  assert(f_w_conv1);
  assert(f_w_conv2);
  assert(f_w_fc1);
  assert(f_w_fc2);

  size_t n_w_conv1 = fread(w_conv1, sizeof(float), countof(w_conv1), f_w_conv1);
  size_t n_w_conv2 = fread(w_conv2, sizeof(float), countof(w_conv2), f_w_conv2);
  size_t n_w_fc1   = fread(w_fc1, sizeof(float), countof(w_fc1), f_w_fc1);
  size_t n_w_fc2   = fread(w_fc2, sizeof(float), countof(w_fc2), f_w_fc2);
  assert(n_w_conv1 == countof(w_conv1));
  assert(n_w_conv2 == countof(w_conv2));
  assert(n_w_fc1 == countof(w_fc1));
  assert(n_w_fc2 == countof(w_fc2));

  // ML inference
  for (int y=0; y<28; y++)
  for (int x=0; x<28; x++) {
    buf_f[y*28+x] = ((float)buf[y*28+x])/255;
  }

  pred = nn(buf_f, w_conv1, w_conv2, w_fc1, w_fc2);
  printf("predicted number is %d\n", (int)pred);
  printf("Output to GPIO17");
  output_gpio17((int) pred);


  // buffer free
  free(buf);
  free(buf_f);

  return 0;
} //}}}
