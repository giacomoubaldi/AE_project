vsim  -t  1ns  work.tb
add wave -divider "Detector Simulator"
add wave -hexadecimal /tb/DUT1/Data_Streams
add wave -hexadecimal /tb/DUT1/Trigger

add wave -divider "Four Frame Receiver"
add wave -hexadecimal /tb/DUT2/Data_StreamIn
add wave -hexadecimal /tb/DUT2/Data_StreamOut

--add wave -divider "Trigger Candidate"
add wave  /tb/DUT3/TriggerCheck
add wave  -hexadecimal /tb/DUT3/TriggerStream

run 500 us
