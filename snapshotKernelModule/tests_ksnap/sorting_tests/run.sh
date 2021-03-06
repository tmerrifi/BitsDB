
#install the library and module
#install_snap_lib.sh > /dev/null

#erase the trace
echo " " | sudo tee /sys/kernel/debug/tracing/trace;
#turn of the trace
echo "0" | sudo tee /sys/kernel/debug/tracing/tracing_on;
#select the type of trace
echo function_graph | sudo tee /sys/kernel/debug/tracing/current_tracer
#echo "" | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
#use sys_read becase (a) its always available and (b) doesn't usually get called during snapshot stuff
echo sys_read | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter

#kill the semaphores and the memory mapped files
sudo rm /dev/shm/*.mem /dev/shm/*.sem *.mem snapshot_* sort_test *memcpy_shm* *.sem *.txt
#sudo rm *.mem *.sem

make clean;
make;

./tester -t$1 -o$2 -d$3 -s$4 -S$5 -n$6 -c$7 -b$8;
sleep 2;
echo "print it out"
sudo cat /sys/kernel/debug/tracing/trace > out