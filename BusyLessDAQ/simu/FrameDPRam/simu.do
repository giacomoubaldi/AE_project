vsim  -t  1ns  work.tb
add wave -divider "Main Signals"
add wave -hexadecimal /tb/*


add wave -divider "Trigger Control Simulator"
add wave -hexadecimal /tb/TC/*
add wave -divider "Frame Pointer Memory"
add wave -hexadecimal /tb/FPM/*

run 500 us