vsim  -t  1ns  work.tbs

add wave -divider "Frame Simulator"
add wave  /tbs/DUT1/Trigger
add wave -hexadecimal /tbs/DUT1/Data_Stream
add wave  /tbs/DUT1/latchedTrigger
add wave -decimal /tbs/DUT1/TriggerCounter





add wave -divider "Frame Receiver"
add wave -hexadecimal /tbs/DUT2/Data_StreamIn
add wave -hexadecimal /tbs/DUT2/Data_StreamOut

add wave -divider "Trigger Candidate"
add wave -hexadecimal /tbs/DUT3/Data_Stream
add wave -hexadecimal /tbs/DUT3/TriggerCheck
add wave -decimal /tbs/DUT3/TriggerCounter
add wave -hexadecimal /tbs/DUT3/TriggerStream




run 500 us
