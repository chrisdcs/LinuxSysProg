#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#define MAX_STR_LEN     50
#define sort_size       20
#define event_size      10

struct shared_data {

    pthread_mutex_t mtx;
    pthread_cond_t full;
    pthread_cond_t empty;

    int test;

    int num_items;
    int flag;
    int avail;
    int no_data;
    int cntr;
    int m;
    int n;
    char buffer[100][50];
    int **sort;
    char ***events;
    char *swap;
    char items[MAX_STR_LEN];
    char input[MAX_STR_LEN];
    FILE *fp;
};

void initialize_everything(struct shared_data *s, int m_items){
    s->test = 0;
    s->avail = 1;
    s->no_data = 0;
    s->cntr = 0;
    s->m = 0, 
    s->n = 0;

    //s->buffer = malloc(sizeof(char*) * m_items);

    s->sort = malloc(sort_size*sizeof(int *));
    for(int i = 0;i < sort_size;i++){
        s->sort[i] = malloc(sort_size*sizeof(int));
    }

    s->events = malloc(sort_size*sizeof(char **));
    for(int i = 0;i < sort_size;i++){
        s->events[i] = malloc(sort_size*sizeof(char *));
        for(int j = 0;j < sort_size;j++){
            s->events[i][j] = malloc(MAX_STR_LEN*sizeof(char));
        }
    }
}

/* drop left n characters of a string */
void bubble_sort(int **arr, int n, struct shared_data *s){
    int i,j,k;
    int temp;
    for(k = 0;k < n;k++){
        for(i = 0;i < n-1;i++){
            for(j = 0;j < n-i-1;j++){
                if((arr[k][j] > arr[k][j+1]) && 
                (arr[k][j+1] != 0) && 
                (arr[k][j] != 0)){
                    s->swap = malloc(sizeof(char)*MAX_STR_LEN);
                    temp = arr[k][j];
                    strcpy(s->swap,s->events[k][j]);
                    arr[k][j] = arr[k][j+1];
                    free(s->events[k][j]);
                    s->events[k][j] = malloc(sizeof(char)*MAX_STR_LEN);
                    strcpy(s->events[k][j],s->events[k][j+1]);
                    arr[k][j+1] = temp;
                    free(s->events[k][j+1]);
                    s->events[k][j+1] = malloc(sizeof(char)*MAX_STR_LEN);
                    strcpy(s->events[k][j+1],s->swap);

                    free(s->swap);
                }
            }
        }
    }
}

int find_change(char *event, int num, struct shared_data *s){
    int flag = 0;
    char *temp1;char *temp2;
    temp1 = malloc(sizeof(char)*event_size);
    strncpy(temp1,event,10);

    for(int i = 0;i < sort_size;i++){
        for(int j = 0;j < sort_size;j++){
            temp2 = malloc(sizeof(char)*event_size);
            strncpy(temp2,s->events[i][j],10);
            if(!strcmp(temp1,temp2)){
                if(j==0){
                    flag = 1;
                    free(s->events[i][j]);
                    s->events[i][j] = malloc(sizeof(char)*MAX_STR_LEN);
                    strcpy(s->events[i][j],event);
                    s->sort[i][j] = num;
                    bubble_sort(s->sort, sort_size, s);
                }
                else
                {
                    free(s->events[i][j]);
                    s->events[i][j] = malloc(sizeof(char)*MAX_STR_LEN);
                    strcpy(s->events[i][j],event);
                    s->sort[i][j] = num;
                    bubble_sort(s->sort, sort_size, s);
                }
            }
            free(temp2);
        }
    }
    return flag;
}

int find_delete(int num,struct shared_data *s){
    int flag = -1;
    for(int i = 0;i < sort_size;i++){
        for(int j = 0;j < sort_size;j++){
            if(num == s->sort[i][j]){
                if(j == 0){
                    s->sort[i][j] = 99999999;
                    if(s->sort[i][j+1] == 99999999)
                        flag = -2;
                    else
                    {
                        flag = i;
                    }
                    
                    free(s->events[i][j]);
                    s->events[i][j] = malloc(sizeof(char) * MAX_STR_LEN);
                    bubble_sort(s->sort, sort_size, s);
                }
                else
                {
                    s->sort[i][j] = 99999999;
                    free(s->events[i][j]);
                    s->events[i][j] = malloc(sizeof(char) * MAX_STR_LEN);
                    bubble_sort(s->sort, sort_size, s);
                }
                
            }
        }
    }
    return flag;
}

void drop_left(char *s, int n){
    char* s2 = s + n;
    while ( *s2 ){
        *s = *s2;
        ++s;
        ++s2;
    }
    *s = '\0';
}

