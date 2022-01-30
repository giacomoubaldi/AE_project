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
    --External clock        -- ts stands for transport stream
    TSClockIn : in std_logic;
    TSResetIn : in std_logic;
    --Trigger signal from GPIO  - general purpose input output
    TriggerIn : in std_logic;
    To_State : in std_logic_vector (1 downto 0);
	-- ADC --
	adc_raw_values : in adc_values_t;
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
	-- Communication with HPS control
	-- Input from Ethernet --
	Comm_Rdreq : in std_logic;
	Comm_RegistersRdreq : in std_logic;
	Comm_Wrreq : in std_logic;
	Comm_DataIn : in std_logic_vector(31 downto 0);
	Comm_EVReadAcknowledge : in std_logic;
    -- Out
	Comm_DataOut : out std_logic_vector(31 downto 0);
	Comm_RegistersOut : out std_logic_vector (31 downto 0);
	Comm_regfifostatus : out std_logic_vector (31 downto 0);
	--
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
      --Outputs
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

  signal TriggerCandidate  : std_logic;  -- Trigger signal received : it will be TriggerOut from InSignControl

  signal TSClockOut, TSResetOut : std_logic;

  signal TSCounter : unsigned (31 downto 0);    --TSCounter Counts the number of TSClocks     --> TriggerControl, EventBuilder
  signal ClockCounter : unsigned (47 downto 0);   --ClockCounter Counts the number of input Clocks    --> TSBx
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
    --ClkCounter counts the number of clocks since DAQIsRunning passed from 0 to 1. The counter has 38bit
    ClockCounterIn : in unsigned (47 downto 0);
    --Trigger is sent to the inner part of the machine if DAQ is Running and the event builder is not busy
    Internal_Trigger : out std_logic ; -- is this needed?
    --Busy out passes a stretched version of the Event Builder Busy to GPIO
    Busy_Out : out std_logic;
    TSCounter : out unsigned (31 downto 0);
    -- Trg Fifos Out
    TrgDataReady : out std_logic;
    TrgReadAcknowledge : in std_logic;
    ClockCounterOut : out unsigned (47 downto 0);
    triggerCounterOut : out unsigned (31 downto 0);
    triggerAcceptedOut : out unsigned (31 downto 0);
    TrgTSCounter : out unsigned (31 downto 0)
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
	BusyOut : out std_logic
  );
  end component;

  component Register_File is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	--- Communication with Local Ethernet Interace ---
	Address : in natural;
	Rdreq : in std_logic;
	Wrreq : in std_logic;
	DataIn : in std_logic_vector (31 downto 0);
	Data_ValidOut : out std_logic := '0';
	DataOut: out std_logic_vector (31 downto 0) := (others => '0');
	--- Selective Reset contains a Reset std_logic line for every Control Register ---
	Selective_Reset : in std_logic_vector(N_CONTROL_REGS-1 downto 0);
	--- Communication with Main_FSM ---
	Monitor_RegistersIn : in MONITOR_REGS_T;
	Control_RegistersOut : out CONTROL_REGS_T := default_CtrlRegisters;
	--- Errors ---
	Reset_Errors : in std_logic;
	Invalid_Address : out std_logic := '0';
	INOut_Both_Active : out std_logic := '0'
  );
  end component;


  component Local_RX is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	-- Data from outher world
	Comm_wrreq : in std_logic;
	Comm_Data : in std_logic_vector(31 downto 0);
	-- Configuration mode from FSM
	DAQ_Config : in std_logic;
	-- Read Request from Local TX
	LocalTX_Rdreq : in std_logic;
	-- Data From Register File
	Register_DataValid : in std_logic;
	Register_DataRead : in std_logic_vector(31 downto 0);
	-- Data to Register file
	Register_Address : out natural;
	Register_Wrreq : out std_logic;
	Register_Rdreq : out std_logic;
	Register_SelectiveReset : out std_logic_vector(N_CONTROL_REGS-1 downto 0) := (others => '0');
	Register_DataWrite : out std_logic_vector(31 downto 0);
	-- Data to LocalTX
	DataOut : out std_logic_vector(31 downto 0);
	Data_Ready : out std_logic := '0';
	-- FIFORX stats
	FifoRX_Usage : out std_logic_vector (8 downto 0);
	FifoRX_Empty : out std_logic;
	FifoRX_AlmostFull : out std_logic;
	FifoRX_Full : out std_logic;
	-- internal FIFO out stats
	FifoOut_Empty : out std_logic;
	FifoOut_Full : out std_logic
  );
  end component;

  component Local_TX is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	-- Comm request of reading stored Data
	Comm_RegistersRdreq : in std_logic;-- local_TX fifo
	Comm_RegistersOut : out std_logic_vector (31 downto 0);-- local_TX fifo

	-- Communication With Local_RX
	LocalRX_Data : in std_logic_vector (31 downto 0);
	LocalRX_Ready: in std_logic;
	LocalRX_Rdreq : out std_logic;

	-- Fifo_TX stats
	RegsFifo_Usage : out std_logic_vector (8 downto 0);
	RegsFifo_Empty : out std_logic;
	RegsFifo_AlmostFull : out std_logic;
	RegsFifo_Full : out std_logic
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
--  signal EvAlmost_full	: std_logic;
--  signal EvEmpty, EvFull : std_logic;
--  signal EvUsedw : std_logic_vector (10 downto 0);
  signal EVDataOutReady : std_logic;
  signal EVFifoStatus : std_logic_vector (15 downto 0);

    -- Registers signals --
  signal Register_Address : natural;
  signal Register_Wrreq : std_logic:='0';
  signal Register_Rdreq : std_logic:='0';
  signal Register_DataValid : std_logic;
  signal Reset_DAQErrors : std_logic;
  signal Register_SelectiveReset : std_logic_vector(N_CONTROL_REGS-1 downto 0);
  signal Register_DataWrite : std_logic_vector(31 downto 0);
  signal Register_DataRead : std_logic_vector(31 downto 0);
  signal Monitor_Registers_Bus : MONITOR_REGS_T:= default_MonRegisters;
  signal Control_Registers_Bus : CONTROL_REGS_T:= default_CtrlRegisters;


  -- From Local_TX --
  signal forLocalRX_Rdreq : std_logic;
  --  signal FifoTX_Full : std_logic;
  signal RegFifoTX_Full : std_logic;
  --  -- From Local_RX --
  signal LocalRX_Data : std_logic_vector(31 downto 0);
  signal LocalRX_Ready: std_logic;
  signal FifoRX_Full : std_logic;
  signal FifoRX_AlmostFull : std_logic;

  signal EB_Busy, Trg_Busy, BusyIn : std_logic;

  signal Reset_Errors : std_logic;

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
	Data_Streams => Data_Streams,
	Data_Valid => Data_Valid,
    EndOfEvent => EndOfEvent
  );

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
    TriggerIn => TriggerIn,

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
	To_State => To_State, -- for the time being --Control_Registers_Bus(ToState_Reg)(1 downto 0),
	Empty_Fifo => '0',--Monitor_Registers_Bus(EB_Fifos_Reg)(EBEmpty_Flag),
	ReadingEvent => '0',-- EB_ReadingEvent,
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
    --Trigger signal from GPIO
    Trigger => TriggerCandidate,
    --Internal Busy from EventFifo
    Busy => BusyIn,

    --Inputs
    ClockCounterIn => ClockCounter,

	-- Outputs
    --Trigger is sent to the inner part of the machine if DAQ is Running and the event builder is not busy
    Internal_Trigger => open,  -- is this needed?
    --Busy out passes a stretched version of the Event Builder Busy to GPIO
    Busy_Out => Trg_Busy,
    TSCounter => TSCounter,
    -- Fifos Out
    TrgDataReady => TrgFifoDataReady,
    TrgReadAcknowledge => TrgReadAcknowledge,
    ClockCounterOut => TrgClockCounter,
    triggerCounterOut => TrgReceivedCounter,
    triggerAcceptedOut => TrgAcceptedCounter,
    TrgTSCounter => TrgTSCounter
  );
  BusyIn <= '1' when EB_Busy='1'
                else '0';

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
    DataAvailable => DataAvailable,
	Data_StreamOut => Data_StreamOut,
	Data_ValidOut => Data_ValidOut,
    EndOfEventOut => EndOfEventOut,
	Data_Size => Data_Size,
	DDR3PointerTail => DDR3PointerTail,
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
    EVReadAcknowledge => Comm_EVReadAcknowledge,
	EVFifoStatus => EVFifoStatus,
	-- A memory is Full
	BusyOut => EB_Busy
  );
  DoStore <= InternalDAQIsRunning and Control_Registers_Bus(Config_reg)(DoStore_Flag);

  -- for simulation only
  Comm_EventReady <= EVDataOutReady;

  -- --------------------------------------------
  -- Register file
  -- --------------------------------------------
  Registers : Register_File
  port map(
    Clock => Clock,
    Reset => Reset,
    Address => Register_Address,
    Rdreq => Register_Rdreq,
    Wrreq => Register_Wrreq,
    DataIn => Register_DataWrite,
    Data_ValidOut => Register_DataValid,
    DataOut => Register_DataRead,
    Selective_Reset => Register_SelectiveReset,
    Monitor_RegistersIn => Monitor_Registers_Bus,
    Control_RegistersOut => Control_Registers_Bus,
    Reset_Errors => Reset_Errors,
    Invalid_Address => Monitor_Registers_Bus (Errors_Reg)(InvalidAddress_Flag),
    INOut_Both_Active => Monitor_Registers_Bus (Errors_Reg)(InOutBothActive_Flag)
  );


  -- --------------------------------------------
  -- Register and Event Transmission to HPS
  -- --------------------------------------------
  LocalCommTX : Local_TX
  port map(
    Clock => Clock,
    Reset => Reset,
    Comm_RegistersRdreq => Comm_RegistersRdreq,
    Comm_RegistersOut => Comm_RegistersOut,
    LocalRX_Data => LocalRX_Data,
    LocalRX_Ready => LocalRX_Ready,
	LocalRX_Rdreq => forLocalRX_Rdreq,
    RegsFifo_Usage => Monitor_Registers_Bus (LocalTX_Fifo_Reg) (24 downto 16),
    RegsFifo_Empty => Monitor_Registers_Bus (LocalTX_Fifo_Reg)(TXRegFifoEmpty_Flag),
    RegsFifo_AlmostFull => Monitor_Registers_Bus (LocalTX_Fifo_Reg)(TXRegFifoAlmostFull_Flag) ,
    RegsFifo_Full => RegFifoTX_Full
  );
  -- This is the FIFO control structure to HPS
  Comm_regfifostatus <=   x"000"& '0'
  	& RegFifoTX_Full & Monitor_Registers_Bus (LocalTX_Fifo_Reg)(TXRegFifoAlmostFull_Flag) & Monitor_Registers_Bus (LocalTX_Fifo_Reg)(TXRegFifoEmpty_Flag) &
  	"000000" & RegFifoTX_Full & Monitor_Registers_Bus (LocalTX_Fifo_Reg) (24 downto 16);
  Comm_DataOut <= (others=>'0');

  -- --------------------------------------------
  -- Register Receiver from HPS
  -- --------------------------------------------
   LocalComm_RX : Local_RX
   port map (
    Clock => Clock,
    Reset => Reset,
    Comm_wrreq => Comm_Wrreq,
    Comm_Data => Comm_DataIn,
    DAQ_Config => internalDAQ_config,
    LocalTX_Rdreq => forLocalRX_Rdreq,
    Register_DataValid => Register_DataValid,
    Register_DataRead => Register_DataRead,
    Register_Address => Register_Address,
    Register_Wrreq => Register_Wrreq,
    Register_Rdreq => Register_Rdreq,
    Register_SelectiveReset  => Register_SelectiveReset,
    Register_DataWrite =>  Register_DataWrite,
    -- Data to LocalTX --
    DataOut => LocalRX_Data,
    Data_Ready => LocalRX_Ready,
    -- FIFO stats --
    FifoRX_Usage => Monitor_Registers_Bus(LocalRX_Fifos_Reg)(8 downto 0),
    FifoRX_Empty => Monitor_Registers_Bus(LocalRX_Fifos_Reg)(RXEmpty_Flag),
    FifoRX_AlmostFull => FifoRX_AlmostFull,
    FifoRX_Full => FifoRX_Full,
    FifoOut_Empty => Monitor_Registers_Bus(LocalRX_Fifos_Reg)(RX_outFifo_Empty_Flag),
    FifoOut_Full =>  Monitor_Registers_Bus(LocalRX_Fifos_Reg)(RX_outFifo_Full_Flag)
  );

  Reset_DAQErrors <= Reset_Errors or internalDAQ_Reset;


  -- --------------------------------------------
  -- Sets all the monitor registers
  -- --------------------------------------------

  -- Register assignments
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(DAQ_IsRunning_Flag) <= internalDAQIsRunning;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(DAQ_Reset_Flag)     <= internalDAQ_Reset;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(DAQ_Config_Flag)    <= internalDAQ_Config;
--??  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(ReadingEvent_Flag)  <= EB_ReadingEvent;
  Monitor_Registers_Bus(FSM_StatusSignals_Reg)(2 downto 0) <= To_mystdlogicvector(Status_Bus);
