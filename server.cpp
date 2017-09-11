// dakota gray -- Todd Dick -- spring 2017 uah cpe 435 lab project
// distributed fast  fourier transforms
// coley-tukey radix-2 implementation
// compile command-----
//				g++ server.cpp -o -server -std-c++11
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
#include <complex>
#include <random>
#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <type_traits>
#include <typeinfo>
#include <cxxabi.h>
#include <sys/time.h>
#include <sys/wait.h>



struct timeval tv1,tv2;

#define TIMER_CLEAR (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec = 0)
#define TIMER_START gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_ELAPSED ((tv2.tv_usec-tv1.tv_usec)+((tv2.tv_sec-tv1.tv_sec)*1000000))
#define TIMER_STOP gettimeofday(&tv2, (struct timezone*)0)

#define PORT 1234
#define MAX 4096
//#define M_PI 3.1415926535897932384
int num_clients = 0;
int done_clients = 0;

using namespace std;


// ======================FFT METHODS====================================
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
// ==============END OF FFT METHODS =========================================

// ======================MAIN================================================

int main(int argc, char * argv[]){
	struct sockaddr_in serv,cli;
	int sd,i,fd;
	char buf1[80],buf2[80],msg1[80],msg2[80];

//Create Socket
	sd = socket(AF_INET,/*SOCK_DGRAM*/SOCK_STREAM,0);
	if(sd < 0){
		printf("\n Error Creating Socket\n");
		exit(1);
	}
//Server Address Parameters
	serv.sin_family = AF_INET;
	serv.sin_port = htons(PORT)/*htons(atoi(argv[1]))*/;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
//Bind Server Address
	int errcode;
	errcode = (bind(sd,(struct sockaddr *) &serv,sizeof(serv)));
	if(errcode){
		cerr << strerror(errcode) << endl;
		printf("\n Binding Error\n");
		exit(2);
	}
	printf("\n Creating Socket & Binding it OK\n");

	listen(sd,2);
// Create array file

//Create Array
cout << "Creating initial array and filling with random values." << endl;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0,1000000);

	auto mastArray_t = new double[4096][4096];

    for(int j= 0; j < 4096; j++) {
        for(int k= 0; k < 4096; k++) {
            mastArray_t[j][k] = dis(gen);
        }
    }


//Split the array
cout << "Creating sub-arrays for use of two clients." << endl;
	auto subArr1 = new double[2048][4096];
	auto subArr2 = new double[2048][4096];
    for (int j= 0; j <2048; j++) {
        for(int k= 0; k <4096; k++) {
            subArr1[j][k] = mastArray_t[j][k];
            subArr2[j][k] = mastArray_t[j+2048][k];
        }
    }

//Write subarrays to files. 

cout << "Storing sub-arrays to files." << endl;
    ofstream out1, out2;
    out1.open("subArr1.dat");
    out2.open("subArr2.dat");
    for (int j =0; j < 2048; j++) {
        for (int k= 0; k < 4096; k++) {
            out1 << subArr1[j][k] << endl;
            out2 << subArr2[j][k] << endl;
        }
    }
    out1.close();
    out2.close();
	
   delete[] subArr1;
   delete[] subArr2;
  
   auto mastArray = new complex<double>[4096][4096];

    for(int j= 0; j < 4096; j++) {
        for(int k= 0; k < 4096; k++) {
            mastArray[j][k] = mastArray_t[j][k];
        }
    }
   delete[] mastArray_t;
// done creating array
cout << "Done writing sub arrays, ready to accept clients!" << endl;
//Start connection

	int client_len;
	while(num_clients < 2){
		client_len = sizeof(cli);
  		
		if( (fd = accept(sd,(struct sockaddr *)&cli,(socklen_t *)&client_len) )>=0){
			num_clients++;
  			if (!fork())
   			{
				if(num_clients ==1){
					char temp[] = "subArr1.dat";
					strcpy(msg1,temp);
					write(fd,msg1,strlen(msg1)+1);
					cout << "transmitted msg: " << msg1 << endl;
					cout << "one client accepted." << endl;
					read(fd,buf1,80);
					cout << buf1 << endl;
				}

				if(num_clients == 2){
					char temp[] = "subArr2.dat";
					//temp.copy(msg2,80);
					strcpy(msg2,temp);
					cout << "transmitted msg: " << msg2 << endl;
					write(fd,msg2,strlen(msg2)+1);
					cout << "both clients accepted." << endl;
					cout << "waiting on clients to calculate . . ." << endl;
					write(fd,buf1,strlen(buf1)+1);
					read(fd,buf2,80);
					cout << buf2 << endl;
				}
				if(num_clients > 2){
					char temp[] = "too many clients already accepted.";
					cout << "transmitted msg: " << temp << endl;
					write(fd,temp,strlen(temp)+1);
				}

				
   				exit(0);
   			}
			else if(num_clients==2){
			wait(0);
			wait(0);}
		}
 		else{
 			perror("accepting connection");
		}
	}
	cout << "Closing socket." << endl;
//Close Socket & connection descriptors	
	close(fd);
	close(sd);

// SERIAL FFT CODE
cout << "Performing serial FFT and storing output." << endl;
double sample_step = 1;
int dimension = 4096;
ofstream fft_serial;
fft_serial.open("fft_serial.dat");
auto tempArr = new complex<double>[4096];

	TIMER_CLEAR;
	TIMER_START;
	for(int row = 0;row < 4096;row++){
		FFT(mastArray[row],dimension,sample_step); }
	TIMER_STOP;

	cout << "\n Time needed for fft serial: " << TIMER_ELAPSED/1000.0 << "ms\n";	
	
	// outer loop for each fft on each row
for(int row = 0; row < 4096; row++){
	// fill temparray with new row
	for(int col = 0; col < 4096; col++) {
    		tempArr[col] = mastArray[row][col];
	}
	// call fft on that row
	//FFT(tempArr,dimension,sample_step);
	// print results of latest fft to file
	for(int i = 0; i < 4096; i++){
    		fft_serial << tempArr[i] << endl;
	}
}
// END SERIAL FFT CODE
fft_serial.close();
delete[] tempArr;

// leave main
	delete[] mastArray;
	//remove("/home/student/dcg0006/CPE435/project/new_dir/fft_serial.dat");
	//remove("/home/student/dcg0006/CPE435/project/new_dir/client_FFT.dat");
	return 0;
}
//end of file
