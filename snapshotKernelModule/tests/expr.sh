
echo function_graph | sudo tee /sys/kernel/debug/tracing/current_tracer
#echo "" | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
echo sys_msync | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
#echo walk_page_range | sudo tee -a /sys/kernel/debug/tracing/set_ftrace_filter

sudo ./master "1" & 
sudo ./slave "1";





#sudo ./master $1
sleep 10;
tail -220 /sys/kernel/debug/tracing/trace > out;
tail -200 /var/log/syslog | grep -v radix > syslog_out