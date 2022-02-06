library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

entity BLDAQ_Module is
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	--
    --External clock 
    TSClockIn : in std_logic;      
    TSResetIn : in std_logic;    
    --Trigger signal from GPIO
    TriggerIn : in std_logic;   
    Busy_Out : out std_logic;    
	-- ADC --
	adc_raw_values : in adc_values_t;
	-- External data IN
	ExtData_Streams : in NSensStreamData_t;  
	ExtData_Valid : in std_logic_vector(N_Sensors-1 downto 0);
    ExtEndOfEvent : in std_logic_vector(N_Sensors-1 downto 0); 	
	-- I/O to RAM Manager 
    DDR3PointerHead : in unsigned(31 downto 0);
	SendPacket : in std_logic;
	WaitRequest : in std_logic;
    DataAvailable : out std_logic;
	Data_StreamOut : out unsigned (31 downto 0);  
	Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
	Data_Size : out unsigned (11 downto 0);  
    DDR3PointerTail : out unsigned(31 downto 0);
	--- Communication with HPS ---
    iofifo_datain         : in   std_logic_vector(31 downto 0);                      -- datain
    iofifo_writereq       : in   std_logic;                                          -- writereq
    iofifo_instatus       : out    std_logic_vector(31 downto 0) := (others => 'X'); -- instatus
    iofifo_dataout        : out    std_logic_vector(31 downto 0) := (others => 'X'); -- dataout
    iofifo_readack        : in   std_logic;                                          -- readack
    iofifo_outstatus      : out    std_logic_vector(31 downto 0) := (others => 'X'); -- outstatus
    iofifo_regdataout     : out   std_logic_vector(31 downto 0) := (others => 'X');  -- regdataout
    iofifo_regreadack     : in   std_logic;                                          -- regreadack
    iofifo_regoutstatus   : out    std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus
	--
	leds : out std_logic_vector(5 downto 0);
	-- For simulation only!!
	Comm_EventReady : out std_logic
  );
end entity;



