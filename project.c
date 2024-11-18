#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define FILE_BUFFER_SIZE 256

typedef struct person
{
    int id,x,y,init_status,movement,amp;
}person;

typedef struct person_thread
{
    int id,first,last;
}person_thread;

//main values used in the program.
int rows,cols,N,sim_time,nr_threads;
person* people;
int* infection_counter;
int** matr;

//files for output and debugging
FILE* f2;
FILE* f3;
FILE* f_pragma_v1;
FILE* f_pragma_v2;
int choice;

//time structures
struct timespec start,finish;
struct timespec start_output,finish_output;
double elapsed,Tserial,Tparallel,elapsed_output,T_omp_v1,T_omp_v2;

//barriers for synch
pthread_barrier_t barrier_move;
pthread_barrier_t barrier_check;
pthread_barrier_t barrier_update;

#define INFECTED_DURATION 2
#define IMMUNE_DURATION 11
#define CHUNK_SIZE 1000

void initialize(char* total_sim_time,char* file_name,char* nr_threads);
void start_simulation_serial(int sim_time,int n,int* infection_counter,person* people,int** matr);
void start_simulation_parallel(int total_sim_time,int N,int *infection_counter,person* people,int** matr);
void omp_v1(int sim_time,int N,int* infection_counter,person* people,int** matr);
void move_person(person* p,int** matr);
void print_people(person* people,int n,int* infection_counter);
void copy_vector(int* infection_counter_serial,int* infection_counter);
void copy_people(person* people_serial,person* people);
void copy_matrix(int** matr_serial,int** matr);
void write_vars_to_file(person* people,FILE* out);


int main(int argc,char* argv[])
{
    if(argc < 4)
    {
        printf("Error ; not enough arguments\n");
        exit(1);
    }
    initialize(argv[1],argv[2],argv[3]);
}

void print_matrix(int** matr)
{
    for(int i=0;i<rows;i++)
    {
        for(int j=0;j<cols;j++)
        {
            printf("%d ",matr[i][j]);
        }
        printf("\n");
    }
}


void init_matrix(int** matr)
{
    for (int i=0;i<rows;i++)
    {
        for(int j=0;j<cols;j++)
        {
            matr[i][j]=0;
        }
    }
}

void move_person(person* p,int** matr)
{
    int direction=p->movement;
    //left
    if(direction==3)
    {
        //y decrease, x stays same
        int new_y=p->y-p->amp;
        if(new_y<=0)
        {
            new_y=0;
            p->movement=2;
        }
        matr[p->x][p->y]=0;
        matr[p->x][new_y]=p->id;
        p->y=new_y;
    }
    //right
    if(direction==2)
    {
        //y decrease, x stays same
        int new_y=p->y+p->amp;
        if(new_y>=cols)
        {
            new_y=cols-1;
            p->movement=3;
        }
        matr[p->x][p->y]=0;
        matr[p->x][new_y]=p->id;
        p->y=new_y;
    }
    //down
    if(direction==1)
    {
        //x decrease, y stays same
        int new_x=p->x-p->amp;
        if(new_x<=0)
        {
            new_x=0;
            p->movement=0;
        }
        matr[p->x][p->y]=0;
        // printf("%d %d\n",new_x,p->id);
        matr[new_x][p->y]=p->id;
        p->x=new_x;
    }
    //up
    if(direction==0)
    {
        //x decrease, y stays same
        int new_x=p->x+p->amp;
        if(new_x>=rows)
        {
            new_x=rows-1;
            p->movement=1;
        }
        matr[p->x][p->y]=0;
        matr[new_x][p->y]=p->id;
        p->x=new_x;
    }
}

