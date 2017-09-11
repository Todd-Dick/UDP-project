// dakota gray -- Todd Dick -- spring 2017 uah cpe 435 lab project
// distributed fourier transform -- client.cpp
// cooley-tukey radix 2 implementation
// compile command ---------
//			g++ -lpthread client.cpp -o client -std=c++11
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <complex.h>
#include <complex>
#include <random>
#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <type_traits>
#include <typeinfo>
#include <cxxabi.h>
#include <pthread.h>
#include <sys/time.h>


struct timeval tv1,tv2;

#define TIMER_CLEAR (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec = 0)
#define TIMER_START gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_ELAPSED ((tv2.tv_usec-tv1.tv_usec)+((tv2.tv_sec-tv1.tv_sec)*1000000))
#define TIMER_STOP gettimeofday(&tv2, (struct timezone*)0)

#define PORT 1234
#define MAX 4096
#define ROWS 2048
//#define M_PI 3.1415926535897932384

using namespace std;

int cid;
//make 2048 size array of pointers to type double complex
typedef complex<double> complexarray;
complexarray *bufferArray[ROWS];
double sample_step = 1.0;
int dimension = 4096;


// ================== FFT METHODS =====================================
int log2(int N)    /*function to calculate the log2(.) of int numbers*/
{
  int k = N, i = 0;
  while(k) {
    k >>= 1;
    i++;
  }
  return i - 1;
}

int check(int n)    //checking if the number of element is a power of 2
{
  return n > 0 && (n & (n - 1)) == 0;
}

int reverse(int N, int n)    //calculating revers number
{
  int j, p = 0;
  for(j = 1; j <= log2(N); j++) {
    if(n & (1 << (log2(N) - j)))
      p |= 1 << (j - 1);
  }
  return p;
}

void ordina(complex<double>* f1, int N) //using the reverse order in the array
{
  complex<double> f2[MAX];
  for(int i = 0; i < N; i++)
    f2[i] = f1[reverse(N, i)];
  for(int j = 0; j < N; j++)
    f1[j] = f2[j];
}

void transform(complex<double>* f, int N) //
{
  ordina(f, N);    //first: reverse order
  complex<double> *W;
  W = (complex<double> *)malloc(N / 2 * sizeof(complex<double>));
  W[1] = polar(1., -2. * M_PI / N);
  W[0] = 1;
  for(int i = 2; i < N / 2; i++)
    W[i] = pow(W[1], i);
  int n = 1;
  int a = N / 2;
  for(int j = 0; j < log2(N); j++) {
    for(int i = 0; i < N; i++) {
      if(!(i & n)) {
        complex<double> temp = f[i];
        complex<double> Temp = W[(i * a) % (n * a)] * f[i + n];
        f[i] = temp + Temp;
        f[i + n] = temp - Temp;
      }
    }
    n *= 2;
    a = a / 2;
  }
}

void FFT(complex<double>* f, int N, double d)
{
  transform(f, N);
  for(int i = 0; i < N; i++)
    f[i] *= d; //multiplying by step
}
// ===========END FFT METHODS =========================================

