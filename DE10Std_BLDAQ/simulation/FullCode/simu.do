vsim  -t  1ns  work.tb
add wave -divider "Main Signals"
add wave -divider "Test bench"
add wave -hexadecimal /tb/*
add wave -divider "Busy Less DAQ Module  "
add wave -hexadecimal /tb/DUT/*
add wave -divider " InSignControl  "
add wave -hexadecimal /tb/DUT/InSigCtrl/*
add wave -divider " Trigger Control  "
add wave -hexadecimal /tb/DUT/TrgCntl/*
add wave -divider " Event Builder  "
add wave -hexadecimal /tb/DUT/EB/*
add wave -divider " Four Frame Receiver  "
add wave -hexadecimal /tb/DUT/EB/FR/*
add wave -divider " Stream Selector  "
add wave -hexadecimal /tb/DUT/EB/FR/SS/*
add wave -divider " Stream 0 "
add wave -hexadecimal /tb/DUT/EB/FR/Sens(0)/FS/*
add wave -divider " Stream 2 "
add wave -hexadecimal /tb/DUT/EB/FR/Sens(2)/FS/*

run 100 us