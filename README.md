Main structures used:
    person:
        -id : person id
        -x,y -> coordinates in matrix
        -init_status -> 0/1 infected/susceptible
        -movement -> 3 left, 2 right, 1 down, 0 up
        -amp -> how large the jump of the person is when they move

    person_thread:
        -id : person_thread id
        -first : the id of first person that the thread will take
        -last : id of the last person that the thread will take
    
    

    people:
        -array of structures of type person ; size = N = nr of people
    
    people_threads:
        -array of structures of type person_thread : size = nr_threads

    matr:
        -matrix that stores the location of the people

    infection counter:
        -numbers how many times a person in the people array has been infected during the simulation
    
    rows:
        -nr of rows in matrix
    
    cols:
        -nr of cols in matrix
-------------------------------------------------------------------------------------------------------------------------------------------------------
Input output:
    Input:
        -simulation time and nr of threads is read from command line as well as the file.
        -the file contains on the first line the max x and y coordinates (the rows and cols values) of the matrix matr
        -the second line contains nr of people in the file (the size of person array of structures) as well as the size of the infection counter
        -an option to choose debug value after running the program (1 or 0)

    Output:
        -The x,y coordinaters of each person, their status and how many times they have been infected along the simulation
        -These values are saved in seperate files each for the version of implementation (serial or parallel)
        -Reset the values back to the initial values after performing serial version
        -After reseting the values, start performing parallel version.
        -If the name of the file was f.txt then the name of output files will be f_serial_out.txt ; f_parallel_out.txt
        -If debug was chose as 1, evolution of each person across the simulation will be printed. If debug was 0, only the final result of each person 
        will be printed.
        -Besides these two files, an additional output file will be created that will save the time it took to execute the two versions as well as speedup and
        efficiency, nr of simulations and nr of threads
        -If name of file was f.txt, result file will be named f_results.txt
        -In the terminal, you will also have displayed if the final result of each people ,from the two versions of the program, is the same
        
------------------------------------------------------------------------------------------------------------------------------------------------------

Algorithm:
    -at every simulation:
        -move each of the person from people array.
        -check for infections for each of them
        -update infections for each of them.

    MOVE:
        -takes a person and the matrix matr
        -gets the direction of that person
        -left (3) : x stays the same, y is modified ;  move left by the persons amplitude ; if we got a value that will move the person out of the border(out of the matrix ; new value<0), set that persons position to left most column in matrix(col==0) and reset its movement to right(2) so it begins moving right at the next simulation
        -right (2) : same logic as left, this time border is at cols (last legal value cols-1). If value higher or equal to border (<=0), set its col to cols-1
        and reset its movement to left so it begins moving left next simulation. Take out of border value as <=0 because even if its still considered legal move when = 0, at next simulation step you will still need to reset that persons movement.
        -down (1) : y stays the same, x is modified ; move down by person amplitude ; if value out of border (>=rows) , set x coordinate to rows, reset movement to up. person begins moving up at next simulation step
        -up (0) : same as down ; value out of border <=0 ; set x coordinate to 0 , reset movement to down
        
    CHECK:
        -takes in a person p1 , length of people array and people array itself
        -if there is another person at p1's location (same x,y ; different ids)
        -if either of them are immune skip checking for infections
        -if there is a person p2 that is at p1's location and p2 is infected (status<=0) and the p1 is susceptible (status=1) , then p1 also becomes infected
        -if p1 is infected and p2 isn't yet, then p2 becomes infected.
    
    UPDATE:
        -takes in people array, size of it and the infection counter.
        -iterates through people array and gets each person.
        -if the person's status is 0(infected simple), then increase value of infection counter at that persons id.
        -if that person is infected and its value in status is bigger than the INFECTED_DURATION, decrease its init status
        -if the person's status is bigger than 1, decrease its status.
        -if the value in status is equal to the infection duration, person becomes immune for IMMUNE_DURATION+1 time
            -when infected, value in status will keep decreasing untill its equal to infected duration. Say we have an infected duration of 2 and immune of 3:
            -status=0
            -sim 1:  0 > -2 => status = -1
            -sim 2:  -1 > -2 => status = -2
            -sim 3: -2 == -2 => status = 1+3 = 4
            -sim 4: 4 > 1 => status = 3
            -sim 5: 3 > 1 => status =2
            -sim 6: 2 > 1 => status =1
            -status = 1 => person becomes susceptible again.
        -important things to note here:
            -we are passing in the function the entire array people and directly modifying the entire array in one step, in the check and move, we pass a person and do computations on them while here we do it on each individual. 
            -the person will become immune for 1+IMMUNE_DURATION sim times because at the end, the person becomes susceptible not infected again.
            -for the case where the person is susceptible, the update function will not do anything on them.
-----------------------------------------------------------------------------------------------------------------------------------------------------------

