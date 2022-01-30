library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

--************************************************************************
--* @short Tester of the Trigger_Control and of the FramePointerDPRam
--************************************************************************
--/
entity TB is
end entity;

architecture bench of TB is

  component FramePointerMemory is
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	-- Writing to Fifo
	MemoryEmpty : out std_logic;
	MemoryFull  : out std_logic;
    ClockCounterIn : in unsigned (47 downto 0);   	
	Data_SizeIn : in unsigned (11 downto 0);  
    DDR3Pointer : in unsigned(31 downto 0);
	WriteRequest : in std_logic;
	-- Reading from Fifo -- is this needed?
--    ClockCounterOut : out unsigned (47 downto 0);   	
    --	Data_SizeOut : out unsigned (11 downto 0);  
    --  DDR3PointerOut : out unsigned(31 downto 0);
--	ReadAcknowledge : in std_logic;

    -- Compare with the trigger
	EvaluateRange : in std_logic;
    ClockCounterMin : in unsigned (47 downto 0);   	
    ClockCounterMax : in unsigned (47 downto 0);   	
	RangeReady : out std_logic;
    DDR3PointerMin : out unsigned(31 downto 0);
    DDR3PointerMax : out unsigned(31 downto 0);
	RangeSize : out unsigned (15 downto 0)
  );	
  end component;

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

    -- Fifos Out
    FifoDataValid : out std_logic;
    FifoDataAcknowledge : in std_logic;
    ClockCounterOut : out unsigned (47 downto 0);   	
    triggerCounterOut : out unsigned (31 downto 0);  
    triggerAcceptedOut : out unsigned (31 downto 0);  
    TSCounterOut : out unsigned (31 downto 0)
  );
  end component;

  component clockgen is
    generic (CLKPERIOD : time; RESETLENGTH : time);
    port(
      Clock : out std_logic;
      nReset : out std_logic);
  end component;

  component RisingEdge_Counter 
    generic ( NBITS: natural := 32);
    port(
      clock    : in std_logic;
      reset    : in std_logic;
      eventToCount    : in std_logic;   
      --Output 
      counts : out unsigned (NBITS-1 downto 0)
    );
  end Component;

  component Counter_nbit is
  generic ( nbit : natural := 16);
  port(
    CLK: in std_logic;
    RESET: in std_logic;
    COUNT : out unsigned (nbit-1 downto 0)
  );
  end component;
  
  component Random_generator is
  generic( seed : std_logic_vector(15 downto 0) :=  x"C0A1" );
  port (
    Clock : in std_logic;
	Reset : in std_logic;
	Enable : in std_logic;
	 
	Random_Data : out unsigned (15 downto 0)
  );
  end component;
  
  signal Clock  : std_logic;
  signal nReset, Reset : std_logic;

  signal BCOClock, nBCOReset, BCOReset : std_logic;
--  signal TSCounter : unsigned(31 downto 0);
  --
  -- Detector simulator data  
  signal ClockCounter : unsigned(47 downto 0);
  -- Trigger Control signals
  signal BusyIn, BusyOut : std_logic;
  signal TriggerIn, TriggerOut : std_logic;
  -- 
  signal FifoDataValid : std_logic;
  signal FifoDataAcknowledge : std_logic;
  signal ClockCounterOut : unsigned (47 downto 0);   	
  signal triggerCounterOut : unsigned (31 downto 0);  
  signal triggerAcceptedOut : unsigned (31 downto 0);  
  signal TSCounterOut : unsigned (31 downto 0);
  --
-- For FramePointerMemory
	-- Writing to Fifo
  signal MemoryEmpty : std_logic;
  signal MemoryFull  : std_logic;
  signal ClockCounterIn : unsigned (47 downto 0);   	
  signal Data_SizeIn : unsigned (11 downto 0);  
  signal DDR3Pointer : unsigned(31 downto 0);
  signal WriteRequest : std_logic;
  -- for generation of random Clock counter
  signal RCCRandom_Data : unsigned (15 downto 0);
  signal enableRCCGen : std_logic;
  signal ClockCounterGen : unsigned (47 downto 0);   	
  signal ClockCounterDelayed : unsigned (47 downto 0);   	
  signal RCCounter : unsigned (15 downto 0);
  
  -- for trigger generator
  signal TrgRandom_Data : unsigned (15 downto 0);
  signal TrgCounter : unsigned (15 downto 0);
  signal enableTrgGen : std_logic;

    -- Compare with the trigger
  signal EvaluateRange : std_logic;
  signal ClockCounterMin : unsigned(47 downto 0);   	
  signal ClockCounterMax : unsigned(47 downto 0);   	
  signal RangeReady : std_logic;
  signal DDR3PointerMin : unsigned(31 downto 0);
  signal DDR3PointerMax : unsigned(31 downto 0);
  signal RangeSize : unsigned (15 downto 0);

  signal DAQIsRunning : std_logic;
  type state is (idle, waiting, reading, ending1, ending2, ending3);
  signal currentState, nextState : state := idle;
  signal EndOfWait : std_logic;
  constant WaitClockCounter : unsigned(47 downto 0):= x"0000_0000_0400";
  signal errors : std_logic_vector(5 downto 0) := (others=>'0');
