library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;


--
-- EventBuilder Entity
--
-- 1. It stores in the DDR3 RAM data from sensors
-- 2. For each Frame stores the frame tags (ClockCounter, DataSize and DDR3Pointer) in a DPRam 
-- 3. it gets a Trigger with a TS and it provides the min and max of ClockCounter for frames to be 
--    associated with the Trigger
-- 4. It gets the DDR3Pointer for the frames with a ClockCounter value 
--    INSIDE the provided range. 
--    Note: DDR3PointerMin points to the first entry, 
--          DDR3PointerMax points to the value AFTER the last valid entry
--    Note2: DDR3PointerMax might point after the valid memory range. In this case, wrap it up!
-- 5. Stores the Min and Max Pointers into an EventFIFO.
-- 6. It provides the data of the first entry in the EventFifo to be read outside (readAhead fifo)


entity EventBuilder is
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
end entity;


architecture algorithm of EventBuilder is
 
  component FourFrameReceiver is
  --Port declaration
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	-- 
	SensorEnableIn : in std_logic_vector(N_sensors-1 downto 0);
    -- store or not a frame?
	DoStore : in std_logic;

    --Input stream
	Data_StreamIn : in NSensStreamData_t;  
	Data_ValidIn : in std_logic_vector(N_sensors-1 downto 0);
    EndOfEventIn : in std_logic_vector(N_sensors-1 downto 0);
	-- Time tags
	TSClock : in unsigned(31 downto 0);
	ClockCounter : in unsigned(47 downto 0);
    -- 
	SendPacket : in std_logic;
	WaitRequest : in std_logic;
	-- Outputs
	-- Stream
    DataAvailable : out std_logic;
	Data_StreamOut : out unsigned (31 downto 0);  
	Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
	Data_Size : out unsigned (11 downto 0);  
    -- Time tags of the first word
	ClockCounterOut : out unsigned(47 downto 0);
	FifoFull : out std_logic_vector(N_sensors-1 downto 0)
  );
  end component;
 
 component FramePointerMemory is
  port (
    Clock : in std_logic;
	Reset : in std_logic;
	
    -- Interface with FourFrameReceiver
	-- Writing Frame tags (ClockCounter, DataSize and DDR3Pointer) to Fifo
	MemoryEmpty : out std_logic;
	MemoryFull  : out std_logic;
    ClockCounterIn : in unsigned (47 downto 0);   	
	Data_SizeIn : in unsigned (11 downto 0);  
    DDR3Pointer : in unsigned(31 downto 0);
	WriteRequest : in std_logic;
    DDR3PointerMinRam : out unsigned(31 downto 0);
    -- Interface with TriggerControl
    -- Compare with the trigger ClockCounter (-preTrigger +afterTrigger)
	EvaluateRange : in std_logic;
    ClockCounterMin : in unsigned (47 downto 0);   	
    ClockCounterMax : in unsigned (47 downto 0);   	
	RangeReady : out std_logic;
    DDR3PointerMin : out unsigned(31 downto 0);
    DDR3PointerMax : out unsigned(31 downto 0);
	RangeSize : out unsigned (15 downto 0)
  );	
  end component;
 
 component EventFifo
	port (
		aclr		: in std_logic ;
		clock		: in std_logic ;
		data		: in std_logic_vector (223 downto 0);
		rdreq		: in std_logic ;
		wrreq		: in std_logic ;
		almost_full		: out std_logic ;
		empty		: out std_logic ;
		full		: out std_logic ;
		q		: out std_logic_vector (223 downto 0);
		usedw		: out std_logic_vector (10 downto 0)
	);
  end component;

  signal EndOfEventInt : std_logic;
  signal Data_SizeInt : unsigned (11 downto 0);  
  signal ClockCounterOut : unsigned(47 downto 0);
  signal RangeReadyInt : std_logic;
  signal EvaluateRange : std_logic;
  
  signal ClockCounterMin : unsigned (47 downto 0);   	
  signal ClockCounterMax : unsigned (47 downto 0);   	
  signal ClockCounterDelayed : unsigned (47 downto 0);   	
  signal RangeSize : unsigned (15 downto 0);
  signal DDR3PointerMin, DDR3PointerMax : unsigned(31 downto 0);
  signal DDR3PointerMinRam : unsigned(31 downto 0);

  type state is (idle, waiting, reading, ending1, ending2, ending3);
  signal currentState, nextState : state := idle;
  signal EndOfWait : std_logic;
  constant WaitClockCounter : unsigned(47 downto 0):= x"0000_0000_0400";

  signal streamFifoFull : std_logic_vector(N_sensors-1 downto 0); 
  constant nullStreamFifoFull : std_logic_vector(N_sensors-1 downto 0) := (others=>'0'); 
  signal ORstreamFifoFull : std_logic;
  
  signal EvDataIn, EvDataOutInt : std_logic_vector (223 downto 0);
  signal EvFull, EvAlmost_full, EvEmpty : std_logic;
  signal EvUsedw : std_logic_vector (10 downto 0);
  signal MemoryEmpty, MemoryFull : std_logic;
	

