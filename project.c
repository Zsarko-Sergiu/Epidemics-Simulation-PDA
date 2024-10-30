#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FILE_BUFFER_SIZE 256

typedef struct person
{
    int id,x,y,init_status,movement,amp;
}person;

int rows,cols,N;

#define INFECTED_DURATION 5
#define IMMUNE_DURATION 2

void initialize(char* total_sim_time,char* file_name,char* nr_threads);
void start_simulation_serial(int sim_time,int n,int* infection_counter,person* people,int** matr);
void move_person(person* p,int** matr);
void print_people(person* people,int n,int* infection_counter);

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

void update_infections(person* people, int n,int* infection_counter) {
    for (int i = 0; i < n; i++) 
    {
        if(people[i].init_status==0)
        {
            infection_counter[people[i].id-1]++;
        }
        //infected. check if its value is also bigger than the infected duration
        if(people[i].init_status<=0 && people[i].init_status>-INFECTED_DURATION)
        {
            people[i].init_status--;
        }
            
        else if(people[i].init_status==-INFECTED_DURATION)
        {
            //finished being infected; make person immune now.
            //1 is hardcoded here because we have two base cases. Either "0" as infected or "1" as susceptible. When we reach "1" we know that the person has finished being immune and is now susceptible again
            people[i].init_status=1+IMMUNE_DURATION;
        }
        else if(people[i].init_status>1)
        {
            people[i].init_status--;
        }
        //if people[i] didn't go through any of the ifs, that means that the person is susceptible (1) value ; nothing needs to be changed in this case.
        

    }
}



void start_simulation_serial(int sim_time,int n,int* infection_counter,person* people,int** matr)
{
    while(sim_time>0)
    {
        printf("Simulation nr %d\n",sim_time);
        for(int i=0;i<n;i++)
        {
            move_person(&people[i],matr);
            check_for_infections(people,n,&people[i]);
        }
        update_infections(people,n,infection_counter);
        print_matrix(matr);
        print_people(people,n,infection_counter);
        sim_time--;
    }
    printf("\nFinished simulation\n");
    print_matrix(matr);
    print_people(people,n,infection_counter);
}

void initialize(char* total_sim_time,char* file_name,char* nr_threads)
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

    //allocate dynamically both people array and matrix for bigger values.
    person* people=(person*)malloc(sizeof(person) * N);
    if(!people)
    {
        printf("Error at mem alloc\n");
        return;
    }

    
    int cnt=0;
    //N by N matrix ; values in the matrix will represent the id's of the people
    int** matr=(int**)malloc(sizeof(int*)*rows);
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
    printf("Initial matrix:\n");
    print_matrix(matr);

    int* infection_counter=(int*)malloc(sizeof(int)*N);
    if(!infection_counter)
    {
        printf("Error allocating memory for infection counter\n");
        return;
    }
    for(int i=0;i<N;i++)
    {
        
        infection_counter[i]=0;
    }

    print_people(people,N,infection_counter);
    printf("\n");
    
    
    int total_sim_time_conv=atoi(total_sim_time);
    start_simulation_serial(total_sim_time_conv,N,infection_counter,people,matr);
    for(int i=0;i<rows;i++)
    {
        free(matr[i]);
    }
    free(matr);
    free(people);
    free(infection_counter);
}

void print_people(person* people,int N,int* infection_counter)
{
    for(int i=0;i<N;i++)
    {
        int tmp=people[i].init_status < 0 ? 0 : people[i].init_status;
        if(tmp>1)
        {
            tmp=1;
        }
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