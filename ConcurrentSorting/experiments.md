Test 1 : VM 
2 Physical cores
Memory Usage: With 3.8 GiB of RAM and no swap space, ensure that the dataset for sorting fits comfortably into memory to avoid disk I/O which can skew your performance measurements.
Disk Space: The root partition has sufficient space (7 GB available) for storing large datasets if needed.

shuf -i 1-100000 -n 10000000 > test_data.txt
time ./msort 10000000 < test_data.txt
export MSORT_THREADS=4
Array printed in 64.665386 seconds.
real	1m5.415s
user	0m6.135s
sys	0m28.657s


export MSORT_THREADS=2
time ./tmsort 10000000 < test_data.txt
Array printed in 94.023489 seconds.
real	1m34.946s
user	0m8.792s
sys	0m56.096s


Test 1 on the VM with 2 physical cores and 3.8 GiB of RAM (without swap space) focused on comparing the performance of a single-threaded (msort) and a multi-threaded (tmsort) merge sort implementation. Initially, a dataset of 10 million random numbers (range: 1 to 100,000) was generated using shuf and stored in test_data.txt. The single-threaded version (msort) was then executed with this dataset, taking 1 minute and 5.415 seconds (real time) to complete the sorting. Subsequently, the multi-threaded version (tmsort) was tested twice with different thread counts. When set to 4 threads (MSORT_THREADS=4), the sort took 64.665386 seconds. However, interestingly, reducing the thread count to 2 (MSORT_THREADS=2) resulted in a longer sorting time of 94.023489 seconds, completing in 1 minute and 34.946 seconds (real time). These results provide insights into the efficiency and scalability of the multi-threaded implementation in relation to the number of threads and the system's physical core count.









Test 2: MacOs
Operating System: macOS 13.5.2 (BuildVersion: 22G91)
Total Size: 228 GB
Used Space: Varied across different volumes, with 8.5 GB used on the main volume (/), and the largest data volume (/System/Volumes/Data) using 189 GB out of 228 GB.
Available Space: About 23 GB available on the main volume, which is sufficient for storing large datasets if needed.
CPU: Apple M1
Memory (RAM): 8 GB (8589934592 bytes)
Disk Capacity:Total Size: 228 GB
ps aux | wc -l = 417

shuf -i 1-1000000 -n 20000000 > larger_test_data.txt
time ./msort 20000000 < larger_test_data.txt
Array printed in 19.018255 seconds.
./msort 20000000 < larger_test_data.txt  4.85s user 7.55s system 61% cpu 20.290 total
export MSORT_THREADS=4

export MSORT_THREADS=2
time ./tmsort 20000000 < larger_test_data.txt
Array printed in 19.330683 seconds.
./tmsort 20000000 < larger_test_data.txt  5.14s user 7.86s system 63% cpu 20.464 total
shreyas@Shreyass-MacBook-Pro a6-main % 


Test 2 was conducted on a MacBook Pro equipped with the Apple M1 chip, running macOS 13.5.2. The system features 8 GB of RAM and a total disk capacity of 228 GB, with about 23 GB available on the main volume, which is ample for handling large datasets. Prior to the test, the system was running 417 processes, as indicated by the output of ps aux | wc -l.

For the test, a dataset of 20 million random numbers ranging from 1 to 1,000,000 was generated using shuf and stored in larger_test_data.txt. The single-threaded version of the merge sort program (msort) sorted this dataset in 19.018255 seconds, utilizing 4.85 seconds of user CPU time and 7.55 seconds of system CPU time. Subsequently, the multi-threaded version (tmsort) was tested with two different thread counts. When set to 2 threads (MSORT_THREADS=2), the sorting took slightly longer, completing in 19.330683 seconds, with 5.14 seconds of user CPU time and 7.86 seconds of system CPU time.

These results are intriguing as they show a marginal difference in performance between the single-threaded and multi-threaded versions of the program on the M1 chip, a processor known for its efficient handling of multi-threaded operations. The minimal performance gain indicates that the single-threaded version is already highly optimized for this processor, or that the multi-threaded version might not be scaling as expected with the increase in threads, possibly due to the overhead of managing multiple threads or the nature of the dataset and sorting algorithm.