begin

  -- --------------------------
  --  DUT2: Four Frame Receiver 
  -- --------------------------  
  FR: FourFrameReceiver 
  --Port declaration
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
	--
	SensorEnableIn => SensorEnableIn,
    -- store or not a frame?
	DoStore => DoStore,
    --Input stream
	Data_StreamIn => Data_Streams,
	Data_ValidIn => Data_Valid,
    EndOfEventIn => EndOfEvent,
	-- Time tags
	TSClock => TSCounter,
	ClockCounter => ClockCounter,
    -- 
	SendPacket => SendPacket,
    WaitRequest => WaitRequest,
	-- Outputs
	-- Stream
    DataAvailable => DataAvailable,
	Data_StreamOut => Data_StreamOut,  
	Data_ValidOut => Data_ValidOut,
    EndOfEventOut => EndOfEventInt,
	Data_Size  => Data_SizeInt,  
    -- Time tags of the first word
	ClockCounterOut  => ClockCounterOut,
	FifoFull  => streamFifoFull                 
  );
  EndOfEventOut <= EndOfEventInt;
  Data_Size <= Data_SizeInt;
  ORstreamFifoFull <= '0' when streamFifoFull = nullStreamFifoFull
                          else '1';

  FPM: FramePointerMemory 
  port map (
    Clock => Clock,
	Reset => Reset,
	-- Writing to Fifo
	MemoryEmpty => MemoryEmpty,
	MemoryFull  => MemoryFull,
    ClockCounterIn => ClockCounterOut,	
	Data_SizeIn => Data_SizeInt,  
    DDR3Pointer => DDR3PointerHead,
	WriteRequest => SendPacket, 
    DDR3PointerMinRam => DDR3PointerMinRam,
    -- Compare with the trigger
	EvaluateRange => EvaluateRange,
    ClockCounterMin => ClockCounterMin,	
    ClockCounterMax => ClockCounterMax,
	RangeReady => RangeReadyInt,
    DDR3PointerMin => DDR3PointerMin,
    DDR3PointerMax => DDR3PointerMax,
	RangeSize => RangeSize
  );

  ------------------------------------ Process INPUT
  -- FSM -- evaluate the next state
  
  process(currentState, FifoDataReady, EndOfWait, RangeReadyInt)
  begin
    case currentState is 
      when idle => 
        if FifoDataReady = '1' then     
          nextState <= waiting; 
		else
 		  nextState <= idle; 
        end if;
		  
	  when waiting =>
        if EndOfWait='1' then
          nextState <= reading;
		else
          nextState <= waiting;
		end if;

	  when reading =>
        if RangeReadyInt='1' then
          nextState <= ending1;
		else
          nextState <= reading;
		end if;
		
	  when ending1 =>                                      
        nextState <= ending2;
		
	  when ending2 =>                                      
        nextState <= ending3;

	  when ending3 =>                                      
        nextState <= idle;

	  when others =>                                      
        nextState <= idle;
		  
    end case;
  end process;

 -- FSM Sequential part
  
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  currentState<=idle;
      EndOfWait <= '0';
	  TrgReadAcknowledge <= '0';
	  EvaluateRange <= '0';
	  ClockCounterDelayed <= (others=>'0');
	  EVDataOutReady <= '0';
	  
	elsif rising_edge(Clock) then

      currentState <= nextState;

      EndOfWait <= '0';
	  TrgReadAcknowledge <= '0';
      EvaluateRange <= '0';
	  if currentState = idle then
	    if FifoDataReady='1' then
		  ClockCounterMin <= TrgClockCounter-PreTriggerClockTicks;
		  ClockCounterMax <= TrgClockCounter+PostTriggerClockTicks;
		  ClockCounterDelayed <= TrgClockCounter+WaitingClockTicks;
  	    end if;
	  elsif currentState = waiting then
        if ClockCounterDelayed < ClockCounter then
          EndOfWait <= '1';
        end if;		  
	  elsif currentState = reading then
        EvaluateRange <= '1';		
	  elsif currentState = ending1 then
	    TrgReadAcknowledge <= '1';
	  end if;

      EVDataOutReady <= not EvEmpty;

	end if;
  end process;


  -- --------------------------------------------
  -- Event FIFO   (Read ahead; one entry per event - 224 bits)
  -- --------------------------------------------
  EVDataIn <= std_logic_vector(DDR3PointerMax) & std_logic_vector(DDR3PointerMin) & std_logic_vector(RangeSize) & std_logic_vector(TrgClockCounter) & 
              std_logic_vector(TrgReceivedCounter) & std_logic_vector(TrgAcceptedCounter) & std_logic_vector(TrgTSCounter); 
  EVFifo: eventfifo
  port map (
    aclr	=> DAQ_Reset,
    clock	=> Clock,
    wrreq	=> RangeReadyInt,
    data	=> EVDataIn,
    rdreq	=> EVReadAcknowledge, -- actually a read acknowledge!!
    q		=> EVDataOutInt,
    almost_full => EvAlmost_full,
    empty	=> EvEmpty,
    full    => EvFull,
    usedw	=> EvUsedw
  );
  EVDataOut    <= EVDataOutInt;  
  EVFifoStatus <= EvFull & EvAlmost_full & EvEmpty & '0' & EvFull & EvUsedw;
  
  BusyOut <= '1' when EvAlmost_full='1' or MemoryFull='1' or ORstreamFifoFull='1'
                 else '0';
  

  -- --------------------------------------------
  -- Define DDR3PointerTail
  -- --------------------------------------------
  -- when running equal to the min of the current event out
  -- when no events equal to the min of the DPRam
  --
  process(Clock, Reset, DAQ_Reset)
  begin
    if Reset = '1' or DAQ_Reset='1' then
	  DDR3PointerTail <= DDR3PointerHead;
	  
	elsif rising_edge(Clock) then
      if EvEmpty='0' then
	    DDR3PointerTail <= unsigned(EvDataOutInt(191 downto 160));
      else
        DDR3PointerTail <= DDR3PointerMinRam;	
      end if;		
    end if;
 
  end process;
  
end architecture;