#include <stdio.h>
#include <stdlib.h>

#define FILE_BUFFER_SIZE 256
int MAX_X_COORD,MAX_Y_COORD,N;

void initialize(char* total_sim_time,char* file_name,char* nr_threads);
void print_people();

typedef struct person
{
    int id,x,y,init_status,movement,amp;
}person;

int main(int argc,char* argv[])
{
    if(argc < 4)
    {
        printf("Error ; not enough arguments\n");
        exit(1);
    }
    initialize(argv[1],argv[2],argv[3]);
}

void initialize(char* total_sim_time,char* file_name,char* nr_threads)
{
    FILE* f=fopen(file_name,"r");
    if(!f)
    {
        printf("Error opening file\n");
        exit(1);
    }
    fscanf(f,"%d %d",&MAX_X_COORD,&MAX_Y_COORD);
    
    char line[FILE_BUFFER_SIZE];
    fgets(line,FILE_BUFFER_SIZE,f);
    fscanf(f,"%d",&N);

    struct person people[N];
    int cnt=0;

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
        cnt++;
    }
    print_people(people,N);
    
}

void print_people(person* people,int N)
{
    for(int i=0;i<N;i++)
    {
        printf("Person nr %d: ID:%d CoordX:%d CoordY:%d Status:%d Movement:%d Amp:%d\n",
            i+1,
            people[i].id,
            people[i].x,
            people[i].y,
            people[i].init_status,
            people[i].movement,
            people[i].amp);

    }   
}