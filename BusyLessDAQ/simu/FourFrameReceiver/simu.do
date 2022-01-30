vsim  -t  1ns  work.tb
add wave -divider "Main Signals"
add wave -hexadecimal /tb/*


add wave -divider "Detector Simulator"
add wave -hexadecimal /tb/DS/*
add wave -divider "Four Frame Receiver"
add wave -hexadecimal /tb/FR/*
add wave -divider "Stream Selector"
add wave -hexadecimal /tb/FR/SS/*

run 500 us