void check_for_infections(person* people,int n,person* p)
{
    for(int i=0;i<n;i++)
    {
        if(people[i].x==p->x && people[i].y==p->y && people[i].id!=p->id)
        {
            //got one person who is on the same square as the person
            person* p2=&people[i];
            
            //check for immunity ; if either of the people are immune, we don't need to check if they can be infected.
            if(p2->init_status>=2 || p->init_status>=2)
                continue;
            //check for infections for p2 or p
            //if p2 is infected(0)
            if(p2->init_status<=0)
                {
                    //if the other person on the same space is susceptible, theyu can be infected (set to 0). Only if they are susceptible will the infection counter increase and the other person
                    //become infected. If both people are already infected, won't increase infection counter or set p->init_status to 0 since that will effectively reset the infected_duration for 
                    //that other person.
                    if(p->init_status==1)
                        {
                            p->init_status=0;
                            
                        }
                }
            //if p is infected
            else if(p->init_status<=0)
                {
                    if(p2->init_status==1)
                    {
                        p2->init_status=0;
                        
                    }
                }
        } 
    }   
}

void update_infections(person* people, int n,int* infection_counter,person* p) {
        if(p->init_status==0)
        {
            infection_counter[p->id-1]++;
        }
        //infected. check if its value is also bigger than the infected duration
        if(p->init_status<=0 && p->init_status>-INFECTED_DURATION)
        {
            p->init_status--;
        }
            
        else if(p->init_status==-INFECTED_DURATION)
        {
            //finished being infected; make person immune now.
            //1 is hardcoded here because we have two base cases. Either "0" as infected or "1" as susceptible. When we reach "1" we know that the person has finished being immune and is now susceptible again
            p->init_status=1+IMMUNE_DURATION;
        }
        else if(p->init_status>1)
        {
            p->init_status--;
        }
        //if people[i] didn't go through any of the ifs, that means that the person is susceptible (1) value ; nothing needs to be changed in this case.
}



void start_simulation_serial(int sim_time,int n,int* infection_counter,person* people,int** matr)
{
    while(sim_time>0)
    {
        // printf("Simulation nr %d\n",sim_time);
        for(int i=0;i<n;i++)
        {
            move_person(&people[i],matr);
            
        }
        for(int i=0;i<n;i++)
        {
            check_for_infections(people,n,&people[i]);
        }
        for(int i=0;i<n;i++)
            update_infections(people,n,infection_counter,&people[i]);
        // print_matrix(matr);
        // print_people(people,n,infection_counter);
        if(choice==1)
        {
            write_vars_to_file(people,f2);
            fprintf(f2,"\n");
        }
        
        sim_time--;
    }
    clock_gettime(CLOCK_MONOTONIC,&start_output);
    if(choice==0)
    {
        write_vars_to_file(people,f2);
    }
    clock_gettime(CLOCK_MONOTONIC,&finish_output);
    elapsed_output=(finish_output.tv_sec-start_output.tv_sec);
    elapsed_output+=(finish_output.tv_nsec-start_output.tv_nsec)/pow(10,9);
    // printf("\nFinished simulation\n");
    // print_matrix(matr);
    // print_people(people,n,infection_counter);
}


//simulate function
void *simulate(void* t) {
    person_thread* my_thread=(person_thread*)t;
    // printf("\n\nHello from thread nr %d ; start:%d end:%d\n", my_thread->id, my_thread->first + 1, my_thread->last + 1);

    int sim_time_threads=sim_time;  //each thread has its own simulation time

    while(sim_time_threads>0) {

        // printf("Thread %d executing simulation nr %d\n",my_thread->id,sim_time_threads);


        //move people
        for (int i=my_thread->first;i<=my_thread->last;i++) 
        {
            move_person(&people[i],matr);
        }
        // printf("Thread %d waiting at barrier move\n",my_thread->id);
        pthread_barrier_wait(&barrier_move);


        //check for infections
        for (int i=my_thread->first;i<=my_thread->last;i++) 
        {
            check_for_infections(people, N, &people[i]);
        }
        // printf("Thread %d waiting at barrier check\n",my_thread->id);
        pthread_barrier_wait(&barrier_check);


        for (int i=my_thread->first;i<=my_thread->last;i++) 
        {
            update_infections(people, N, infection_counter ,&people[i]);
        }
        // printf("Thread %d waiting at barrier update\n",my_thread->id);
        pthread_barrier_wait(&barrier_update);

        //will only print the results when the thread is the main thread executing the function ; due to racing between the threads, the output shown will most likely differ at each step from the serial version
        //but the final result will be the same
        if(choice==1 && my_thread->id==0)
        {
            write_vars_to_file(people,f3);
            fprintf(f3,"\n");
        }
        
        //decrease sim time for the thread
        sim_time_threads--;

        // printf("Thread %d decremented its sim_time to %d\n",my_thread->id,sim_time_threads);
    }

    pthread_exit(NULL);
}