int get_number(char *input){
    char *year;char *mon;char *day;
    char *hour;char *min;
    char *number;

    year = malloc(4*sizeof(char));
    mon = malloc(2*sizeof(char));
    day = malloc(2*sizeof(char));

    hour = malloc(2*sizeof(char));
    min = malloc(2*sizeof(char));

    number = malloc(12*sizeof(char));

    strncpy(year,input+6,4);
    strncpy(mon,input,2);
    strncpy(day,input+3,2);
    strncpy(hour,input+11,2);
    strncpy(min,input+14,2);

    strcat(number,year);
    strcat(number,mon);
    strcat(number,day);
    strcat(number,hour);
    strcat(number,min);
    
    return atoi(number);
}

int update_sort(int num, char *str, struct shared_data *s){
    int flag=0;
    //printf("%d\n",num);
    if(s->cntr == 0){
        s->sort[s->m][s->n] = num;
        strcpy(s->events[s->m][s->n],str);
    }
    else
    {
        for(int i =0;i < sort_size;i++){
            if(s->sort[i][0]/10000 == num/10000){
                for (int j = 0;j < sort_size;j++){
                    if(s->sort[i][j] == 0){
                        s->sort[i][j] = num;
                        strcpy(s->events[i][j],str);
                        if(num % 10000 < s->sort[i][0] % 10000)
                            flag = 1;
                        break;
                    }
                }
                break;
            }
            else
            {
                if(s->sort[i][0] == 0){
                    s->sort[i][0] = num;
                    strcpy(s->events[i][0],str);
                    flag = 1;
                    break;
                }
            }
        }
    }
    
    return flag;
}

int compare_time(char *time1, char *time2){
    char *h1;char *h2;char *m1;char *m2;
    int is_early = 0;
    h1 = malloc(sizeof(char) * 2);
    h2 = malloc(sizeof(char) * 2);
    m1 = malloc(sizeof(char) * 2);
    m2 = malloc(sizeof(char) * 2);
    strncpy(h1,time1,2);
    strncpy(h2,time2,2);
    strncpy(m1,time1+3,2);
    strncpy(m2,time2+3,2);
    if(atoi(h1) < atoi(h2))
        is_early = 1;
    if(atoi(h1) == atoi(h2)){
        if(atoi(m1) < atoi(m2))
            is_early = 1;
    }
    else
    {
            is_early = 0;
    }
    
    return is_early;
}

void email_filter(char input[MAX_STR_LEN], struct shared_data *s){
    s->flag = 2;
    char *pt;
    char cpy[50];
    char * temp;

    drop_left(input,9);
    strcpy(cpy, input);
    pt = strtok(cpy, ",");
    int cnt = 0;
        
    int type = 0;
    int len0;int len1;int len2;int len3;int len4;
        while(pt != NULL){
            char *temp;
            temp = pt;
            pt = strtok(NULL, ",");

            /*determine if email match right format*/
            switch(cnt){
                case 0:
                    len0 = strlen(temp);
                    if(len0 > 1)
                        s->flag = 0;
                    if(*temp == 'C'){
                    // "C" here means create an event
                        type = 1;
                    }
                    else if(*temp == 'X'){
                        type = 2;
                    }
                    else if(*temp == 'D'){
                        type = 3;
                    }
                        break;
                    case 1:// event
                        len1 = strlen(temp);
                        if(len1 > 10)
                            s->flag = 0;
                        break;
                    case 2:
                        len2 = strlen(temp);
                        if(len2 != 10)
                            s->flag = 0;
                        break;
                    case 3:
                        len3 = strlen(temp);
                        if(len3 != 5)
                            s->flag = 0;
                        break;
                    case 4:
                        len4 = strlen(temp);
                        if(len4 > 10)
                            s->flag = 0;
                        else
                            s->flag = 1;
                        break;
                    default:
                        printf("error!\n");
                }
                if(s->flag == 0)
                    break;
                cnt++;
            }
            
            if(s->flag == 1){
                
                switch (type)
                {
                    case 1:
                        strcpy(s->items,input);
                        break;
                    case 2:
                        strcpy(s->items,input);
                        break;
                    case 3:
                        strcpy(s->items,input);
                        break;
                    default:
                        exit(-1);
                        break;
                }
                
            }
            else{}
}

