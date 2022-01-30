library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity Trigger_Control is
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

    -- Fifos Out
    TrgDataReady : out std_logic;
    TrgReadAcknowledge : in std_logic;
    ClockCounterOut : out unsigned (47 downto 0);   	
    triggerCounterOut : out unsigned (31 downto 0);  
    triggerAcceptedOut : out unsigned (31 downto 0);  
    TrgTSCounter : out unsigned (31 downto 0)
  );
end entity;


   
----Architecture of the Trigger Control ----
  
architecture Structural of Trigger_Control is

  --Declaration of components--
  
  --Used for TSClockCounter and TriggerCounter
  component RisingEdge_Counter 
    port(
      clock    : in std_logic;
      reset    : in std_logic;
      eventToCount    : in std_logic;   
      --Output 
      counts : out unsigned (31 downto 0)
    );
    end Component;
  --Busy_Stretch--
  --Makes BusyOut='0' if Busy has been '0' for 16 or more clocks
  component Busy_Stretch 
    generic (nbit: natural := 16); 
    port (
      --Inputs
      Clock : in std_logic;
      Reset : in std_logic;
      Busy : in std_logic;
      --Outputs
      Busy_Out : out std_logic
    );
  end component;

  component TrgFifo
	port (
		aclr		: IN STD_LOGIC ;
		clock		: IN STD_LOGIC ;
		data		: IN STD_LOGIC_VECTOR (143 DOWNTO 0);
		rdreq		: IN STD_LOGIC ;
		wrreq		: IN STD_LOGIC ;
		almost_full	: OUT STD_LOGIC ;
		empty		: OUT STD_LOGIC ;
		full		: OUT STD_LOGIC ;
		q		    : OUT STD_LOGIC_VECTOR (143 DOWNTO 0);
		usedw		: OUT STD_LOGIC_VECTOR (7 DOWNTO 0)
	);
  end component;

    
  ----  Internal Signals  ----	 
                        
  signal Internal_Trigger_candidate : std_logic :='0';     
  -- Used if DaqReset acts like Reset  
  signal EveryReset : std_logic;                        
  -- Used if TSReset acts like Reset
  signal past_trigger : std_logic := '0';  
  signal triggerDelayer, triggerDelayerAcc : std_logic_vector(31 downto 0) := (others=>'0');  
  signal BusyM, BusyL : std_logic;
  
  -- fifo signals
  signal dataIn, dataOut : STD_LOGIC_VECTOR (143 DOWNTO 0);
  signal empty, almost_full, full : std_logic;
  signal usedw : std_logic_vector(7 downto 0);
  
  signal trigRecCounter, trigAccCounter : unsigned(31 downto 0);
  signal TSCounterInt : unsigned(31 downto 0);
  
