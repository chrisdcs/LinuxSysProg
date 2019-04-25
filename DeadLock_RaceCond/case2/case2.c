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

pthread_t threads[3];

int fd;
char buf[30] = "Hello Device Driver!";

void *pthread_write(void *arg){
    int *i = (int*)arg;
    int id = *i;

    printf("thread No. %d is running\n",id);

    //int fd;
    fd = open(ASP_DEVICE,O_RDWR);
    if(fd==-1){
        printf("open failed\n");
    }
    printf("open device %d successful\n", fd);

    char buffer[30];
    write(fd,buf,sizeof(buf));
    read(fd,buffer,sizeof(buffer));

    printf("%s\n",buffer);
}

void *pthread_change_mode_1_to_2(void *arg){
    int *i = (int*) arg;
    int id = *i;

    printf("thread No. %d is running\n",id);

    ioctl(fd,E2_IOCMODE2,1);
}

void *pthread_change_mode_2_to_1(void *arg){
    int *i = (int*) arg;
    int id = *i;

    printf("thread No. %d is running\n",id);

    ioctl(fd,E2_IOCMODE1,2);
}

void main(){
    
    int x = 0;
    int y = 1;
    int z = 2;

    fd = open(ASP_DEVICE,O_RDWR);
    pthread_create(&(threads[z]), NULL, pthread_change_mode_2_to_1, (void*)&z);
    pthread_create(&(threads[y]), NULL, pthread_write, (void*)&y);
    pthread_create(&(threads[x]), NULL, pthread_change_mode_1_to_2, (void*)&x);
    
	
    pthread_join(threads[y], NULL);
    pthread_join(threads[x], NULL);
    
    //pthread_join(threads[z], NULL);

    pthread_exit(NULL);
}