void calendar_filter(void *arg, struct shared_data *s){
    s->fp = fopen("output.txt","a");
    char *pt;
    char *copy;
    char *event;
    char *empty;
    char *input = (char *) arg;
    int earliest;
    int type = 0;
    int num;
    int flag;
    char *t = malloc(1*sizeof(char));

    strncpy(t,input,1);
    if(*t == 'C'){
    // "C" here means create an event
        type = 1;
    }
    else if(*t == 'X'){
        type = 2;
    }
    else if(*t == 'D'){
        type = 3;
    }

    drop_left(input, 2);
    copy = malloc(sizeof(char) * 40);
    strncpy(copy,input+11,strlen(input)-11);
    //printf("%s\n",copy);
    
    if(type == 1){
        num = get_number(copy);
        flag = update_sort(num,input,s);
        bubble_sort(s->sort,sort_size, s);
        if(flag || s->cntr == 0)
            fputs(strcat(copy,"\n"), s->fp);
        s->cntr++;
    }
    if(type == 2){
    /* Make changes to string temp0 for the changed part */
        event = malloc(sizeof(char) * MAX_STR_LEN);
        strcpy(event,input);
        num = get_number(copy);
        earliest = find_change(event,num,s);
        if(earliest){
            fputs(strcat(copy,"\n"), s->fp);
        }
    }
    else if(type == 3){
    /* delete event */
        num = get_number(copy);
        earliest = find_delete(num,s);
        if(earliest >= 0 && 
        strlen(s->events[earliest][0])){
            free(copy);
            copy = malloc(sizeof(char) * MAX_STR_LEN);
            event = malloc(sizeof(char) * MAX_STR_LEN);
            strcpy(copy,s->events[earliest][0]);
            strncpy(event,copy+11,strlen(copy)-11);
            strcat(event,"\n");
            fputs(event,s->fp);
        }
        if(earliest == -2){
            empty = malloc(sizeof(char)*MAX_STR_LEN);
            strncpy(empty,copy,10);
            strcat(empty,",--:--,NA        \n");
            fputs(empty,s->fp);
        }
    }
    fclose(s->fp);
}


/* thread function 1: producer */

static void
producer(int N, struct shared_data *s){
    int in = 0;
    while (fgets(s->input, MAX_STR_LEN, stdin)){
        s->avail = 1;
        pthread_mutex_lock(&(s->mtx));
        email_filter(s->input,s);
        while(s->num_items == N){
            pthread_cond_wait(&(s->full), &(s->mtx));
        }
        // producer write into buffer
        if(s->flag == 1){
            //s->buffer[in] = malloc((MAX_STR_LEN) * sizeof(char));
            strcpy(s->buffer[in], s->items);
            in = (in + 1) % N;
            s->num_items++;
        }
        if(s->num_items == 1)
            pthread_cond_signal(&(s->empty));
        pthread_mutex_unlock(&(s->mtx));
    }
    // when get out of the loop: not quit immediately
    s->no_data = 1;
    pthread_cond_wait(&(s->full), &(s->mtx));
    pthread_exit(NULL);
}

/* thread function 2: consumer */

static void
consumer(int N, struct shared_data *s){
    char* local;
    int out = 0;
    while(s->avail){
        pthread_mutex_lock(&(s->mtx));
        while(s->num_items == 0){
            pthread_cond_wait(&(s->empty), &(s->mtx));
        }
        local = malloc(MAX_STR_LEN * sizeof (char*));
        strcpy(local, s->buffer[out]);
        out = (out + 1) % N;
        calendar_filter(local,s);
        s->num_items--;
        if(s->num_items == N - 1)
            pthread_cond_signal(&(s->full));
        pthread_mutex_unlock(&(s->mtx));

        if(s->no_data == 1 && s->num_items == 0)
            break;
    }
    pthread_cond_signal(&(s->full));
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){

    int max_num_items = atoi(argv[argc-1]);

    /* mutex and attributes */
    pthread_mutexattr_t psharedm;
    pthread_condattr_t psharedc1;
    pthread_condattr_t psharedc2;

    /* initialize and make process shared */
    (void) pthread_mutexattr_init(&psharedm);
    (void) pthread_mutexattr_setpshared(&psharedm,
    PTHREAD_PROCESS_SHARED);
    (void) pthread_condattr_init(&psharedc1);
    (void) pthread_condattr_setpshared(&psharedc1,
    PTHREAD_PROCESS_SHARED);
    (void) pthread_condattr_init(&psharedc2);
    (void) pthread_condattr_setpshared(&psharedc2,
    PTHREAD_PROCESS_SHARED);

    struct shared_data *sptr;
    /* create a mapping for shared memory */
    sptr = mmap(NULL, sizeof(struct shared_data), 
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    (void) pthread_mutex_init(&sptr->mtx, &psharedm);
    (void) pthread_cond_init(&sptr->empty, &psharedc1);
    (void) pthread_cond_init(&sptr->full, &psharedc2);
    
    /* initialize everything in parent process */
    initialize_everything(sptr,max_num_items);

    /* clear output.txt */
    sptr->fp = fopen("output.txt","w");
    fclose(sptr->fp);

    /* create child process for email filter */
    pid_t email = fork();
    pid_t calendar;

    switch(email){
    case -1:
        printf("error\n");
        exit(-1);
        break;

    case 0: /* child process email */
        /* code */
        producer(max_num_items,sptr);
        break;

    default: /* parent process: kernel */
        calendar = fork();
        switch(calendar){
        case -1:
            printf("error\n");
            exit(-1);
            break;

        case 0: /* child process calendar */
            consumer(max_num_items,sptr);
            break;
        default: /* parent process: kernel */
            wait(NULL);
            break;
    }
        wait(NULL);// wait email process to terminate
        break;
    }

    munmap((void *)sptr,sizeof(struct shared_data));
    /* program terminates: all process shut down */
    return 0;
}