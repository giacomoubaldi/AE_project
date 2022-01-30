vsim  -t  1ns  work.tb
add wave -divider "Main Signals"
add wave /tb/Clock
add wave /tb/Reset
add wave /tb/TSReset
add wave /tb/TSClock
add wave -hexadecimal /tb/TSCounter

add wave -divider "Test inside entity"
add wave /tb/mainClock/*

add wave -divider "Test inside risingedge"
add wave /tb/counter/*

run 10 us