architecture Structural of BLDAQ_Module is
  
  component DetectorSimulator is
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
    --Outputs
	Data_Streams : out NSensStreamData_t;  
	Data_Valid : out std_logic_vector(N_Sensors-1 downto 0);
    EndOfEvent : out std_logic_vector(N_Sensors-1 downto 0)	 
  );
  end component;

  -- Detector simulator data
  signal SimuData_Streams : NSensStreamData_t;  
  signal SimuData_Valid : std_logic_vector(N_Sensors-1 downto 0);
  signal SimuEndOfEvent : std_logic_vector(N_Sensors-1 downto 0);
  signal Data_Streams : NSensStreamData_t;  
  signal Data_Valid : std_logic_vector(N_Sensors-1 downto 0);
  signal EndOfEvent : std_logic_vector(N_Sensors-1 downto 0);

  component Counter_nbit is
  generic ( nbit : natural := 16);
  port(
    Clock: in std_logic;
    Reset: in std_logic;
    Count : out unsigned (nbit-1 downto 0)
  );
  end component Counter_nbit;

  component InSignControl is
  port (
    --Inputs 
    Clock : in std_logic;          
    Reset : in std_logic;                
    --External clock 
    TSClockIn : in std_logic;      
    TSResetIn : in std_logic;    
    --Trigger signal from GPIO
    TriggerIn : in std_logic;       
     
    --Outputs
    TSClockOut : out std_logic;      
    TSResetOut : out std_logic;    
    --Trigger signal received
    TriggerOut  : out std_logic
  );
  end component InSignControl;
  
  signal TriggerCandidate  : std_logic;  -- Trigger signal received 

  signal TSClockOut, TSResetOut : std_logic;
  --TSCounter Counts the number of TSClocks      
  signal TSCounter : unsigned (31 downto 0);  
  signal ClockCounter : unsigned (47 downto 0); 
  signal BusyClock : std_logic;  

  component Main_FSM
  generic( -- Minimum time to wait before checking if it's possible to go to config mode
	Time_Out : unsigned (31 downto 0)  );
  port(
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	To_State : in std_logic_vector (1 downto 0);
	-- MV: Usage of these two signals has to be checked!!
	Empty_Fifo : in std_logic;
	ReadingEvent : in std_logic;
	--State of the machine
	Status : out MainFSM_state;
	--Outputs
	DAQIsRunning : out std_logic;
	DAQ_Reset : out std_logic;
	DAQ_Config : out std_logic
  );
  end component;
  ----  FSM Signals  ----
  signal internalDAQIsRunning : std_logic :='0';
  signal internalDAQ_Reset : std_logic:='0';
  signal internalDAQ_Config : std_logic:='0';
  signal Status_Bus : MainFSM_state;
  signal DoStore : std_logic := '0';
  signal DataAvailableInt : std_logic;

  component Trigger_Control is
  port (
    --Inputs 
    Clock : in std_logic;          
    Reset : in std_logic;                
    --The Finite state machine is in Running State
    DAQIsRunning : in std_logic;  
    --Resets Counters preparing for run 
    DAQ_Reset : in std_logic;
    --External clock 
    TSClock : in std_logic;      
    TSReset : in std_logic;    
    --Trigger signal from GPIO
    Trigger : in std_logic;       
    --Internal Busy from Event Builder
    Busy : in std_logic;
     
    --Inputs
    ClockCounterIn : in unsigned (47 downto 0);      
    --Trigger is sent to the inner part of the machine if DAQ is Running and the event builder is not busy
    Internal_Trigger : out std_logic ; -- is this needed?
    triggerAccepted : out unsigned (31 downto 0);  
    Busy_Out : out std_logic; 
    TSCounter : out unsigned (31 downto 0);

    -- Fifos Out
    TrgDataReady : out std_logic;
    TrgReadAcknowledge : in std_logic;
    ClockCounterOut : out unsigned (47 downto 0);   	
    triggerCounterOut : out unsigned (31 downto 0);  
    triggerAcceptedOut : out unsigned (31 downto 0);  
    TrgTSCounter : out unsigned (31 downto 0);
    --
	TrgFifo_info : out std_logic_vector(15 downto 0);
	ErrorOnTrg : out std_logic
  );
  end component;
  
  component EventBuilder is
  port (
    Clock : in std_logic;
	Reset : in std_logic;
	DAQ_Reset : in std_logic;
	-- From General Control
	SensorEnableIn : in std_logic_vector(N_sensors-1 downto 0);
	DoStore : in std_logic;
		-- Time registers for the time interval to read
	PreTriggerClockTicks : in unsigned(47 downto 0);
	PostTriggerClockTicks : in unsigned(47 downto 0);
	WaitingClockTicks : in unsigned(47 downto 0);
	-- Time tags
	TSCounter : in unsigned(31 downto 0);
	ClockCounter : in unsigned(47 downto 0);
	-- From sensors
    Data_Streams : in NSensStreamData_t;  
	Data_Valid : in std_logic_vector(N_Sensors-1 downto 0);
    EndOfEvent : in std_logic_vector(N_Sensors-1 downto 0);
    -- I/O to RAM Manager 
    DDR3PointerHead : in unsigned(31 downto 0);
	SendPacket : in std_logic;
	WaitRequest : in std_logic;
    DataAvailable : out std_logic;
	Data_StreamOut : out unsigned (31 downto 0);  
	Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
	Data_Size : out unsigned (11 downto 0);  
    DDR3PointerTail : out unsigned(31 downto 0);
	-- From/to TriggerControl
    FifoDataReady : in std_logic;
    TrgClockCounter : in unsigned (47 downto 0);   	
	TrgReceivedCounter : in unsigned (31 downto 0);  
    TrgAcceptedCounter : in unsigned (31 downto 0);  
    TrgTSCounter : in unsigned (31 downto 0);
    TrgReadAcknowledge : out std_logic;
	-- To the extrnal process reading the EventFifo
	EVDataOut : out std_logic_vector (223 downto 0);
	EVDataOutReady : out std_logic;
    EVReadAcknowledge: in std_logic;
	EVFifoStatus : out std_logic_vector (15 downto 0);
	    -- A memory is Full ....
	BusyOut : out std_logic;
	EBDebug : out std_logic_vector(31 downto 0)
  );
  end component;
  
  -- ---------------------------------- trigger candidate from datastream
  component Trigger_Candidate is
    --Port declaration
     port (
     --Inputs
     Clock : in std_logic;
     Reset : in std_logic;
  
  --Input stream
  Data_Stream : in unsigned (31 downto 0);
  --Data_Valid: in std_logic;
  Data_Valid : in std_logic;
  
  --Output
  --TriggerCounterOut: out unsigned (11 downto 0)
  TriggerCheck : out std_logic;
  TriggerStream: out unsigned (31 downto 0)
  
  );
  end component;



  component CommunicationModule is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	--- Communication with DAQMOdule
	DAQ_Config : in std_logic;
	Monitor_Registers_Bus : in MONITOR_REGS_T;
	Control_Registers_Bus : out CONTROL_REGS_T := default_CtrlRegisters;
	--- Communication with HPS ---
    iofifo_datain         : in   std_logic_vector(31 downto 0);                    -- datain
    iofifo_writereq       : in   std_logic;                                        -- writereq
    iofifo_instatus       : out    std_logic_vector(31 downto 0) := (others => 'X'); -- instatus
    iofifo_dataout        : out    std_logic_vector(31 downto 0) := (others => 'X'); -- dataout
    iofifo_readack        : in   std_logic;                                        -- readack
    iofifo_outstatus      : out    std_logic_vector(31 downto 0) := (others => 'X'); -- outstatus
    iofifo_regdataout     : out   std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
    iofifo_regreadack     : in   std_logic;                                        -- regreadack
    iofifo_regoutstatus   : out    std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus
	-- Data To/From Event Builder
	FifoEB_info : in std_logic_vector(15 downto 0);
	EVReadAcknowledge : out std_logic;
	-- FIFO RX/TX stats
	FifoRX_info : out std_logic_vector(15 downto 0);
	FifoTX_info : out std_logic_vector(15 downto 0);
	--- Errors ---
	Reset_Errors : in std_logic;
	Invalid_Address : out std_logic := '0';
	INOut_Both_Active : out std_logic := '0'
  );
  end component;

  -- Trigger Fifo (mostly Out)
  signal TrgFifoDataReady : std_logic;
  signal TrgReadAcknowledge : std_logic;
  signal TrgClockCounter : unsigned (47 downto 0);   	
  signal TrgReceivedCounter : unsigned (31 downto 0);  
  signal TrgAcceptedCounter : unsigned (31 downto 0);  
  signal TrgTSCounter : unsigned (31 downto 0);

  signal EvDataOut : std_logic_vector (223 downto 0);
  signal EVDataOutReady : std_logic;
  signal EVFifoStatus : std_logic_vector (15 downto 0);
  signal EVReadAcknowledge : std_logic;
  signal DDR3PointerTailInt : unsigned(31 downto 0);

  -- Registers signals --
  signal Monitor_Registers_Bus : MONITOR_REGS_T:= default_MonRegisters;
  signal Control_Registers_Bus : CONTROL_REGS_T:= default_CtrlRegisters;
	  
  -- FIFO RX/TX stats
  signal FifoEB_info : std_logic_vector(15 downto 0);
  signal FifoRX_info : std_logic_vector(15 downto 0);
  signal FifoTX_info : std_logic_vector(15 downto 0);

  signal EB_Busy, Trg_Busy, BusyIn : std_logic;
  signal Reset_Errors : std_logic;
  signal SimuFlag : std_logic;

  signal triggerAccepted : unsigned (31 downto 0);
  signal TrgFifo_info : std_logic_vector(15 downto 0);
  signal ErrorOnTrg : std_logic;  


