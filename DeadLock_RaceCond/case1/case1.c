#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include <linux/ioctl.h>
#include <pthread.h>

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)

#define ASP_DEVICE "/dev/a5"

pthread_t threads[2];

void *pthread_open(void *arg){
    int *i = (int*)arg;
    int id = *i;

    printf("thread No. %d is running\n",id);

    int fd;
    fd = open(ASP_DEVICE,O_RDWR);
    if(fd==-1){
        printf("open failed\n");
    }
    printf("open device %d successful\n", fd);
}

void *pthread_change_mode(void *arg){
    int *i = (int*) arg;
    int id = *i;

    printf("thread No. %d is running\n",id);

    int fd;
    fd = open(ASP_DEVICE,O_RDWR);
    if(fd==-1){
        printf("open failed\n");
    }
    printf("open device %d successful\n", fd);
    ioctl(fd,E2_IOCMODE2,1);
}

void main(){
    
    int x = 0;
    int y = 1;

    pthread_create(&(threads[x]), NULL, pthread_change_mode, (void*)&x);
	pthread_create(&(threads[y]), NULL, pthread_open, (void*)&y);

    pthread_join(threads[x], NULL);
    pthread_join(threads[y], NULL);

    pthread_exit(NULL);
}