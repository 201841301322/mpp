#!/bin/sh

# (normal mode)
ir_cut_enable()
{
	# pin_mux
	echo "64" > /sys/class/gpio/unexport;
	echo "65" > /sys/class/gpio/unexport;
	echo "64" > /sys/class/gpio/export;
	echo "65" > /sys/class/gpio/export;
	
	# dir
	echo "out" > /sys/class/gpio/gpio64/direction;
	echo "out" > /sys/class/gpio/gpio65/direction;
	
	# data, GPIO8_0: 0, GPIO8_1: 1  (normal mode)
	echo "0" > /sys/class/gpio/gpio64/value;
	echo "1" > /sys/class/gpio/gpio65/value;
	
	# back to original 
	sleep 1;
	echo "0" > /sys/class/gpio/gpio64/value;
	echo "0" > /sys/class/gpio/gpio65/value;
}
# (ir mode)
ir_cut_disable()
{
	# pin_mux
	echo "64" > /sys/class/gpio/unexport;
	echo "65" > /sys/class/gpio/unexport;
	echo "64" > /sys/class/gpio/export;
	echo "65" > /sys/class/gpio/export;
	
	# dir
	echo "out" > /sys/class/gpio/gpio64/direction;
	echo "out" > /sys/class/gpio/gpio65/direction;
	
	# data, GPIO8_0: 1, GPIO8_1: 0  (ir mode)
	echo "1" > /sys/class/gpio/gpio64/value;
	echo "0" > /sys/class/gpio/gpio65/value;
	
	# back to original 
	sleep 1
	echo "0" > /sys/class/gpio/gpio64/value;
	echo "0" > /sys/class/gpio/gpio65/value;
}


if [ $# -eq 0 ]; then
    echo "normal mode: ./demo_config.sh 0";
    echo "ir mode    : ./demo_config.sh 1";
else
    if [ $1 -eq 0 ]; then
        echo "normal mode, ir_cut on"
        ir_cut_enable > /dev/null;
    fi

    if [ $1 -eq 1 ]; then
        echo "ir mode, ir_cut off"
        ir_cut_disable > /dev/null ;
    fi
fi