begin            --Structure description--
                                                  
  EveryReset <= Reset or DAQ_Reset;
  
  --Counts the number of main trigger
  -- note: it counts a delayed signal so that it is possible to
  -- store in the event the current trigger number (starting from 0)
  MainTriggerCounter : RisingEdge_Counter 
  port map(                                       
    clock => Clock,
    reset => EveryReset,
    eventToCount =>  triggerDelayer(31),
    counts => trigRecCounter
  );  
  MainTriggerCounterAcc : RisingEdge_Counter 
  port map(                                       
    clock => Clock,
    reset => EveryReset,
    eventToCount =>  triggerDelayerAcc(31),
    counts => trigAccCounter
  );  
                                                  
                                                
  --TS_Counter--
  --Counts the number of TSClocks
  TS_Counter : RisingEdge_Counter
  port map(                                        
    clock => Clock,
    reset => TSReset,
    eventToCount => TSClock,
    counts => TSCounterInt
  );
  TSCounter <= TSCounterInt;
  
   
  Trigger_Main : process (Clock, EveryReset)  
  begin
    if EveryReset = '1' then      
	  triggerDelayer<= (others=>'0');
	  triggerDelayerAcc<= (others=>'0');
      Internal_Trigger_candidate<='0';   
      past_trigger<='0';
    elsif rising_edge(Clock) then   

	  triggerDelayerAcc <=  triggerDelayerAcc(30 downto 0) & Internal_Trigger_candidate; 
      Internal_Trigger_candidate<='0';
      if Trigger = '1' and past_trigger = '0' then
	    triggerDelayer <=  triggerDelayer(30 downto 0) & '1'; 
        if BusyM = '0' and DAQIsRunning = '1' then
          Internal_Trigger_candidate<='1';
        end if;
      else 
	    triggerDelayer <=  triggerDelayer(30 downto 0) & '0'; 
	  end if;
      past_trigger<=Trigger;

    end if;
  end process;
   
  Internal_Trigger <= Internal_Trigger_candidate;
   
  -- prepare a long busy
  BusyM <= '1' when Busy='1' or almost_full='1'
               else '0';
 
  --Busy_Stretcher--
  --Busy_Out = '0' if Busy has been '0' for 16 or more clocks
  Busy_Stretcher : Busy_Stretch
  port map(                                        
    Clock => Clock,
    Reset => Reset,
    Busy => BusyM,
    Busy_Out => BusyL
  );
  Busy_Out <= BusyL; 
   
  Internal_Trigger <= Internal_Trigger_candidate; 

  DataIn <= std_logic_vector(ClockCounterIn & trigRecCounter & trigAccCounter & TSCounterInt);
  
  TrgFif: TrgFifo
	port map (
		aclr  => EveryReset,
		clock => Clock,
		data  => DataIn,	
		rdreq => TrgReadAcknowledge,
		wrreq => Internal_Trigger_Candidate, -- note: the Trigger starts a writing to the fifo
		almost_full	=> almost_full,
		empty		=> empty,
		full		=> full,
		q	=> dataOut,
		usedw		=> usedw
	);

  TrgDataReady <= not empty;
  TrgTSCounter <= unsigned(dataOut(31 downto 0));
  triggerAcceptedOut <= unsigned(dataOut(63 downto 32));
  triggerCounterOut <= unsigned(dataOut(95 downto 64));
  ClockCounterOut <= unsigned(dataOut(143 downto 96));



  -- debugTrg <= TriggerError & Internal_Trigger_candidate & Busy & past_trigger & triggerR & DAQIsRunning & Clock & EveryReset; 
  
  -- -- debug trigger conditions
  
  -- Trigger_Check : process (Clock, EveryReset)  
    -- variable triggerRet,pastTriggerRet, BusyRet,DAQIsRunningRet,TrigcandRet : std_logic_vector(7 downto 0) := (others=>'0');  
	-- variable counter : integer :=0;
  -- begin
    -- if EveryReset = '1' then      
	  -- TriggerError <= '0';
	  -- pastTriggerRet := (others=>'0');
	  -- triggerRet := (others=>'0');
	  -- BusyRet := (others=>'0');
	  -- DAQIsRunningRet := (others=>'0');
	  -- TrigcandRet := (others=>'0');
	  -- counter :=0;
	-- elsif rising_edge(Clock) then   
	
	  -- if( triggerRet(7 downto 6)="01" and Busy='0' ) then   
	    -- TriggerError <='1';
		-- debugreg <= BusyRet(7 downto 0 ) & triggerRet(7 downto 2 ) & pastTriggerRet(7 downto 2 ) &  DAQIsRunningRet(7 downto 2 ) & TrigcandRet(7 downto 2 );
	  -- end if;
	  
      -- triggerRet := triggerRet(6 downto 0 ) & TriggerR;
      -- pastTriggerRet :=  pastTriggerRet(6 downto 0 ) & past_trigger;
	  -- BusyRet := BusyRet(6 downto 0 ) & Busy;
	  -- DAQIsRunningRet := DAQIsRunningRet(6 downto 0 ) & DAQIsRunning;
	  -- TrigcandRet := TrigcandRet(6 downto 0 ) & Internal_Trigger_candidate;	  
	  -- if( counter<100000000 ) then
	    -- counter := counter+1;
		-- TriggerError <='0';
	  -- else
	    -- counter := 0;
	  -- end if;
    -- end if;
  -- end process;
  
end architecture;
     

     