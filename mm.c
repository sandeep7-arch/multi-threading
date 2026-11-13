#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
int n; //global variable

struct mm { 
  const int* M;
  const int* m;
  int* out;
  int s;
  int e;
  int B;
  double elapsed;
};

void native_mm(int *M , int *m , int *out) {
  for(int i = 0 ; i < n ; i++) {
    for(int j = 0 ; j < n ; j++) {
      int tmp = 0;
     for(int k = 0 ; k < n ; k++) {
        tmp += M[i*n+k]*m[k*n+j];
      } 
      out[i*n+j] = tmp;
    }
  }
}

void tile_mm(int *M , int *m , int *out , int B) {
 
  for(int si = 0 ; si < n ; si+=B) {
    for(int sj = 0 ; sj < n ; sj+=B) {
      for(int sk = 0 ; sk < n ; sk+=B) {

       for(int i = si; i < si+B && i < n; i++) {
    for(int k = sk; k < sk+B && k < n; k++) {

        int r = M[i*n+k]; //cache behavior bro

        for(int j = sj; j < sj+B && j < n; j++) {
            out[i*n+j] += r * m[k*n+j];
        }
    }
}      }
     }
  }

}

void* thread_native_mm(void* arg) {
  struct mm* args = (struct mm*)arg;

  for(int i = args->s ; i < args->e ; i++) {
    for(int j = 0 ; j < n ; j++) {
      int tmp = 0 ;
      for(int k = 0 ; k < n ; k++) {
        tmp += args->M[i*n+k]*(args->m[k*n+j]);
      }
      args->out[i*n+j] = tmp;
    }
  }
  return NULL;
}


void* thread_tile_mm(void* arg) {
    struct mm *a = (struct mm*)arg;
    const int N = n, B = a->B, e = a->e;
    const int* restrict M = a->M;
    const int* restrict m = a->m;
    int* restrict out = a->out;

    for (int si = a->s; si < e; si += B)
        for (int sj = 0; sj < N; sj += B)
            for (int sk = 0; sk < N; sk += B)
                for (int i = si; i < si + B && i < e; i++)
                    for (int k = sk; k < sk + B && k < N; k++) {
                        int r = M[i*N + k];
                        for (int j = sj; j < sj + B && j < N; j++)
                            out[i*N + j] += r * m[k*N + j];
                    }
    return NULL;
}
int main() {
  srand(time(NULL));
  scanf("%d" , &n);
  printf("Populating the TWO Square matrices of Size %d\n" , n);
  int* M = (int*)malloc(n*n*sizeof(int)); 
  int* m = (int*)malloc(n*n*sizeof(int)); 
  int* out = (int*)malloc(n*n*sizeof(int)); 
  for(int i = 0 ; i < n ; i++) {
    for(int j = 0 ; j < n ; j++) {
      M[i*n+j] = rand()%936;
      m[i*n+j] = rand()%789;
    }
  }
  printf("Populating the matrices completed!!\n");
  struct timespec start , end;
  double time_taken;

  clock_gettime(CLOCK_MONOTONIC , &start);
  native_mm(M , m , out);
  clock_gettime(CLOCK_MONOTONIC , &end);
  time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  printf("NATIVE MM : %.9f\n" , time_taken);


  struct mm arg;
  arg.M = M;
  arg.m = m;
  arg.out = out;
  arg.s = 0;
  arg.e = n/2;
  struct mm arg1;
  arg1.M = M;
  arg1.m = m;
  arg1.out = out;
  arg1.s = n/2;
  arg1.e = n;
  pthread_t t1 , t2;
  clock_gettime(CLOCK_MONOTONIC , &start);
  pthread_create(&t1 , NULL , thread_native_mm , &arg);
  pthread_create(&t2 , NULL , thread_native_mm , &arg1);
  pthread_join(t1 , NULL);
  pthread_join(t2 , NULL);
  clock_gettime(CLOCK_MONOTONIC , &end);
  time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  printf("THREADS NATIVE MM : %.9f\n" , time_taken);

  for(int i = 0 ; i < n ; i++) {
    for(int j = 0 ; j < n ; j++) {
      out[i*n+j] = 0;
    }
  }

  clock_gettime(CLOCK_MONOTONIC , &start);
  tile_mm(M , m , out , 32);
  clock_gettime(CLOCK_MONOTONIC , &end);
  time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  printf("TILE MM : %.9f\n" , time_taken);

  for(int i = 0 ; i < n ; i++) {
    for(int j = 0 ; j < n ; j++) {
      out[i*n+j] = 0;
    }
  }

  struct mm args[2];
  pthread_t t[2];
  int B = 32;
  int total_tiles = (n + B - 1) / B;
  int chunk = (total_tiles + 1) / 2;
  time_taken = 0; 
   for(int i = 0; i < 2; i++) {
       int start_tile = i * chunk;
       int end_tile = (i == 0) ? chunk : total_tiles;
       args[i].M = M;
       args[i].m = m;
       args[i].out = out;
       args[i].B = B;
  
       args[i].s = start_tile * B;
       args[i].e = (end_tile * B < n) ? end_tile * B : n;
  
       }

   clock_gettime(CLOCK_MONOTONIC, &start);
   for(int i = 0; i < 2; i++) {
       pthread_create(&t[i], NULL, thread_tile_mm, &args[i]);
   }
  
   for(int i = 0; i < 2; i++) {
       pthread_join(t[i], NULL);
   }
   clock_gettime(CLOCK_MONOTONIC, &end);
  
  
   time_taken = (end.tv_sec - start.tv_sec)
             + (end.tv_nsec - start.tv_nsec) / 1e9;
  
  printf("THREADS TILE MM : %.9f\n", time_taken);
  // printf("Thread 1: %.6f\n", args[0].elapsed);
  // printf("Thread 2: %.6f\n", args[1].elapsed);

  
  

// struct mm arg_2;
//
// arg_2.M = M;
// arg_2.m = m;
// arg_2.out = out;
// arg_2.s = 0;
// arg_2.e = n;
// arg_2.B = 32;
//
// clock_gettime(CLOCK_MONOTONIC, &start);
//
// thread_tile_mm(&arg_2);
//
// clock_gettime(CLOCK_MONOTONIC, &end);
//
// time_taken = (end.tv_sec - start.tv_sec) +
//              (end.tv_nsec - start.tv_nsec) / 1e9;
//
// printf("SINGLE THREAD TILE MM : %.9f\n", time_taken);
//
  free(M);
  free(m);
  free(out);


    return 0;
}