-- Trigger Candidate
signal TriggerCheck : std_logic := '0';
signal TriggerStream: unsigned (31 downto 0);

-- Event builder
signal Data_StreamEB : unsigned (31 downto 0);
signal Data_ValidEB : std_logic;


begin

  -- --------------------------------------------
  --   Detector simulator... to be put elsewhere!
  -- --------------------------------------------
  DS: DetectorSimulator
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
    --Outputs
	Data_Streams => SimuData_Streams,
	Data_Valid => SimuData_Valid,
    EndOfEvent => SimuEndOfEvent	 
  );
  
  process(SimuFlag, ExtData_Streams, ExtData_Valid, ExtEndOfEvent, SimuData_Streams, SimuData_Valid, SimuEndOfEvent)
  begin
    case SimuFlag is
    when '0' =>
      Data_Streams <= ExtData_Streams;
      Data_Valid <= ExtData_Valid;
  	  EndOfEvent <= ExtEndOfEvent;
    when others =>
      Data_Streams <= SimuData_Streams;
      Data_Valid <= SimuData_Valid;
  	  EndOfEvent <= SimuEndOfEvent;
    end case;
  end process;

  SimuFlag <= Control_Registers_Bus(Config_Reg)(UseSimulation);
  
  -- --------------------------------------------
  -- External Control Signals
  -- --------------------------------------------
  InSigCtrl: InSignControl 
  port map (
    --Inputs 
    Clock => Clock,         
    Reset => Reset,                
    --External signals
    TSClockIn => TSClockIn,      
    TSResetIn => TSResetIn,    
    TriggerIn => TriggerCheck,   --- from TriggerCandidate    
     
    --Outputs
    TSClockOut => TSClockOut,
    TSResetOut => TSResetOut,    
    --Trigger signal received
    TriggerOut => TriggerCandidate
  );

  -- --------------------------------------------
  -- Main long BX time stamp
  -- --------------------------------------------
  TSBX: Counter_nbit
    generic map(nbit => 48)
	port map(Clock => Clock, Reset => Reset, Count => ClockCounter);
  --  BusyClock <= '1' when ClockCounter(47 downto 16)=x"ffff_ffff" or ClockCounter(47 downto 16)=x"0000_0000", -- 65k*20*2= 2.7 ms, 4096*20*2=160. us
  BusyClock <= '1' when ClockCounter(47 downto 8)=x"ffff_ffff_ff" or ClockCounter(47 downto 8)=x"0000_0000_00" -- 256*20*2=10 us
                   else '0';

  -- --------------------------------------------
  -- Main Final State Machine
  -- --------------------------------------------
  Finite_State_Machine: Main_FSM
  generic map( Time_Out => x"0010_0000") -- about 1M Clock ticks = 20 ms at 50 MHz clock
  port map(
    -- Inputs
    Clock => Clock,
	Reset => Reset,
	To_State => Control_Registers_Bus(ToState_Reg)(1 downto 0),
	Empty_Fifo => Monitor_Registers_Bus(EB_Fifos_Reg)(EBEmpty_Flag),
	ReadingEvent => DataAvailableInt,
	Status => Status_Bus,
	-- Outputs
	DAQIsRunning => internalDAQIsRunning,
	DAQ_Reset => internalDAQ_Reset,
	DAQ_Config => internalDAQ_Config
  );	  
	  
  -- --------------------------------------------
  -- Trigger control
  -- --------------------------------------------
  TrgCntl: Trigger_Control 
  port map (
	Clock => Clock, Reset =>Reset,
    DAQIsRunning => internalDAQIsRunning, DAQ_Reset => internalDAQ_Reset,
    --External clock 
    TSClock => TSClockOut, TSReset => TSResetOut,
    --Trigger signal from signal
    -- (before from GPIO)
    Trigger => TriggerCandidate,     
    --Internal Busy from EventFifo
    Busy => BusyIn,
     
    --Inputs
    ClockCounterIn => ClockCounter, 

	-- Outputs
    Internal_Trigger => open,  -- is this needed?
	triggerAccepted => triggerAccepted,
    --Busy out passes a stretched version of the Event Builder Busy to GPIO 
    Busy_Out => Trg_Busy,
    TSCounter => TSCounter,
    -- Fifos Out
    TrgDataReady => TrgFifoDataReady,
    TrgReadAcknowledge => TrgReadAcknowledge,
    ClockCounterOut => TrgClockCounter,	
    triggerCounterOut => TrgReceivedCounter,
    triggerAcceptedOut => TrgAcceptedCounter,
    TrgTSCounter => TrgTSCounter,
    TrgFifo_info => TrgFifo_info,
    ErrorOnTrg => Monitor_Registers_Bus (Errors_Reg)(triggerLost_Flag)
  );
  BusyIn <= '1' when EB_Busy='1' or BusyClock='1' or
                (TrgFifoDataReady='1' and Control_Registers_Bus(Config_Reg)(SingleTrigger_Flag)='1')
                else '0';
  Busy_Out <= Trg_Busy;
  -- --------------------------------------------
  -- Event building
  -- --------------------------------------------
  EB: EventBuilder
  port map (
	Clock => Clock, Reset => Reset,
	DAQ_Reset => internalDAQ_Reset,
	-- From General Control
	SensorEnableIn => Control_Registers_Bus(Config_reg)(N_Sensors-1 downto 0),
	DoStore => DoStore, 
	-- Time registers for the time interval to read
	PreTriggerClockTicks(31 downto 0) => unsigned(Control_Registers_Bus(preTrig_reg)), 
	PreTriggerClockTicks(47 downto 32) => (others=>'0'),
	PostTriggerClockTicks(31 downto 0) => unsigned(Control_Registers_Bus(postTrig_reg)), 
	PostTriggerClockTicks(47 downto 32) => (others=>'0'),
	WaitingClockTicks(31 downto 0) => unsigned(Control_Registers_Bus(waitTrig_reg)), 
	WaitingClockTicks(47 downto 32) => (others=>'0'),
	-- Time tags
	TSCounter => TSCounter,
	ClockCounter => ClockCounter,
	-- From sensors
	Data_Streams => Data_Streams,
	Data_Valid => Data_Valid,
    EndOfEvent => EndOfEvent,

    -- I/O to RAM Manager 
	DDR3PointerHead => DDR3PointerHead,
	SendPacket => SendPacket,
	WaitRequest => WaitRequest,
    DataAvailable => DataAvailableInt,
	Data_StreamOut => Data_StreamEB,
	Data_ValidOut => Data_ValidEB,
    EndOfEventOut => EndOfEventOut,
	Data_Size => Data_Size,
	DDR3PointerTail => DDR3PointerTailInt,
	-- From/to TriggerControl
    FifoDataReady => TrgFifoDataReady,
    TrgClockCounter => TrgClockCounter,	
	TrgReceivedCounter => TrgReceivedCounter,
	TrgAcceptedCounter => TrgAcceptedCounter,
	TrgTSCounter => TrgTSCounter,
    TrgReadAcknowledge => TrgReadAcknowledge,
	-- To the Registers
	EVDataOut => EvDataOut,
	EVDataOutReady => EVDataOutReady,
    EVReadAcknowledge => EVReadAcknowledge,
	EVFifoStatus => EVFifoStatus,
	-- A memory is Full
	BusyOut => EB_Busy,
	EBDebug => Monitor_Registers_Bus (EBDebug_Reg)
  );
  DoStore <= InternalDAQIsRunning and Control_Registers_Bus(Config_reg)(DoStore_Flag);
  DDR3PointerTail <= DDR3PointerTailInt;
  DataAvailable <= DataAvailableInt;
  
  -- for simulation only
  Comm_EventReady <= EVDataOutReady;


  Data_StreamOut <= Data_StreamEB;   -- out of eventbuilder
  Data_ValidOut <= Data_ValidEB;


  -- --------------------------------------------
  -- Trigger candidate checker
  -- --------------------------------------------
  TrgCandidate: Trigger_Candidate 
     port map (
     --Inputs
     Clock => Clock,
     Reset => Reset,
  
  --Input stream   -- from event builder 
  Data_Stream => Data_StreamEB,
  --Data_Valid: in std_logic;
  Data_Valid => Data_ValidEB ,
  
  --Output
  --TriggerCounterOut: out unsigned (11 downto 0)
  TriggerCheck =>  TriggerCheck,
  TriggerStream =>  TriggerStream
  
  );



  -- --------------------------------------------
  -- Register file
  -- --------------------------------------------
  ComMod: CommunicationModule 
  port map (
    Clock => Clock,
    Reset => Reset,
	--- Communication with DAQMOdule
	DAQ_Config => internalDAQ_Config,
	Monitor_Registers_Bus => Monitor_Registers_Bus,
	Control_Registers_Bus => Control_Registers_Bus,
	
	--- Communication with HPS ---
    iofifo_datain                         => iofifo_datain, 
    iofifo_writereq                       => iofifo_writereq,
    iofifo_instatus                       => iofifo_instatus,
    iofifo_dataout                        => iofifo_dataout,
    iofifo_readack                        => iofifo_readack,
    iofifo_outstatus                      => iofifo_outstatus,
    iofifo_regdataout                     => iofifo_regdataout,
    iofifo_regreadack                     => iofifo_regreadack,
    iofifo_regoutstatus                   => iofifo_regoutstatus,
	-- Data To/From Event Builder
	FifoEB_info => EVFifoStatus,
	EVReadAcknowledge => EVReadAcknowledge,
	-- FIFO RX/TX stats
	FifoRX_info => FifoRX_info,
	FifoTX_info => FifoTX_info,
	--- Errors ---
	Reset_Errors => Reset_Errors,
	Invalid_Address => Monitor_Registers_Bus (Errors_Reg)(InvalidAddress_Flag),
	INOut_Both_Active => Monitor_Registers_Bus (Errors_Reg)(InOutBothActive_Flag)
  );
  
  --Reset_DAQErrors <= Reset_Errors or internalDAQ_Reset;
   
  -- --------------------------------------------
  -- Sets all the monitor registers
  -- --------------------------------------------
  
  -- Register assignments
  Monitor_Registers_Bus(Firmware_Reg) <= Firmware_Version;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(DAQ_IsRunning_Flag) <= internalDAQIsRunning;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(DAQ_Reset_Flag)     <= internalDAQ_Reset;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(DAQ_Config_Flag)    <= internalDAQ_Config;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(Busy_Flag)    <= BusyIn;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(BusyEB_Flag)  <= EB_Busy;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(BusyTrg_Flag) <= Trg_Busy;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(2 downto 0) <= To_mystdlogicvector(Status_Bus);

  Monitor_Registers_Bus (TriggerCounter_Reg) <= std_logic_vector(triggerAccepted); 
  Monitor_Registers_Bus (BCOCounter_Reg)     <= std_logic_vector(TSCounter);    
  Monitor_Registers_Bus (ClkCounter_Reg)     <= std_logic_vector(ClockCounter(47 downto 16));  
  Monitor_Registers_Bus (LSB_ClkCounter_Reg) <= x"0000" & std_logic_vector(ClockCounter(15 downto 0));    
  
  Monitor_Registers_Bus(EB_Fifos_Reg) <= TrgFifo_info & EVFifoStatus;

  Monitor_Registers_Bus(LocalRXTX_Fifo_Reg) <= FifoTX_info & FifoRX_info;
  Monitor_Registers_Bus(Errors_Reg)(31 downto 4)              <= (others=>'0');
  
  GenADC:   -- copy eight 12 bit values into four 32 bit registers
  for i in 0 to 3 generate
    Monitor_Registers_Bus(ADC_ch0_reg+i)(11 downto 0)  <= adc_raw_values(i+i);
    Monitor_Registers_Bus(ADC_ch0_reg+i)(23 downto 12) <= adc_raw_values(i+i+1);
  end generate;
  --
  -- Event Builder data to regs
  Monitor_Registers_Bus(EvHeader_Reg)       <= Control_Registers_Bus(VariableHeader_Reg)(31 downto 16) & EVFifoStatus;
  Monitor_Registers_Bus(EvTSCnt_Reg)        <= EvDataOut(31 downto 0);
  Monitor_Registers_Bus(EvTrgAccCnt_Reg)    <= EvDataOut(63 downto 32);
  Monitor_Registers_Bus(EvTrgRecCnt_Reg)    <= EvDataOut(95 downto 64);
  Monitor_Registers_Bus(EvTrgMSBClkCnt_Reg) <= EvDataOut(143 downto 112);
  Monitor_Registers_Bus(EvTrgLSBClkCnt_Reg) <= EvDataOut(159 downto 144) & EvDataOut(111 downto 96); -- size and LSB clock counter
  Monitor_Registers_Bus(EvDDR3PtrMin_Reg)   <= EvDataOut(191 downto 160);
  Monitor_Registers_Bus(EvDDR3PtrMax_Reg)   <= EvDataOut(223 downto 192);

  -- Memory status
  Monitor_Registers_Bus(DDR3PtrTail_Reg) <= std_logic_vector(DDR3PointerTailInt);
  Monitor_Registers_Bus(DDR3PtrHead_Reg) <= std_logic_vector(DDR3PointerHead);
  -- LEDS
  leds <= internalDAQIsRunning & internalDAQ_Config & internalDAQ_Reset & Monitor_Registers_Bus(FSM_StatusSignals_Reg)(2 downto 0);
--?? To be finished
  Reset_Errors <= '0';

end architecture;