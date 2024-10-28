#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FILE_BUFFER_SIZE 256
int MAX_X_COORD,MAX_Y_COORD,N;

typedef struct person
{
    int id,x,y,init_status,movement,amp;
}person;

void initialize(char* total_sim_time,char* file_name,char* nr_threads);
void print_people();
void print_matrix();
void start_simulation_serial(int sim_time,int n,int infection_counter[n],struct person people[n],int matr[][n]);
void move_person(person p,int n,int matr[][n]);


int main(int argc,char* argv[])
{
    if(argc < 4)
    {
        printf("Error ; not enough arguments\n");
        exit(1);
    }
    initialize(argv[1],argv[2],argv[3]);
}

void print_matrix(int n,int matr[][n])
{
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            printf("%d ",matr[i][j]);
        }
        printf("\n");
    }
}

void init_matrix(int n,int matr[][n])
{
    for (int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            matr[i][j]=0;
        }
    }
}

void move_person(person p,int n,int matr[][n])
{
    int direction=p.movement;
    //left
    if(direction==3)
    {
        //y decrease, x stays same
        int new_y=p.y-p.amp;
        if(new_y<0)
        {
            new_y=abs(new_y);
            p.movement=2;
        }
        matr[p.x][p.y]=0;
        matr[p.x][new_y]=p.id;
        p.y=new_y;
    }
    //right
    if(direction==2)
    {
        //y decrease, x stays same
        int new_y=p.y+p.amp;
        if(new_y>=10)
        {
            new_y=n-(new_y-n);
            p.movement=3;
        }
        matr[p.x][p.y]=0;
        matr[p.x][new_y]=p.id;
        p.y=new_y;
    }
    //down
    if(direction==1)
    {
        //x decrease, y stays same
        int new_x=p.x-p.amp;
        if(new_x<0)
        {
            new_x=abs(new_x);
            p.movement=0;
        }
        matr[p.x][p.y]=0;
        printf("%d %d\n",new_x,p.id);
        matr[new_x][p.y]=p.id;
        p.x=new_x;
    }
    //up
    if(direction==0)
    {
        //x decrease, y stays same
        int new_x=p.x+p.amp;
        if(new_x>=10)
        {
            new_x=n-(new_x-n);
            p.movement=1;
        }
        matr[p.x][p.y]=0;
        matr[new_x][p.y]=p.id;
        p.x=new_x;
    }
}

void start_simulation_serial(int sim_time,int n,int infection_counter[n],person people[n],int matr[][n])
{
    while(sim_time>0)
    {
        for(int i=0;i<n;i++)
        {
            person p=people[i];
            move_person(p,n,matr);
        }
        printf("Simulation time %d\n",sim_time);
        print_matrix(n,matr);
        printf("\n");
        sim_time--;

    }
    printf("finished simulation\n");
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
    //N by N matrix ; values in the matrix will represent the id's of the people
    int matr[N][N];

    init_matrix(N,matr);

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
    print_matrix(N,matr);
    print_people(people,N);
    int infection_counter[N];
    for(int i=0;i<N;i++)
    {
        infection_counter[i]=0;
    }
    
    int total_sim_time_conv=atoi(total_sim_time);
    start_simulation_serial(total_sim_time_conv,N,infection_counter,people,matr);
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