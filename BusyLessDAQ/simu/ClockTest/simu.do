vsim  -t  1ns  work.tb

add wave -divider "Main Signals"
add wave /tb/Clock
add wave /tb/nReset
add wave /tb/Reset
add wave /tb/TSClock
add wave /tb/TSReset
add wave -hexadecimal /tb/TSCounter
add wave /tb/onoff

add wave -divider "Rising Edge"
add wave -hexadecimal  /tb/Dut/*
add wave -divider "Random bit"
add wave -hexadecimal  /tb/RB/*


run 10 us