begin

	-- --------------------------
    --  Clocks and Counters
    -- --------------------------  
  mainClock: clockgen
    generic map (CLKPERIOD => 20 ns, -- 50 MHz clock
                 RESETLENGTH => 240 ns)
    port map ( Clock => Clock, nReset => nReset);
  Reset <= not nReset;

  BXCounter : Counter_nbit
  generic map( nbit => 48)
  port map(                                        
    clk => Clock,
    reset => Reset,
    count => ClockCounter
  );

  BCO_Clock: clockgen
    generic map (CLKPERIOD => 1 us, -- 1 MHz clock
                 RESETLENGTH => 750 ns)
    port map ( Clock => BCOClock, nReset => nBCOReset);
  BCOReset <= not nBCOReset;
  DAQIsRunning <= nBCOReset;
  --BCO_Counter--
  --Counts the number of BCOClocks
  -- BCO_Counter : RisingEdge_Counter
  -- port map(                                        
    -- clock => Clock,
    -- reset => BCOReset,
    -- eventToCount => BCOClock,
    -- counts => TSCounter
  -- );
  
  -- --------------------------------------------
  --   DUT1: Trigger_Control simulator... 
  -- --------------------------------------------
  TC: Trigger_Control
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
	
    DAQIsRunning => nBCOReset, -- for the time being  
    --Resets Counters preparing for run 
    DAQ_Reset => BCOReset,
    --External clock 
    TSClock => BCOClock,      
    TSReset => BCOReset,
    --Trigger signal from GPIO
    Trigger => TriggerIn,
    --Internal Busy from Event Builder
    Busy => BusyIn, -- for the time being
     
    --Inputs
    --ClkCounter counts the number of clocks since DAQIsRunning passed from 0 to 1.
    ClockCounterIn => ClockCounter,
    --Trigger is sent to the inner part of the machine if DAQ is Running and the event builder is not busy
    Internal_Trigger => TriggerOut, -- is this needed?
    --Busy out passes a stretched version of the Event Builder Busy to GPIO 
    Busy_Out => BusyOut,

    -- Fifos Out
    FifoDataValid => FifoDataValid,
    FifoDataAcknowledge => FifoDataAcknowledge, 
    ClockCounterOut => ClockCounterOut,
    triggerCounterOut => triggerCounterOut, 
    triggerAcceptedOut => triggerAcceptedOut,
    TSCounterOut => TSCounterOut
  );

  BusyIn <= '0'; -- BusyOut; -- Ha senso??
  
  -- --------------------------
  --  DUT2: FramePointerMemory
  -- --------------------------  
  FPM: FramePointerMemory 
  --Port declaration
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
	--
	-- Writing to Fifo
	MemoryEmpty => MemoryEmpty,
	MemoryFull  => MemoryFull,
    ClockCounterIn => ClockCounterIn,   	
	Data_SizeIn => Data_SizeIn, 
    DDR3Pointer => DDR3Pointer,
	WriteRequest => WriteRequest,
	-- Reading from Fifo -- is this needed?
--    ClockCounterOut : out unsigned (47 downto 0);   	
    --	Data_SizeOut : out unsigned (11 downto 0);  
    --  DDR3PointerOut : out unsigned(31 downto 0);
