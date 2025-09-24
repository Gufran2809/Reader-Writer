README FILE 


you need to provide input file(inp-params.txt) along with the code
input file consisting of parameters which are - nw: the number of writer threads, nr: the number of reader
threads, kw: the number of times each writer thread tries to enter the CS, kr: the number of
times each reader thread tries to enter the CS, μCS, μRem.

There are two codes one for each algorithm.
To run first-code
First command  -> g++ rw-cs22btech11014.cpp
Second command -> ./a.out

The program will output the following in an 2 output files
(i)Displays the log of all the events as shown for each of the algorithms in output file: RW-log.txt.
(ii)AverageTimeRW.txt, consisting of the average time a thread takes to gain entry to the Critical
Section and worst case time (both reader and writer threads)

To run second-code
First command  -> g++ frw-cs22btech11014.cpp
Second command -> ./a.out

The program will output the following in an 2 output files
(i)Displays the log of all the events as shown for each of the algorithms in output file: FairRW-log.txt.
(ii)AverageTimeFairRW.txt, consisting of the average time a thread takes to gain entry to the Critical
Section and worst case time (both reader and writer threads)