Serial implementation:
    -making use of the functions above, the serial implementation is pretty simple.
    -while our sim time is > 0
        -take each person in people array, move each person.
        -check infections for each person in people array
        -update people array (again, we update the whole arrray of people directly, we don't update each individual)
        -if debug was 1, then here we print the values in the f_serial_out.txt output file
    -after finishing simulation, if debug was 0, write only the final result in the f_serial_out.txt file.

-------------------------------------------------------------------------------------------------------------------------------------------------------------

Parallel implementation:

    -nothing changes in the logic from the serial implementation.
    -allocate memory for threads ; allocate memory for people threads.
    -we parallelize the algorithm by dividing the number of people across threads given as command line arg values
    -first, if we have more threads than nr of people, then the function will exit and print an error.
    -we then divide the number of people by the number of threads, obtaining value in threads people.
    -since we aren't sure if the division will not give a rest, we also save the rest in a value.
    -initialize a tmp value to 0.
    -for each of the threads, set its id and set its first value to tmp.
    -get the number of people per thread = value in threads people + 1 if the id of the thread is smaller than the rest and 0 if its bigger.
    -this basically assures that the first threads will take a larger number of people than the later threads.
    -assign last value in thread to tmp + people per thread - 1
    -after doing this, create the thread and simulate for that value.
    -after that, remodify the value in tmp to tmp+people per thread. This assures that the next thread will have the first value as the next value from last thread (first_thread1 = last_thread2+1)
    -after simulating for each thread, join them to get the result.
    -if debug was 0, write only the final result in f_parallel_out.txt.
    -destroy barriers to free the memory.

    PARALLEL SIMULATION EXPLAINED:
        -we have three barriers, one for each fo the functions move check and update.
        -sim time will not be shared. each thread will have its own sim time (sim_time_threads)
        -while sim_time_threads > 0
            -each thread moves the persons with the ids from start to last (start = 3 ;last = 6 => thread moves person with ids 3,4,5,6)
            -each thread waits at a barrier so that the other threads move their respective people.
            -after each thread moved their people, move on to check.
            -check works exactly the same as move.
            -after each thread checked their people, passing the check barrier, move to update
            -this is a little bit more interesting since because the update function takes in the entire people array and updates the entire array in one iteration, if we let each thread update the entire array, this will produce bad results.
            -we instead only pick one thread to update the statuses of the people (in this program its the main thread with id=0, but it can reallistically be
            any thread you want ; doesn't make a difference). After that specific thread will finish updating the values, we move over the update barrier.
            -after updating the values, if we are in the main thread and debug=1, we write the values in the output file. we only do this for one thread again because if we don't restrict it to one thread, each thread will write its own output to the file. 
            -decrement sim_time_threads
        
---------------------------------------------------------------------------------------------------------------------------------------------------------------

Discussion regarding speedup,efficiency:
    
    Implementation:
        -start measuring time elapsed from beginning of execution of algorithm, both serial and parallel. 
        -in the algorithm, when writing the final values in the output files, another clock will begin that will get the time it took to write those values.
        -the final values for Tserial and Tparallel will be the time it took to execute their respective algorithm - the time it took to write the final values to the output file.
        -efficiency is computed using formula : efficiency=Tserial/(nr_threads*Tparallel)
        -speedup is computed using formula : speedup=efficiency*nr_threads;
        -as mentioned in the input/output part, these values are not printed on the screen, they are written in a specific file create for the program.
    
    Discussion:
        -for the discussion ill take into consideration two input files : epidemics10.txt and epidemics10K.txt (10 nr of people, 10K nr of people)
            
            -for epidemics10.txt:
                -nr simulations:5 ; nr threads:10 ; SERIAL:0.000011 ; PARALLEL:0.002196 ; Efficiency: 0.000498 ; Speedup: 0.004976
                -nr simulations:5 ; nr threads:2 ; SERIAL:0.000016 ; PARALLEL:0.000811 ; Efficiency: 0.010489 ; Speedup: 0.020977
                -nr simulations:5 ; nr threads:5 ; SERIAL:0.000015 ; PARALLEL:0.001497 ; Efficiency: 0.002058 ; Speedup: 0.010291
                -nr simulations:5 ; nr threads:7 ; SERIAL:0.000011 ; PARALLEL:0.002244 ; Efficiency: 0.000783 ; Speedup: 0.005484

                -overall, serial version performs better than parallel no matter how many threads are given.

            -for epidemics10K.txt:
                -nr simulations:5 ; nr threads:2 ; SERIAL:2.206294 ; PARALLEL:1.241106 ; Efficiency: 0.910046 ; Speedup: 1.820093
                -nr simulations:5 ; nr threads:5 ; SERIAL:2.267069 ; PARALLEL:0.550330 ; Efficiency: 0.875572 ; Speedup: 4.377860
                -nr simulations:5 ; nr threads:10 ; SERIAL:2.306438 ; PARALLEL:0.462241 ; Efficiency: 0.538002 ; Speedup: 5.380018
                -nr simulations:5 ; nr threads:100 ; SERIAL:2.310847 ; PARALLEL:0.391671 ; Efficiency: 0.064634 ; Speedup: 6.463381
                -nr simulations:5 ; nr threads:250 ; SERIAL:2.296568 ; PARALLEL:0.416665 ; Efficiency: 0.023940 ; Speedup: 5.985115
                -nr simulations:5 ; nr threads:1000 ; SERIAL:2.212474 ; PARALLEL:0.695585 ; Efficiency: 0.003339 ; Speedup: 3.339127
                -nr simulations:5 ; nr threads:5000 ;SERIAL:2.280444 ;PARALLEL:5.367058 ; Efficiency: 0.000086 ; Speedup: 0.427775
                
                -overall, the parallel version performs better than the serial version. The cases where the serial version performs better seem to be cases where the thread number is way too high which leads to overhead. 
                -Too low of a number of threads will lead to a smaller speedup. Too high of a nr of threads also does that, and may even lead to the serial version being better than the parallel one.
                -a medium value will perform the best (haven't tested for the "perfect value" but 100 threads seems to perform the best out of the nr of threads)

            -overall it seems that for a small number of people, the serial version will perform the best and parallel version will never outperform it, while for a large number of people, you need to keep testing and experimenting with the number of threads to find the best value
