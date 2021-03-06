library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


package BLDAQ_Package is

 -- Contained in monitor register 0  (abs reg 16)
  constant Firmware_Version : std_logic_vector(31 downto 0) := x"de100000"; 
  
  constant N_MONITOR_REGS : natural := 32; -- Number of mapped monitor registers
  constant N_CONTROL_REGS : natural := 32; -- Number of mapped control registers
  
  -- Types for monitor and control register arrays --
  type CONTROL_REGS_T is ARRAY (0 to N_CONTROL_REGS-1) of 
                         STD_LOGIC_VECTOR(31 downto 0);
  type MONITOR_REGS_T is ARRAY (0 to N_MONITOR_REGS-1) of 
                         STD_LOGIC_VECTOR(31 downto 0);
   
  -- State of the Main Finite State Machine --
  type MainFSM_state is (Idle, Config, PrepareForRun, Run, EndOfRun, WaitingEmptyFifo);

  -- This is the timestamp using the internal clock
  subtype TimeStampLong_t is unsigned(47 downto 0);
  -- This is the timestamp using the external BCO signal
  subtype TimeStampExt_t is unsigned(31 downto 0);

  -- 
  constant N_Sensors : natural := 4; -- number of sensors to consider
  subtype StreamData_t is unsigned(31 downto 0);
  type NSensStreamData_t is ARRAY (N_sensors-1 downto 0) of StreamData_t;
  type NSensTSExt_t is ARRAY(N_sensors-1 downto 0) of TimeStampExt_t;
  type NSensTSLong_t is ARRAY(N_sensors-1 downto 0) of TimeStampLong_t;
  
  -- Constants    
  -- To_State signal Code for Main_FSM --
  constant IdleToConfig : std_logic_vector (1 downto 0) := "00";
  constant ConfigToRun : std_logic_vector (1 downto 0) := "01";
  constant RunToConfig : std_logic_vector (1 downto 0) := "10";
  constant ConfigToIdle : std_logic_vector (1 downto 0) := "11";
  --
  -- These are the values used for the header of data read from register file --
  constant Header1_RF : std_logic_vector (31 downto 0) := x"EADE2020";
  constant Header2_RF : std_logic_vector (31 downto 0) := x"EADE2121";
  -- These are the values used for the footer of data coming from register file --
  constant Footer1_RF : std_logic_vector (31 downto 0) := x"FAFEFAFE";
  constant Footer2_RF : std_logic_vector (31 downto 0) := x"BACCA000";
   
  -- Number of cycles LocalEth have to wait before reading the first word of an instruction --
  constant Timeout : natural := 10;
   
    -- Default values for every Register in the register file --
  -- Control Registers --
  
  constant CtrlReg0 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg1 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg2 : std_logic_vector (31 downto 0) := x"EADEBECC";
  constant CtrlReg3 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg4 : std_logic_vector (31 downto 0) := x"0000010F";
  constant CtrlReg5 : std_logic_vector (31 downto 0) := x"00000001";
  constant CtrlReg6 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg7 : std_logic_vector (31 downto 0) := x"00000400";
  constant CtrlReg8 : std_logic_vector (31 downto 0) := x"00000400";
  constant CtrlReg9 : std_logic_vector (31 downto 0) := x"00000800";
  constant CtrlReg10 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg11 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg12 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg13 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg14 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlReg15 : std_logic_vector (31 downto 0) := x"00000000";
  constant CtrlNullReg : std_logic_vector (31 downto 0) := x"00000000";
   
  -- Monitor Registers --
   
  constant MonReg0 : std_logic_vector (31 downto 0) := Firmware_Version;
  constant MonReg1 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg2 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg3 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg4 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg5 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg6 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg7 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg8 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg9 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg10 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg11 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg12 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg13 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg14 : std_logic_vector (31 downto 0) := x"00000000";
  constant MonReg15 : std_logic_vector (31 downto 0) := x"00000000"; 
  constant MonNullReg : std_logic_vector (31 downto 0) := x"00000000"; 
  
  -- Default Arrays --
  constant default_MonRegisters : MONITOR_REGS_T := (
  	MonReg0, MonReg1, MonReg2, MonReg3,
  	MonReg4, MonReg5, MonReg6, MonReg7,
  	MonReg8, MonReg9, MonReg10, MonReg11,
  	MonReg12, MonReg13, MonReg14, MonReg15, 
	MonNullReg, MonNullReg, MonNullReg, MonNullReg,
	MonNullReg, MonNullReg, MonNullReg, MonNullReg,
	MonNullReg, MonNullReg, MonNullReg, MonNullReg,
	MonNullReg, MonNullReg, MonNullReg, MonNullReg
	);
  constant default_CtrlRegisters : CONTROL_REGS_T := (
  	CtrlReg0, CtrlReg1, CtrlReg2, CtrlReg3,
  	CtrlReg4, CtrlReg5, CtrlReg6, CtrlReg7,
  	CtrlReg8, CtrlReg9, CtrlReg10, CtrlReg11,
  	CtrlReg12, CtrlReg13, CtrlReg14, CtrlReg15, 
	CtrlNullReg, CtrlNullReg, CtrlNullReg, CtrlNullReg,
	CtrlNullReg, CtrlNullReg, CtrlNullReg, CtrlNullReg,
	CtrlNullReg, CtrlNullReg, CtrlNullReg, CtrlNullReg,
	CtrlNullReg, CtrlNullReg, CtrlNullReg, CtrlNullReg
	);
  -- End of Default Values --
  
  ----  Headers of the incoming instrucions  ----
  -- Change FSM State --
  constant Header_ChangeState : std_logic_vector (31 downto 0) := x"EADE0080";
  -- Read registers from register file --
  constant Header_ReadRegs : std_logic_vector (31 downto 0) := x"EADE0081";
  -- Write registers from register file --
  constant Header_WriteRegs : std_logic_vector (31 downto 0) := x"EADE0082";
  -- Reset Registers of the register file --
  constant Header_ResetRegs : std_logic_vector (31 downto 0) := x"EADE0083";
  -- Footer for the Ethernet instructons --
  constant Instruction_Footer : std_logic_vector(31 downto 0) := x"F00E0099"; 
  
  -- Control Register Map --
  constant Dummy_Reg : natural := 0;
  constant ToState_Reg : natural := 1;
  constant VariableHeader_Reg : natural := 2;
  constant BusyMode_Reg : natural := 3;
  constant Config_Reg : natural := 4;
  constant   DoStore_Flag : natural := 8;
  constant RAM_interface_En_Reg : natural := 5;
  constant simulated_acquisition_reg : natural := 6;
  constant preTrig_reg : natural := 7;
  constant postTrig_reg : natural := 8;
  constant waitTrig_reg : natural := 9;
  -- Monitor Register Map and Flag map --
  constant Firmware_Reg : natural := 0;
  constant FSM_StatusSignals_Reg : natural := 1;
  constant   DAQ_IsRunning_Flag : natural := 28;
  constant   DAQ_Reset_Flag : natural := 27;
  constant   DAQ_Config_Flag : natural := 26;
  constant   ReadingEvent_Flag : natural := 25;
  constant Errors_Reg : natural := 2;
  constant   ErrorBusy_Flag : natural := 0;
  constant   ErrorNotRunning_Flag : natural := 1;
  constant   InvalidAddress_Flag : natural := 2;
  constant   InOutBothActive_Flag : natural := 3;
  constant TriggerCounter_Reg : natural := 3;
  constant BCOCounter_Reg : natural := 4;
  constant ClkCounter_Reg : natural := 5;
  constant LSB_ClkCounter_Reg : natural := 6;
  constant EB_Fifos_Reg : natural := 7;
  constant   EBFull_Flag : natural := 31;
  constant   EBAlmostFull_Flag : natural := 30;
  constant   EBEmpty_Flag : natural := 29;
  constant   EBMetadataFull_Flag : natural := 28;
  constant   EBMetadataFull_Flag2 : natural := 20;
  constant   EBFull_Flag2 : natural := 11;
  constant LocalTX_Fifo_Reg : natural := 8;  -- bits 11-0 non pi?? usati....
  constant   TXFull_Flag : natural := 31;
  constant   TXFull_Flag2 : natural := 11;
  constant   TXAlmostFull_Flag : natural := 30;
  constant   TXEmpty_Flag : natural := 29;
  constant   TXRegFifoFull_Flag : natural := 28;
  constant   TXRegFifoFull_Flag2 : natural := 25;
  constant   TXRegFifoAlmostFull_Flag : natural := 27;
  constant   TXRegFifoEmpty_Flag : natural := 26;
  constant LocalRX_Fifos_Reg : natural := 9;
  constant   RXFull_Flag : natural := 31;
  constant   RXFull_Flag2 : natural := 9;
  constant   RXAlmostFull_Flag : natural := 30;
  constant   RXEmpty_Flag : natural := 29;
  constant   RX_outFifo_Full_Flag : natural := 28;
  constant   RX_outFifo_Empty_Flag : natural := 27;
  constant ADC_ch0_reg : natural := 10;--------------------------------------
  constant ADC_ch1_reg : natural := 10;
  constant ADC_ch2_reg : natural := 11;
  constant ADC_ch3_reg : natural := 11;
  constant ADC_ch4_reg : natural := 12;
  constant ADC_ch5_reg : natural := 12;
  constant ADC_ch6_reg : natural := 13;
  constant ADC_ch7_reg : natural := 13;
  -- Event regs
  constant EvHeader_Reg       : natural := 14;
  constant EvTSCnt_Reg        : natural := 15;
  constant EvTrgAccCnt_Reg    : natural := 16;
  constant EvTrgRecCnt_Reg    : natural := 17;
  constant EvTrgMSBClkCnt_Reg : natural := 18;
  constant EvTrgLSBClkCnt_Reg : natural := 19;
  constant EvDDR3PtrMin_Reg   : natural := 20;
  constant EvDDR3PtrMax_Reg   : natural := 21;
   
   
   -- GPIO_0 Pin configuration--
  constant Trigger_Pin  : natural := 33; -- 1;
  constant BCOReset_Pin : natural := 30;
  constant BCOClock_Pin : natural := 35;--25;--25 -- 9
  constant BusyOut_Pin  : natural := 31; --11;
  constant BeamOut_Pin      : natural := 17;
  constant BeamStartOut_Pin : natural := 19;
  constant BeamEndOut_Pin   : natural := 21;
  -- The other pins are connected to ground -- 
   
   
    -- ADC parameters and definitions
  constant N_CHANNELS : natural := 8; -- Number of channels of the ADC
  
  -- Array containing the values of the ADC
  type adc_values_t is ARRAY (0 to N_CHANNELS-1) of 
                         STD_LOGIC_VECTOR(11 downto 0);
  
  -- Function to convert state --
  function To_mystdlogicvector ( Status : MainFSM_state )
                   return std_logic_vector;
end;

package body BLDAQ_Package is

  function To_mystdlogicvector ( Status : MainFSM_state )
    return std_logic_vector is
	   variable result : std_logic_vector( 2 downto 0 );
    begin
	   case Status is
		  when idle =>
		    result := "000";
		  when Config =>
          result := "001";
		  when PrepareForRun =>
          result := "010";
		  when Run =>
          result := "011";
		  when EndOfRun =>
          result := "100";
		  when WaitingEmptyFifo =>
          result := "101";
		  when others =>
          result := "110";
      end case;
  		return result;
	end To_mystdlogicvector;
	
end BLDAQ_Package;
 