void start_simulation_parallel(int total_sim_time,int N,int *infection_counter,person* people,int** matr)
{
    pthread_t* threads=(pthread_t*)malloc(sizeof(pthread_t) * nr_threads);
    person_thread* people_threads=(person_thread*)malloc(sizeof(person_thread) * nr_threads);
    if(nr_threads>N)
    {
        printf("There are more threads than people. Exiting.\n");
        return;
    }

    //initialize barriers
    pthread_barrier_init(&barrier_move,NULL,nr_threads);
    pthread_barrier_init(&barrier_check,NULL,nr_threads);
    pthread_barrier_init(&barrier_update,NULL,nr_threads);

    //divide the N people to nr_threads
    int thread_people=N/nr_threads;
    int rest=N%nr_threads;
    // printf("Each Thread contains %d people: Rest:%d\n",thread_people,rest);
    
    int tmp=0;

    for(int i=0;i<nr_threads;i++)
    {
        people_threads[i].id=i;
        people_threads[i].first=tmp;
        int people_for_this_thread=thread_people+(i<rest ? 1 : 0);
        people_threads[i].last = tmp+people_for_this_thread-1; 
        pthread_create(&threads[i],NULL,simulate,(person_thread*)&people_threads[i]);
        tmp+=people_for_this_thread;
    }

    for(int i=0;i<nr_threads;i++)
    {
        pthread_join(threads[i],NULL);
    }
    clock_gettime(CLOCK_MONOTONIC,&start_output);
    if(choice==0)
    {
        write_vars_to_file(people,f3);
    }
    clock_gettime(CLOCK_MONOTONIC,&finish_output);
    elapsed_output=(finish_output.tv_sec-start_output.tv_sec);
    elapsed_output+=(finish_output.tv_nsec-start_output.tv_nsec)/pow(10,9);
    // printf("\nFinished simulation\n");
    // print_matrix(matr);
    // print_people(people,N,infection_counter);

    //clean up barriers
    pthread_barrier_destroy(&barrier_move);
    pthread_barrier_destroy(&barrier_check);
    pthread_barrier_destroy(&barrier_update);

    // free(people_threads);
    // free(threads);
    // pthread_exit(NULL);
}


void omp_v1(int sim_time,int n,int* infection_counter,person* people,int** matr)
{
    //this version is basically the same thing as the serial version but you add parallel for's before moving checking and updating each person.
    while(sim_time>0) 
    {
        //move
        #pragma omp parallel for schedule(dynamic,CHUNK_SIZE)
        for (int i=0;i<n;i++) 
        {
            move_person(&people[i],matr);
        }

        //check
        #pragma omp parallel for schedule(dynamic,CHUNK_SIZE)
        for (int i=0;i<n;i++)
        {
            check_for_infections(people,n,&people[i]);
        }

        //update
        #pragma omp parallel for schedule(dynamic,CHUNK_SIZE)
        for (int i=0;i<n;i++)
        {
            update_infections(people,n,infection_counter,&people[i]);
        }

        //here you have it as critical since you want only one thread to write the values to the file, and while writing those values to the file, you don't want any thread to access and also write
        //to that file as the thread is executing.
        #pragma omp critical
        {
            if(choice==1)
            {
                write_vars_to_file(people,f_pragma_v1);
                fprintf(f_pragma_v1,"\n");
            }
        }
        
        #pragma omp critical
        {
            //decrease sim time
            sim_time--;
        }
        
    }
    clock_gettime(CLOCK_MONOTONIC,&start_output);
    if(choice==0)
    {
        write_vars_to_file(people,f_pragma_v1);
        fprintf(f_pragma_v1,"\n");
    }
    clock_gettime(CLOCK_MONOTONIC,&finish_output);
    elapsed_output=(finish_output.tv_sec-start_output.tv_sec);
    elapsed_output+=(finish_output.tv_nsec-start_output.tv_nsec)/pow(10,9);
}

