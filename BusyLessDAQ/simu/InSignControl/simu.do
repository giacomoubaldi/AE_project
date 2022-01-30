vsim  -t  1ns  work.tb
add wave -divider "Main Signals"
add wave /tb/Clk
add wave /tb/Reset
add wave /tb/BCOReset
add wave /tb/BCOClock
add wave /tb/Trigger
add wave /tb/BCOResetOut
add wave /tb/BCOClockOut
add wave /tb/TriggerOut
add wave -hexadecimal /tb/BCOCounter
add wave -hexadecimal /tb/trigRecCounter


add wave -divider "Test bench"
add wave -hexadecimal /tb/*
add wave -divider "In signal controls "
add wave -hexadecimal /tb/DUT_TS/*

run 10 us