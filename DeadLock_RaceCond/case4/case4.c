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

#define DEVICE "/dev/a5"

char buf[30] = "Hello World";
pthread_mutex_t lock;

pthread_t threads[2];

struct data
{
	int threadId;
	int  filedesc;
};

struct data data1,data2;

void *test0(void *arg)
{
	struct data *i = (struct data*)arg;
	int id  = i->threadId;
	int fd0 = i->filedesc;
	char buffer[30];
	
	printf("\n Thread id %d has started \n", id);
	printf("Thread id %d trying to acquire lock \n",id);
	pthread_mutex_lock(&lock);
	        printf("Thread id %d inside critical region \n",id);
		read(fd0,buffer,sizeof(buffer));
		strcpy(buf,buffer);
	pthread_mutex_unlock(&lock);		
	printf("Thread id %d released lock \n",id);	
	printf("Data Read using thread %d from device \n",id);
	close(fd0);
	printf("\n Thread id %d closed device \n", id);	
	pthread_exit(NULL);
 
}

void *test1(void *arg)
{
	struct data *i = (struct data*)arg;
	int id  = i->threadId;
	int fd0 = i->filedesc;
        char buffer[30];
	printf("\n Thread id %d has started \n", id);
        printf("Thread id %d trying to acquire lock \n",id);
	pthread_mutex_lock(&lock);
		printf("Thread id %d inside critical region \n",id);
		strcpy(buffer,buf);
		write(fd0,buffer,sizeof(buffer));
		ioctl(fd0,E2_IOCMODE1,1);
	pthread_mutex_unlock(&lock);
        printf("Thread id %d released lock \n",id);
        printf("Data Written using thread %d to device \n",id);
	close(fd0);
	printf("\n Thread id %d closed device \n", id);
	pthread_exit(NULL);
 
}

int main() 
{
	int fd0,fd1;
	char* msg;
	
	if (pthread_mutex_init(&lock, NULL) != 0)
           {
		printf("\n mutex init failed\n");
		return 1;
           }
	
	fd0 = open(DEVICE, O_RDWR);
        if(fd0 == -1)
	{
		msg =   strerror(errno);
                printf("File %s either does not exist or has been locked by another process\n", DEVICE);
                fprintf(stderr, "open failed: %s\n", msg);
		exit(-1);
	}
        printf(" Device opened from main thread in mode 1 file descriptor =  %d \n",fd0);
	ioctl(fd0,E2_IOCMODE2,2);
	printf("Device mode changed to mode 2 \n");
	fd1 = open(DEVICE, O_RDWR);
        if(fd1 == -1)
	{
		msg =   strerror(errno);
                printf("File %s either does not exist or has been locked by another process\n", DEVICE);
                fprintf(stderr, "open failed: %s\n", msg);
		exit(-1);
	}
        printf(" Device opened from main thread in mode 2 file descriptor =  %d \n",fd1);
	
	
	data1.threadId = 0;
	data1.filedesc = fd0;
	pthread_create(&(threads[0]), NULL, test0, (void*)&data1);
	
	data2.threadId = 1;
	data2.filedesc = fd1;
	pthread_create(&(threads[1]), NULL, test1, (void*)&data2);
	
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	
	pthread_mutex_destroy(&lock);
	pthread_exit(NULL);
}
