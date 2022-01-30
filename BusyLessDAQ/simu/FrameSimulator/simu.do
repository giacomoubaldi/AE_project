vsim  -t  1ns  work.tb
add wave -divider "Main Signals"
add wave -hexadecimal /tb/*


add wave -divider "Frame Simulator"
add wave -hexadecimal /tb/DUT1/*
add wave -divider "Frame Receiver"
add wave -hexadecimal /tb/DUT2/*

run 500 us