// ===============THREAD HANDLE FUNCTION===============================
void * threadFFT(void * arg){ 
 int *start_end = (int *) arg;
 int start = start_end[0];
 int end = start_end[1];
 int tid = start_end[2];
 int chunksize = end - start;
 char tempchar [256]; 
 // create array
 /* call to remove file: std::remove("/home/js/file.txt"); */
 // call FFT on chunk
 for(int row = start; row < (start + chunksize - 1);row++){
 	FFT(bufferArray[row],dimension,sample_step);
 }
 // end FFT call
 //cout << "inside thread: " << tid <<  endl << "start: "<< start << "  end: " << end << endl;
 
}
// ==================================MAIN=================================
int main(int argc,char * argv[]){
 struct sockaddr_in server;
 struct hostent *host_info;
 int sock,i,j;
 const char *server_name = "127.0.0.1";
 char readbuf[80],writebuf[80];

 // socket setup
 sock = socket(AF_INET,/*SOCK_DGRAM*/SOCK_STREAM,0);

 if (sock < 0)
 {
  perror("creating stream socket");
  exit(1);
 }

 host_info = gethostbyname(server_name);

 if (host_info == NULL)
 {
  fprintf(stderr,"%s:unknown host: %s\n",argv[0],server_name);
  exit(2);
 }

 server.sin_family = host_info->h_addrtype;
 memcpy((char*)&server.sin_addr,host_info->h_addr,host_info->h_length);
 server.sin_port = htons(PORT);

 // CONNECT TO SERVER
 if (connect(sock,(struct sockaddr *)&server,sizeof(server)) < 0)
 {
  perror("connecting to server");
  exit(3);
 }

 printf("connect to server %s\n",server_name);
 // pull filename from server
 read(sock,readbuf,80);
 printf("filename to read from:'%s'",readbuf);
 printf("\n\n");
 //strncmp returns 0 if strings are equal up to num characters
 if((strncmp(readbuf,"subArr1.dat",11)) == 0){
 	cid = 1;
	cout << "fetching subArr1.dat" << endl;
 }
 else {
	cid = 2;
	cout << "fetching subArr2.dat" << endl;
 }

 // open file stream for read
 float f;
 // USED FILE * TO ALLOW FOR FSCANF()
 FILE * in;
 in = fopen(readbuf,"r");
 // FILL ARRAY
	cout << " reading from: '" << readbuf << "'" << endl;
	cout << "filling array" << endl;
	for(int i = 0;i<ROWS;i++){
		bufferArray[i] = new complexarray[4096];
		for(int x = 0; x<4096;x++){
			fscanf(in,"%f",&f);
			bufferArray[i][x] = f;
		}
	}
	
 cout << "debug:  " << bufferArray[2047][4095] << endl;

 // END FILL ARRAY
 fclose(in);
 
 // 	prep threads
 int threadcnt;
 cout << "enter threadcount: (4,8,16)" << endl;
 cin >> threadcnt;
 // get chunk size
 int rowsper = 2048/threadcnt;
 
 pthread_t threads[threadcnt];
 int errcode;
 int start_end[threadcnt][3]; // array of arguments to threads
 // for loop to spawn threads

 TIMER_CLEAR;
 TIMER_START;
 for(int tid = 0; tid < threadcnt; tid++){
	start_end[tid][0] = tid*rowsper; // init start val for thread 'tid'
	start_end[tid][1] = (start_end[tid][0] + rowsper) - 1; // init end val
	start_end[tid][2] = tid; // give thread its tid
	//make threads
	//cout << "spawning thread: " << tid << endl;
	errcode = pthread_create(&threads[tid], NULL, threadFFT, &start_end[tid]);
	if(errcode!=0) {
		cerr << "Pthread creation Error: " << strerror(errcode) << endl;
		exit(1);
	}
 }
 cout << "waiting to join threads" << endl;
 //join threads
 for(int tid = 0; tid < threadcnt; tid++){
 	errcode = pthread_join(threads[tid], NULL);
 }
 if(errcode){
	cerr << "Pthread join Error: " << strerror(errcode) << endl;
	exit(1);
 }
 TIMER_STOP;
 cout << "\n Time needed for parallel fft with " << threadcnt << " threads: " << TIMER_ELAPSED/1000.0 << "ms\n";
 // DONE WITH THREADS
 // /* call to remove file: */
	if(cid == 1)
	remove("/home/student/dcg0006/CPE435/project/new_dir/subArr1.dat");
	if(cid == 2) 
	remove("/home/student/dcg0006/CPE435/project/new_dir/subArr2.dat");
 
 ofstream out;
 if(cid == 1){
	cout << "Writing output to file from client: " << cid << endl;
	out.open("client_FFT.dat");
	
	for(int row = 0; row < 2048;row++){
		for(int col = 0;col < 4096;col++){
			out << bufferArray[row][col] << endl;
		}
	}
	out.close();
	// write to socket that client1 done writing output file
	char tmp[] = "done1";
	write(sock,tmp,strlen(tmp)+1);
 }
 else if(cid == 2){
	cout << "Waiting for client 1 to finish to write output to file. . ." << endl;
		
		sleep(30);
		cout << "writing to file . . ." << endl;

			cout << "writing client2 output to file . . ." << endl;
			out.open("client_FFT.dat",ofstream::app);
			// append client 2 fft results to fft result file
			for(int row = 0; row < 2048;row++){
				for(int col = 0;col < 4096;col++){
					out << bufferArray[row][col] << endl;
				}
			}
			out.close();
			char tmp[] = "done2";
			write(sock,tmp,strlen(tmp)+1);
 }

}


//end of file