--	ReadAcknowledge : in std_logic;

    -- Compare with the trigger
	EvaluateRange => EvaluateRange,
    ClockCounterMin => ClockCounterMin, 	
    ClockCounterMax => ClockCounterMax,
	RangeReady => RangeReady,
    DDR3PointerMin => DDR3PointerMin,
    DDR3PointerMax => DDR3PointerMax,
	RangeSize => RangeSize
  );	

  -- a number to generate a delay for the next trigger
  RG1: Random_generator 
  generic map( seed => x"C0A1" )
  port map (
    Clock => Clock,
	Reset => Reset,
	Enable => enableTrgGen,
	Random_Data => TrgRandom_Data
  );
  enableTrgGen <= '1' when TrgCounter<x"0003" else '0';
  
  -- Generate a random trigger
  process(Clock, Reset) is
    variable mask : unsigned(15 downto 0) := x"0fff";
  begin 
    if Reset='1' then
	  TriggerIn <= '0';
	  TrgCounter <= x"2000";
	elsif rising_edge(Clock) then
	  TriggerIn <= '0';
      if DAQIsRunning='1' then
        if not(TrgCounter=x"0000") then
		  TrgCounter <= TrgCounter-1;
		else
		  TrgCounter <= (TrgRandom_Data and mask)+x"0014";
		  TriggerIn <= '1';
		end if;
	  end if;
	end if;
  end process;	


 -- a number to generate a delay for the Frame info
  RG2: Random_generator 
  generic map( seed => x"C0A1" )
  port map (
    Clock => Clock,
	Reset => Reset,
	Enable => enableRCCGen,
	Random_Data => RCCRandom_Data
  );
  enableRCCGen <= '1' when RCCounter<x"0003" else '0';
  
  -- Generate a random ClockCounter
  process(Clock, Reset) is
    variable mask : unsigned(15 downto 0) := x"00ff";
  begin 
    if Reset='1' then
	  RCCounter <= x"1000";
	  WriteRequest <= '0';
	elsif rising_edge(Clock) then
	  WriteRequest <= '0';
      if DAQIsRunning='1' then
        if not(RCCounter=x"0000") then
		  RCCounter <= RCCounter-1;
		else
		  RCCounter <= (RCCRandom_Data and mask)+x"0014";
		  ClockCounterGen <= ClockCounter-1000;
		  WriteRequest <= '1';
		end if;
	  end if;

	end if;
  end process;	
  
  ClockCounterIn <= ClockCounterGen;


 -- Update DDR3Pointer
  process(Clock, Reset) is
    variable WriteRequestDelayed : std_logic_vector(3 downto 0);
  begin 
    if Reset='1' then
	  DDR3Pointer <= (others=>'0');
	  Data_SizeIn <= x"020"; -- fixed number for the first size!
      WriteRequestDelayed := "0000";
	elsif rising_edge(Clock) then
      if WriteRequestDelayed(3)='1' then
        DDR3Pointer <= DDR3Pointer+Data_SizeIn;
        Data_SizeIn <= x"020"; -- for the first tests!!  TrgRandom_Data(11 downto 0)+20; -- recycle a random variable!
	  end if;
	  WriteRequestDelayed := WriteRequestDelayed(2 downto 0) & WriteRequest;
	end if;
  end process;	

------------------------------------ Process INPUT
  -- FSM -- evaluate the next state
  
  process(currentState, FifoDataValid, EndOfWait, RangeReady)
  begin
    case currentState is 
      when idle => 
        if FifoDataValid = '1' then     
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
        if RangeReady='1' then
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
	  FifoDataAcknowledge <= '0';
	  EvaluateRange <= '0';
	  ClockCounterDelayed <= (others=>'0');
	elsif rising_edge(Clock) then
      currentState <= nextState;
      EndOfWait <= '0';
	  FifoDataAcknowledge <= '0';
      EvaluateRange <= '0';
	  if currentState = idle then
	    if FifoDataValid='1' then
		  ClockCounterMin <= ClockCounterOut-1000;
		  ClockCounterMax <= ClockCounterOut+2000;
		  ClockCounterDelayed <= ClockCounterOut+3000;
  	    end if;
	  elsif currentState = waiting then
        if ClockCounterDelayed < ClockCounter then
          EndOfWait <= '1';
        end if;		  
	  elsif currentState = reading then
	    -- nothing to do??
        EvaluateRange <= '1';		
	  elsif currentState = ending1 then
	    FifoDataAcknowledge <= '1';
	  end if;
	end if;
  end process;


  -- Verification
    -- Verification part
  errors(0) <= '1' when EvaluateRange='1' and MemoryEmpty='1' else '0';
  errors(1) <= '1' when EvaluateRange='1' and  ClockCounterMin>=ClockCounterMax else '0';
  errors(2) <= '1' when RangeReady='1' and DDR3PointerMin>=DDR3PointerMax else '0';
  errors(3) <= '1' when RangeReady='1' and RangeSize=x"0000" else '0';
  errors(4) <= '1' when RangeReady='1' and not(DDR3PointerMin+RangeSize=DDR3PointerMax) else '0';

end architecture bench;
 
