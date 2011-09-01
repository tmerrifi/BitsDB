
#kill that semaphore
sudo echo > /sys/kernel/debug/tracing/trace;
sudo rm /dev/shm/*sem_snapshot_sys_call_test*;
#sudo rm snapshot_test_shm;

echo function_graph | sudo tee /sys/kernel/debug/tracing/current_tracer
echo "" | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
#echo sys_msync | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
#echo do_wp_page | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter


./tester $1 $2 $3 $4 $5 $6

cat /sys/kernel/debug/tracing/trace > out;
tail -200 /var/log/syslog | grep -v radix > syslog_out
