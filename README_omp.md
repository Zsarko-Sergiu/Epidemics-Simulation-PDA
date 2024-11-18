The serial algorithm is almost the same as in the first project. Only difference is that now update infections changes the states of a singular person
instead of every person soo in the serial algorithm, you need to call it now with a for on the entire array of person. (Previously,update_infections was called
only once in the serial algorithm and in its content, every person was updated. Now, update_infections is called N times where N is the nr of people.).

This change was made soo that omp_v1 will be easier to understand and less code will be written for it.

Besides this change, the serial algorithm,reading from file and output is basically the same. As in the previous assignment, an output file is created
for both omp v1 and omp v2 as well as a comparision between them is printed on the screen whether the content of the files omp_v1 and omp_v2 are the same or not.
The results for speedup and efficiency are saved in a file f_results_omp.txt (where f.txt is the name of the input file.)

OMP V1:
    Parallelize serial algorithm using PARALLEL FOR'S from omp library. Add a parallel for before each of the for's that takes a person from the array and moves,checks and updates them. Since the time it takes to move,check and update each person is done mostly uniformly (for a person p1 and p2, the time it takes to perform those operations on them is roughly the same), a STATIC SCHEDULE will work best. There will be some example outputs bellow where dynamic and static will be compared for static and dynamic. Critical sections are also added where we write the variables to file and when we decrease the simulation time. This is because we don't want racing to potentially occur and a thread to write his variables to the file or decrease sim time before
    another thread finishes his work. We then write the final output values to the file, without measuring the time it took to write them.
OMP V2:
    Parallelize serial algorithm using TASKS from omp library. This is more similar to the first parallel version. Here we have tasks for each of move, check and update and between these tasks we have the equivelant to a barrier in open mp (taskwait). This will make it so that each thread will move their respective values first and then wait untill all the other threads move their respective values. The same goes for both check and update. As in OMP V1, we have a critical section for both writing to file and decreasing simulation time. The same explanation is given as above. Writing final values also occurs as above.

EXAMPLE OUTPUT:
    Dynamic schedule:
        Chunk Size=1000:
            epidemics10K:
            nr simulations:10
            nr threads:1000

            SERIAL:4.299806
            OMP V1:0.955585
            EFFICIENCY:0.004500
            SPEEDUP:4.499656

            SERIAL:4.299806
            OMP V2:1.706477
            EFFICIENCY:0.002520
            SPEEDUP:2.519697

        Chunk Size=100:
            epidemics10K:
            nr simulations:10
            nr threads:1000

            SERIAL:4.126613
            OMP V1:0.862460
            EFFICIENCY:0.004785
            SPEEDUP:4.784701

            SERIAL:4.126613
            OMP V2:1.745695
            EFFICIENCY:0.002364
            SPEEDUP:2.363879
        
        No Chunk Size:
            epidemics10K:
            nr simulations:10
            nr threads:1000

            SERIAL:4.424017
            OMP V1:0.848489
            EFFICIENCY:0.005214
            SPEEDUP:5.213995

            SERIAL:4.424017
            OMP V2:1.696428
            EFFICIENCY:0.002608
            SPEEDUP:2.607841

    Static schedule:
        No Chunk Size:
            nr simulations:10
            nr threads:1000

            SERIAL:4.151953
            OMP V1:0.902622
            EFFICIENCY:0.004600
            SPEEDUP:4.599879

            SERIAL:4.151953
            OMP V2:1.877129
            EFFICIENCY:0.002212
            SPEEDUP:2.211863
        
        Chunk size=100:
            nr simulations:10
            nr threads:1000

            SERIAL:3.884759
            OMP V1:0.861746
            EFFICIENCY:0.004508
            SPEEDUP:4.508010

            SERIAL:3.884759
            OMP V2:1.677180
            EFFICIENCY:0.002316
            SPEEDUP:2.316244

        Chunk Size=1000:
            nr simulations:10
            nr threads:1000

            SERIAL:3.830980
            OMP V1:1.049087
            EFFICIENCY:0.003652
            SPEEDUP:3.651728
            
            SERIAL:3.830980
            OMP V2:1.666937
            EFFICIENCY:0.002298
            SPEEDUP:2.298215

Overall, both OMP V1 and V2 offer a speedup over their serial version. OMP v1 seems to beat V2 in all speedup cases. Modifying the chunk size
has an effect on the overall speedup. It seems that a higher value for chunk size will lead to a less significant speedup, and in both the static and
dynamic cases, the best value for chunk size is the one where there is no value. 

Modifying the nr of threads for epidemics10K from 1000 to 100 we get the following values:

Dynamic:
    Chunk Size=1000:
        nr simulations:10
        nr threads:100

        SERIAL:4.168374
        OMP V1:0.763616
        EFFICIENCY:0.054587
        SPEEDUP:5.458730

        SERIAL:4.168374
        OMP V2:1.373255
        EFFICIENCY:0.030354
        SPEEDUP:3.035397

    Chunk Size=100:
        nr simulations:10
        nr threads:100

        SERIAL:3.957747
        OMP V1:0.690376
        EFFICIENCY:0.057327
        SPEEDUP:5.732739

        SERIAL:3.957747
        OMP V2:1.410401
        EFFICIENCY:0.028061
        SPEEDUP:2.806114

    No Chunk Size:
        nr simulations:10
        nr threads:100

        SERIAL:4.252916
        OMP V1:0.612113
        EFFICIENCY:0.069479
        SPEEDUP:6.947926

        SERIAL:4.252916
        OMP V2:1.370638
        EFFICIENCY:0.031029
        SPEEDUP:3.102873

Static:
    No Chunk Size:
        nr simulations:10
        nr threads:100

        SERIAL:4.111816
        OMP V1:0.636478
        EFFICIENCY:0.064603
        SPEEDUP:6.460262

        SERIAL:4.111816
        OMP V2:1.385718
        EFFICIENCY:0.029673
        SPEEDUP:2.967281

    Chunk Size=100:
        nr simulations:10
        nr threads:100

        SERIAL:4.496954
        OMP V1:0.756528
        EFFICIENCY:0.059442
        SPEEDUP:5.944199

        SERIAL:4.496954
        OMP V2:1.358232
        EFFICIENCY:0.033109
        SPEEDUP:3.310888
    
    Chunk Size=1000:
        nr simulations:10
        nr threads:100

        SERIAL:4.199243
        OMP V1:0.800539
        EFFICIENCY:0.052455
        SPEEDUP:5.245517

        SERIAL:4.199243
        OMP V2:1.390054
        EFFICIENCY:0.030209
        SPEEDUP:3.020922



RESULTS:
-Performance is better for 100 threads ; general performance for both static and dynamic is the best when the chunk size is not given. Performance for both omp versions decrease as the number
of chunk Size increases.