--  Status <= Monitor_Registers_Bus(FSM_StatusSignals_Reg)(2 downto 0);
--??  Monitor_Registers_Bus (ClkCounter_Reg)     <= std_logic_vector(FromEB_ClkCounter);
--??  Monitor_Registers_Bus (LSB_ClkCounter_Reg) <= x"0000" & std_logic_vector(FromEB_LSBClkCounter);
--??  Monitor_Registers_Bus (BCOCounter_Reg)     <= std_logic_vector(FromEB_BCOCounter);
--??  Monitor_Registers_Bus (TriggerCounter_Reg) <= std_logic_vector(FromEB_triggerCounter);

--??  Monitor_Registers_Bus(EB_Fifos_Reg)(EBFull_Flag)            <= EB_FifoFull;
--??  Monitor_Registers_Bus(EB_Fifos_Reg)(EBFull_Flag2)           <= EB_FifoFull;
--??  Monitor_Registers_Bus(EB_Fifos_Reg)(EBMetadataFull_Flag)    <= EB_MetadataFull;
--??  Monitor_Registers_Bus(EB_Fifos_Reg)(EBMetadataFull_Flag2)   <= EB_MetadataFull;
--??  Monitor_Registers_Bus(LocalTX_Fifo_Reg)(TXFull_Flag)        <= FifoTX_Full;
--??  Monitor_Registers_Bus(LocalTX_Fifo_Reg)(TXFull_Flag2)       <= FifoTX_Full;
  Monitor_Registers_Bus(LocalTX_Fifo_Reg)(TXRegFifoFull_Flag) <= RegFifoTX_Full;
  Monitor_Registers_Bus(LocalTX_Fifo_Reg)(TXRegFifoFull_Flag2)<= RegFifoTX_Full;
  Monitor_Registers_Bus(LocalRX_Fifos_Reg)(RXFull_Flag)       <= FifoRX_Full;
  Monitor_Registers_Bus(LocalRX_Fifos_Reg)(RXFull_Flag2)      <= FifoRX_Full;
  Monitor_Registers_Bus(LocalRX_Fifos_Reg)(RXAlmostFull_Flag) <= FifoRX_AlmostFull;
  Monitor_Registers_Bus(Errors_Reg)(31 downto 4)              <= (others=>'0');

  GenADC:   -- copy eight 12 bit values into four 32 bit registers
  for i in 0 to 3 generate
    Monitor_Registers_Bus(ADC_ch0_reg+i)(11 downto 0)  <= adc_raw_values(i+i);
    Monitor_Registers_Bus(ADC_ch0_reg+i)(23 downto 12) <= adc_raw_values(i+i+1);
  end generate;
  --
  -- Event Builder data to regs
  Monitor_Registers_Bus(EvHeader_Reg)       <= EVFifoStatus & Control_Registers_Bus(VariableHeader_Reg)(15 downto 0);
  Monitor_Registers_Bus(EvTSCnt_Reg)        <= EvDataOut(31 downto 0);
  Monitor_Registers_Bus(EvTrgAccCnt_Reg)    <= EvDataOut(63 downto 32);
  Monitor_Registers_Bus(EvTrgRecCnt_Reg)    <= EvDataOut(95 downto 64);
  Monitor_Registers_Bus(EvTrgMSBClkCnt_Reg) <= EvDataOut(143 downto 112);
  Monitor_Registers_Bus(EvTrgLSBClkCnt_Reg) <= EvDataOut(159 downto 144) & EvDataOut(111 downto 96); -- size and LSB clock counter
  Monitor_Registers_Bus(EvDDR3PtrMin_Reg)   <= EvDataOut(191 downto 160);
  Monitor_Registers_Bus(EvDDR3PtrMax_Reg)   <= EvDataOut(223 downto 192);


--?? To be finished
  Reset_Errors <= '0';

end architecture;