void omp_v2(int sim_time, int n, int* infection_counter, person* people, int** matr)
{
    #pragma omp parallel
    {
        #pragma omp single
        {
            while (sim_time>0)
            {
                //move
                for (int i=0;i<n;i++)
                {
                    #pragma omp task firstprivate(i)
                    move_person(&people[i], matr);
                }
                #pragma omp taskwait  
                //check
                for (int i=0;i<n;i++)
                {
                    #pragma omp task firstprivate(i)
                    check_for_infections(people, n, &people[i]);
                }
                #pragma omp taskwait  

                //update
                for (int i=0;i<n;i++)
                {
                    #pragma omp task firstprivate(i)
                    update_infections(people, n, infection_counter, &people[i]);
                }
                #pragma omp taskwait  

                //if debug 1, write to file.
                if (choice==1)
                {
                    #pragma omp task
                    {
                        
                        #pragma omp critical
                        {
                            write_vars_to_file(people, f_pragma_v2);
                            fprintf(f_pragma_v2, "\n");
                        }
                    }
                }
                #pragma omp critical
                {
                    sim_time--;
                }
                
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC,&start_output);
    if (choice==0)
    {
        write_vars_to_file(people, f_pragma_v2);
        fprintf(f_pragma_v2, "\n");
    }
    clock_gettime(CLOCK_MONOTONIC,&finish_output);
    elapsed_output=(finish_output.tv_sec-start_output.tv_sec);
    elapsed_output+=(finish_output.tv_nsec-start_output.tv_nsec)/pow(10,9);
}

void copy_vector(int* infection_counter_serial,int* infection_counter)
{
    // printf("%d\n",N);
    
    for(int i=0;i<N;i++)
    {
        // printf("%d\n",i);
        infection_counter_serial[i]=infection_counter[i];
    }
    // printf("test\n");
}

void copy_people(person* people_serial,person* people)
{
    // printf("testing\n");
    for(int i=0;i<N;i++)
    {
        people_serial[i]=people[i];
    }
}

void copy_matrix(int** matr_serial,int** matr)
{
    for(int i=0;i<rows;i++)
    {
        for(int j=0;j<cols;j++)
        {
            matr_serial[i][j]=matr[i][j];
        }
    }
}

void write_vars_to_file(person* people,FILE* out)
{
    for(int i=0;i<N;i++)
    {
        int tmp=people[i].init_status < 0 ? 0 : people[i].init_status;
        char* status;
        if(tmp==0)
        {
            status="Infected (0)";
        }
        if(tmp==1)
        {
            status="Susceptible (1)";
        }
        if(tmp>1)
        {
            status="Immune (>1)";
        }
        fprintf(out," ID:%d CoordX:%d CoordY:%d Status:%s Infection Counter:%d\n",
            people[i].id,
            people[i].x,
            people[i].y,
            status,
            infection_counter[people[i].id-1]);

    }   
}

int check_results_people(person* people_serial)
{
    int result=1;
    for(int i=0;i<N;i++)
    {
        if(
            people_serial[i].amp!=people[i].amp || 
            people_serial[i].id!=people[i].id || 
            people_serial[i].init_status!=people[i].init_status || 
            people_serial[i].x!=people[i].x || 
            people_serial[i].y!=people[i].y || 
            people_serial[i].movement!=people[i].movement
        )
            {
                printf("Error ; people differ: %d\n",i);
                result=0;
            }
            
    }
    return result;
}

int check_results_infection(int* infection_counter_serial)
{
    int result=1;
    for(int i=0;i<N;i++)
    {
        if(infection_counter_serial[i]!=infection_counter[i])
        {
            printf("Error ; infections differ : %d\n",i);
            result=0;;
        }
    }
    return result;
}

void check_results(person* people_serial,int** matr_serial,int* infection_counter_serial,int val)
{
    //NOTE: Here you only check for the people and the infections. You don't check for the matrix also since for higher number of threads, the results will most likely differ.
    //At the end, you will have some people that will be at the same position lets say a person with id nr 1, nr 6 and nr 7. In the serial version, the person there will be for example the person
    //with id nr 1 or 6 or 7 while in the parallel version, there can also be those values. The values in the matrix do not account for overlap. Because of this, only the x and y coordinates of the
    //people in the people array will only be accounted for in terms of coordinates.
    if(check_results_people(people_serial) && check_results_infection(infection_counter_serial))
    {
        if(val==1)
            printf("Values in serial and parallel are the same.\n");
        else
        {
            printf("Values in omp1 and omp2 are the same.\n");
        }
    }
}

void initialize(char* total_sim_time,char* file_name,char* nr_threads_str)
{

    FILE* f=fopen(file_name,"r");
    if(!f)
    {
        printf("Error opening file\n");
        exit(1);
    }
    fscanf(f,"%d %d",&rows,&cols);
    
    char line[FILE_BUFFER_SIZE];
    fgets(line,FILE_BUFFER_SIZE,f);
    fscanf(f,"%d",&N);

    printf("Welcome. Select your preffered mode:\n 0 -> Normal mode (evolution of each person not printed)\n 1 -> Debug mode: (evolution of each person printed.)\n");

    scanf("%d",&choice);

    // printf("%d\n",choice);

    //allocate dynamically both people array and matrix for bigger values.
    people=(person*)malloc(sizeof(person) * N);
    if(!people)
    {
        printf("Error at mem alloc\n");
        return;
    }

    
    int cnt=0;
    //N by N matrix ; values in the matrix will represent the id's of the people
    matr=(int**)malloc(sizeof(int*)*rows);
    if(!matr)
    {
        printf("error allocating memory for matrix 2\n");
        return;
    }
    for(int i=0;i<rows;i++)
    {
        matr[i]=(int*)malloc(sizeof(int) * cols);
        if(!matr[i])
        {
            printf("Error; ran out of memory allocating matrix 1\n");
            return;
        }
    }

    init_matrix(matr);
    
    fgets(line,FILE_BUFFER_SIZE,f);
    while(fgets(line,FILE_BUFFER_SIZE,f))
    {
        sscanf(line,"%d %d %d %d %d %d",
            &people[cnt].id,
            &people[cnt].x,
            &people[cnt].y,
            &people[cnt].init_status,
            &people[cnt].movement,
            &people[cnt].amp);
        
        matr[people[cnt].x][people[cnt].y]=people[cnt].id;
        cnt++;
    }
    // printf("Initial matrix:\n");
    // print_matrix(matr);

    int** matr_reset=(int**)malloc(sizeof(int*)*rows);
    if(!matr_reset)
    {
        printf("error allocating memory for matrix 2\n");
        return;
    }
    for(int i=0;i<rows;i++)
    {
        matr_reset[i]=(int*)malloc(sizeof(int) * cols);
        if(!matr_reset[i])
        {
            printf("Error; ran out of memory allocating matrix 1\n");
            return;
        }
    }

    person* people_reset=(person*)malloc(sizeof(person) * N);
    if(!people_reset)
    {
        printf("Error at mem alloc\n");
        return;
    }

    copy_people(people_reset,people);
    copy_matrix(matr_reset,matr);

    infection_counter=(int*)malloc(sizeof(int)*N);
    if(!infection_counter)
    {
        printf("Error allocating memory for infection counter\n");
        return;
    }
    for(int i=0;i<N;i++)
    {
        
        infection_counter[i]=0;
    }

    // print_people(people,N,infection_counter);
    // printf("\n");
    
    sim_time=atoi(total_sim_time);
    nr_threads=atoi(nr_threads_str);

    int* infection_counter_serial=(int*)malloc(sizeof(int)*N);
    if(!infection_counter_serial)
    {
        printf("Error allocating memory\n");
    }
    int** matr_serial=(int**)malloc(sizeof(int*)*rows);
    for(int i=0;i<rows;i++)
    {
        matr_serial[i]=(int*)malloc(sizeof(int) * cols);
        if(!matr[i])
        {
            printf("Error; ran out of memory allocating matrix 1\n");
            return;
        }
    }
    person* people_serial=(person*)malloc(sizeof(person)*N);

    //_serial_out.txt -> 12 characters (leave 1 for '/0')
    //_parallel_out.txt -> 14 characters (leave 1 for '/0')
    char* path_serial=(char*)malloc(sizeof(char) * (strlen(file_name)+12));
    char* path_parallel=(char*)malloc(sizeof(char)* (strlen(file_name)+14));

    strcpy(path_serial,file_name);
    path_serial=strtok(path_serial,".txt");
    
    strcpy(path_parallel,file_name);
    path_parallel=strtok(path_parallel,".txt");
    strcat(path_serial,"_serial_out.txt");
    strcat(path_parallel,"_parallel_out.txt");

    char* results_file=(char*)malloc(sizeof(char)*(strlen(file_name)+9));
    strcpy(results_file,file_name);
    results_file=strtok(results_file,".txt");
    strcat(results_file,"_results.txt");

    char* results_file_omp=(char*)malloc(sizeof(char)*(strlen(file_name)+13));
    strcpy(results_file_omp,file_name);
    results_file_omp=strtok(results_file_omp,".txt");
    strcat(results_file_omp,"_results_omp.txt");
    
    char* path_pragma1=(char*)malloc(sizeof(char) * (strlen(file_name)+10));
    // f_omp1_out.txt
    strcpy(path_pragma1,file_name);
    path_pragma1=strtok(path_pragma1,".txt");
    strcat(path_pragma1,"_omp1_out.txt");

    char* path_pragma2=(char*)malloc(sizeof(char) * (strlen(file_name)+10));
    // f_omp2_out.txt
    strcpy(path_pragma2,file_name);
    path_pragma2=strtok(path_pragma2,".txt");
    strcat(path_pragma2,"_omp2_out.txt");



    // printf("\n\n\n\nFILE PATH\n%s %s\n",path_serial,path_parallel);

    f2 = fopen(path_serial,"w");
    f3 = fopen(path_parallel,"w");
    FILE* f4=fopen(results_file,"w");
    f_pragma_v1=fopen(path_pragma1,"w");
    f_pragma_v2=fopen(path_pragma2,"w");
    FILE* f_pragma_results=fopen(results_file_omp,"w");

    fprintf(f4,"nr simulations:%d\nnr threads:%d\n",sim_time,nr_threads);
    fprintf(f_pragma_results,"nr simulations:%d\nnr threads:%d\n",sim_time,nr_threads);
    //serial
    //start:
    clock_gettime(CLOCK_MONOTONIC,&start);
    start_simulation_serial(sim_time,N,infection_counter,people,matr);
    clock_gettime(CLOCK_MONOTONIC,&finish);

    elapsed=(finish.tv_sec-start.tv_sec);
    elapsed+=(finish.tv_nsec-start.tv_nsec)/pow(10,9);
    Tserial=elapsed-elapsed_output;
    fprintf(f4,"SERIAL:%lf\n",Tserial);

    //store the values for matr,infection counter and people somewhere and reset the values to work on the parallel version.
    
        copy_vector(infection_counter_serial,infection_counter);
        
        copy_matrix(matr_serial,matr);
        
        copy_people(people_serial,people);
        
    //reset the values in matr,people and infection_counter

    copy_people(people,people_reset);
    copy_matrix(matr,matr_reset);

    for(int i=0;i<N;i++)
    {
        infection_counter[i]=0;
    }

    

    //can finally start simulation for parallel.

    clock_gettime(CLOCK_MONOTONIC,&start);
    start_simulation_parallel(sim_time,N,infection_counter,people,matr);
    clock_gettime(CLOCK_MONOTONIC,&finish);

    elapsed=(finish.tv_sec-start.tv_sec);
    elapsed+=(finish.tv_nsec-start.tv_nsec)/pow(10,9);
    Tparallel=elapsed-elapsed_output;
    fprintf(f4,"PARALLEL:%lf\n",Tparallel);

    double efficiency1=Tserial/(nr_threads*Tparallel);
    double speedup1=efficiency1*nr_threads;

    fprintf(f4,"Efficiency: %lf\nSpeedup: %lf\n",efficiency1,speedup1);

    check_results(people_serial,matr_serial,infection_counter_serial,1);

    //reset values for omp v1
    copy_people(people,people_reset);
    copy_matrix(matr,matr_reset);

    for(int i=0;i<N;i++)
    {
        infection_counter[i]=0;
    }

    clock_gettime(CLOCK_MONOTONIC,&start);
    omp_v1(sim_time,N,infection_counter,people,matr);
    clock_gettime(CLOCK_MONOTONIC,&finish);

    elapsed=(finish.tv_sec-start.tv_sec);
    elapsed+=(finish.tv_nsec-start.tv_nsec)/pow(10,9);
    T_omp_v1=elapsed-elapsed_output;

    copy_vector(infection_counter_serial,infection_counter);    
    copy_matrix(matr_serial,matr);
    copy_people(people_serial,people);

    //reset values for omp v2
    copy_people(people,people_reset);
    copy_matrix(matr,matr_reset);

    for(int i=0;i<N;i++)
    {
        infection_counter[i]=0;
    }

    clock_gettime(CLOCK_MONOTONIC,&start);
    omp_v2(sim_time,N,infection_counter,people,matr);
    clock_gettime(CLOCK_MONOTONIC,&finish);

    elapsed=(finish.tv_sec-start.tv_sec);
    elapsed+=(finish.tv_nsec-start.tv_nsec)/pow(10,9);
    T_omp_v2=elapsed-elapsed_output;

    efficiency1=Tserial/(nr_threads*T_omp_v1);
    speedup1=efficiency1*nr_threads;

    fprintf(f_pragma_results,"\nSERIAL:%lf\nOMP V1:%lf\nEFFICIENCY:%lf\nSPEEDUP:%lf\n",Tserial,T_omp_v1,efficiency1,speedup1);

    efficiency1=Tserial/(nr_threads*T_omp_v2);
    speedup1=efficiency1*nr_threads;

    fprintf(f_pragma_results,"\nSERIAL:%lf\nOMP V2:%lf\nEFFICIENCY:%lf\nSPEEDUP:%lf\n",Tserial,T_omp_v2,efficiency1,speedup1);

    for(int i=0;i<rows;i++)
    {
        free(matr[i]);
    }

    check_results(people_serial,matr_serial,infection_counter_serial,2);

    free(matr);
    free(people);
    free(infection_counter);
}

void print_people(person* people,int N,int* infection_counter)
{
    for(int i=0;i<N;i++)
    {
        int tmp=people[i].init_status < 0 ? 0 : people[i].init_status;
        // if(tmp>1)
        // {
        //     tmp=1;
        // }
        printf("Person nr %d: ID:%d CoordX:%d CoordY:%d Status:%d Movement:%d Amp:%d Infection Counter:%d\n",
            i+1,
            people[i].id,
            people[i].x,
            people[i].y,
            tmp,
            people[i].movement,
            people[i].amp,
            infection_counter[people[i].id-1]);

    }   
}