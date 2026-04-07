// 要用-pthread参数来编译
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUMBER_OF_THREADS 10 // 表示要创建10个线程
// 线程函数，参数是线程ID，每个线程启动之后都会执行这个函数，打印一行问候语和线程ID
void *print_hello_world(void *tid){ 
 printf("Hello World. Greetings from thread %d\n",tid); 
 pthread_exit(NULL);  // 线程函数结束，退出线程
} 
// 在main里面创建NUMBER_OF_THREADS个线程，每个线程执行print_hello_world函数，传入线程ID作为参数
int main(int argc, char *argv[]) { 
 pthread_t threads[NUMBER_OF_THREADS];  // 线程组
 int status, i; 
 for(i=0; i < NUMBER_OF_THREADS; i++){ 
 printf("Main here. Creating thread %d\n", i); 
 // create函数的参数：线程ID，线程属性（NULL表示默认属性），线程函数，线程函数的参数
 status = pthread_create(&threads[i], NULL, print_hello_world, (void *)i); // 创建线程，传入线程ID作为参数
 // 异常处理
 if (status != 0) { 
 printf("Oops. pthread_create returned error code %d\n", status); 
 exit(-1); 
 } 
 } 
 // 主线程阻塞自己，等待所有线程结束
 for (i=0; i < NUMBER_OF_THREADS; i++){ pthread_join(threads[i], NULL);}  // 等待所有线程结束
 return 0;
}