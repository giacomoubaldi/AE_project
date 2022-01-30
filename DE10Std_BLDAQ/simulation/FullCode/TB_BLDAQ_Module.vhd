library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

--************************************************************************
--* @short Tester of the FourFrameReceiver
--************************************************************************
--/
entity TB is
end entity;

architecture bench of TB is

  component BLDAQ_Module is
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
    iofifo_datain         : in   std_logic_vector(31 downto 0);                    -- datain
    iofifo_writereq       : in   std_logic;                                        -- writereq
    iofifo_instatus       : out    std_logic_vector(31 downto 0) := (others => 'X'); -- instatus
    iofifo_dataout        : out    std_logic_vector(31 downto 0) := (others => 'X'); -- dataout
    iofifo_readack        : in   std_logic;                                        -- readack
    iofifo_outstatus      : out    std_logic_vector(31 downto 0) := (others => 'X'); -- outstatus
    iofifo_regdataout     : out   std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
    iofifo_regreadack     : in   std_logic;                                        -- regreadack
    iofifo_regoutstatus   : out    std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus
	--
	leds : out std_logic_vector(5 downto 0);
	-- For simulation only!!
	Comm_EventReady : out std_logic
  );
  end component;

  component DataTransfer is
  port (
    Clock : in std_logic;
	Reset : in std_logic;
    -- I/O to Event Builder 
	SendPacket : out std_logic;
    DataAvailable : in std_logic;
	Data_Stream : in unsigned (31 downto 0);  
	Data_Valid : in std_logic;
    EndOfEvent : in std_logic;
    -- I/O to DDR3
    data_ack    : in std_logic;
    data_event  : out std_logic_vector(31 downto 0);
    data_empty  : out std_logic;
	--
	Fifo_info : out std_logic_vector(16 downto 0)
  );
  end component;
  
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
  signal ExtData_Streams : NSensStreamData_t;  
  signal ExtData_Valid : std_logic_vector(N_Sensors-1 downto 0);
  signal ExtEndOfEvent : std_logic_vector(N_Sensors-1 downto 0); 

  component TimeStampGenerator is  
  port (
    Clock : in std_logic;
    Reset : in std_logic;
	DAQRunning : in std_logic;
    TSReset    : out std_logic;
    TSClock    : out std_logic
  );
  end component;
  signal GenTSReset, GenTSClock : std_logic;

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
    Clock: in std_logic;
    Reset: in std_logic;
    Count : out unsigned (nbit-1 downto 0)
  );
  end component;
  
  signal Clock  : std_logic;
  signal nReset, Reset : std_logic;
  type corestatus is (idle, config, run, stop);
  signal status : corestatus := idle;

  type state is (idle, waiting, reading, ending);
  signal currentState, nextState : state := idle;
  signal FSM : std_logic_vector (1 downto 0);
  
  -------------------------------------------------
    -- Trigger sources
  signal randombits : std_logic_vector(15 downto 0) :=(others=>'0');
  signal Trigger : std_logic := '0';
  signal Busy_Out : std_logic;   
  --
  -- BLDAQ_Module
  signal BCOClock, nBCOReset, TSReset, TSClock : std_logic;
    --Trigger signal from GPIO
	-- ADC --
  signal adc_raw_values : adc_values_t;

  signal DoRead, DoStore : std_logic := '0';
  signal Comm_EventReady : std_logic;
  signal fifo_readack, fifo_empty : std_logic := '0';
  signal Data_ToRAM : std_logic_vector (31 downto 0);  
	-- I/O to RAM Manager 
  signal SendPacket : std_logic;
  signal WaitRequest : std_logic;
  signal DataAvailable : std_logic;

  signal Data_StreamOut :  unsigned (31 downto 0);  
  signal Data_ValidOut :  std_logic;
  signal EndOfEventOut :  std_logic;
  signal Data_Size :  unsigned (11 downto 0);  
  signal DDR3PointerHead, DDR3PointerTail, NextDDR3PointerHead : unsigned(31 downto 0);
  signal RamEntries : unsigned(31 downto 0);
  constant RamSize : unsigned(31 downto 0) := x"00010000";
	-- Communication with HPS control
	--
  signal ClockCounter : unsigned(47 downto 0);
   signal leds : std_logic_vector(5 downto 0);

  -- for fifo IO
  signal iofifo_datain    :  std_logic_vector(31 downto 0);                       -- datain
  signal iofifo_writereq  :  std_logic;                                           -- writereq
  signal iofifo_instatus  :  std_logic_vector(31 downto 0) := (others => 'X');    -- instatus
  signal iofifo_dataout   :  std_logic_vector(31 downto 0) := (others => 'X');    -- dataout
  signal iofifo_readack   :  std_logic;                                           -- readack
  signal iofifo_outstatus :  std_logic_vector(31 downto 0) := (others => 'X');    -- outstatus
  signal iofifo_regdataout   :  std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
  signal iofifo_regreadack   :  std_logic;                                        -- regreadack
  signal iofifo_regoutstatus :  std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus 
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
    clock => Clock,
    reset => Reset,
    count => ClockCounter
  );

  TSGen: TimeStampGenerator 
  port map (
    Clock => Clock, Reset => Reset,
	DAQRunning => leds(5),
    TSReset => TSReset,
    TSClock => TSClock
  );
  
  -- --------------------------------------------
  --   DUT: BusyLessDAQ_Module 
  -- --------------------------------------------
  DUT: BLDAQ_Module
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
    TSClockIn => TSClock,     
    TSResetIn => TSReset, 
    --Trigger signal from GPIO
    TriggerIn => Trigger,
	Busy_Out => Busy_Out,
	-- ADC --
	adc_raw_values => adc_raw_values,
		-- External data IN
	ExtData_Streams => ExtData_Streams,
	ExtData_Valid => ExtData_Valid,
    ExtEndOfEvent => ExtEndOfEvent,
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
	-- Communication with HPS control
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
	leds => leds,
	Comm_EventReady => Comm_EventReady
  );
 -- -----------------------------------------------------------
  --   Transfer of data from Event builder to DDR3
  -- -----------------------------------------------------------

  DT: DataTransfer 
  port map (
    Clock => Clock,
	Reset => Reset,
    -- I/O to Event Builder 
	SendPacket => SendPacket,
    DataAvailable => DataAvailable,
	Data_Stream => Data_StreamOut, 
	Data_Valid => Data_ValidOut,
    EndOfEvent => EndOfEventOut,
    -- I/O to DDR3
    data_ack    => fifo_readack,
    data_event  => Data_ToRAM,
    data_empty  => fifo_empty,
	--
	Fifo_info => open
  );

  -- -----------------------------------------------------------
  --   Detector simulator... to be replaced with real detectors!
  -- -----------------------------------------------------------
  DS: DetectorSimulator
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
    --Outputs
	Data_Streams => ExtData_Streams,
	Data_Valid => ExtData_Valid,
    EndOfEvent => ExtEndOfEvent	 
  );
  
  
------------------------------------ Process INPUT
  -- FSM -- evaluate the next state
  
  process(currentState, DoStore, EndOfEventOut )
  begin
	SendPacket <= '0';
    case currentState is 
      when idle => 
        if DoStore = '1' then     
          nextState <= reading; 
	      SendPacket <= '1';
		else
 		  nextState <= idle; 
        end if;

	  when reading =>
        if EndOfEventOut='1' then
          nextState <= ending;
	      SendPacket <= '0';
		else
          nextState <= reading;
		end if;
		
	  when ending =>                                      
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
	  DDR3PointerHead <= (others=>'0');	
      NextDDR3PointerHead <= (others=>'0');	  
	  WaitRequest <= '0';
	  doStore <= '0';

	elsif rising_edge(Clock) then
      currentState <= nextState;
	  if currentState = idle then
	    doStore <= '0';
	    if DataAvailable='1' and ramEntries+Data_Size<RamSize then
          DoStore <= '1';
		  if unsigned('0'& DDR3PointerHead(30 downto 0))+Data_Size>RamSize then
		    NextDDR3PointerHead(31) <= not DDR3PointerHead(31);
		    NextDDR3PointerHead(30 downto 0) <=  DDR3PointerHead(30 downto 0)+Data_Size-RamSize(30 downto 0);
          else
		    NextDDR3PointerHead(31) <= DDR3PointerHead(31);
		    NextDDR3PointerHead(30 downto 0) <=  DDR3PointerHead(30 downto 0)+Data_Size;		  
		  end if;
		end if;
      elsif currentState = ending then
        DDR3PointerHead <= NextDDR3PointerHead;
	    doStore <= '0';
	  end if;
    end if;
  end process;

  ramEntries <= unsigned('0' & (DDR3PointerHead(30 downto 0)-DDR3PointerTail(30 downto 0))) when DDR3PointerHead(31)=DDR3PointerTail(31)
                else unsigned('0' & (DDR3PointerHead(30 downto 0)-DDR3PointerTail(30 downto 0)))+RamSize;

  adc_raw_values <= (others=>(others=>'0'));

  -- FSM
  

  stimulus: process(Clock) is
    variable counter: natural := 0;
 begin
    if nReset='0' then
	  counter := 0;
 	  status <= idle;
      FSM <= "11";
    elsif( rising_edge(Clock) ) then

      case status is
        when idle =>
          FSM <= "11";
 		  if counter>20 then
   		    status <= config;
            counter := 0;
		  end if;
        when config =>
          FSM <= "00";
 		  if counter>2000 then
   		    status <= run;
            counter := 0;
		  end if;
        when run =>
          FSM <= "01";
 		  if counter>200000 then
   		    status <= stop;
            counter := 0;
		  end if;
        when stop =>
          FSM <= "10";
 		  if counter>20 then
   		    status <= idle;
            counter := 0;
		  end if;
      end case;
      counter := counter+1;
      --
    end if;
  end process;
 
-----------------------------------------
-- define random Trigger
-----------------------------------------

  stimulus2: process(Clock, Reset) is
    variable counter : natural := 0;
 begin
    if Reset='1' then
	  counter := 200;
      Trigger <= '0';
    elsif( rising_edge(Clock) ) then
	    if status=run then
		  if counter=0 then
		    Trigger <= '1';
		    counter := 13+to_integer(unsigned(randombits(8 downto 0)));
		  else
		    Trigger <= '0';
		    counter := counter-1;
		  end if;
		end if;
      --
    end if;
  end process;
  
  
    -- find a new 16 bits random value
  rnd: process( clock, Reset) is
    variable randomnumber : std_logic_vector(15 downto 0) :=  x"C0A1";
  begin
	if(Reset = '1') then
      randombits <= (others=>'0');
	  randomnumber :=  x"C0A1";
	elsif rising_edge(Clock) then
      randombits <= randomnumber;  -- main process output
  	  randomnumber :=   randomnumber(14 downto 10)
                          & (not randomnumber(15) xor randomnumber(9))
                          &       randomnumber(8 downto 5)
                          & (not randomnumber(15) xor randomnumber(4))
                          & randomnumber(3 downto 2)
                          & (not randomnumber(15) xor randomnumber(1))
                          & randomnumber(0)
                          & (not randomnumber(15) xor randomnumber(2)); 
    end if;
  end process;	


-- process output

    -- -- find a new 16 bits random value
  -- myout: process( clock, Reset) is
    -- variable counter : natural :=  100;
  -- begin
	-- if Reset = '1' then
      -- counter := 100;
-- --	  Comm_EVReadAcknowledge <= '0';
	-- elsif rising_edge(Clock) then
-- --      Comm_EVReadAcknowledge <= '0';		
      -- if counter=0 then
	    -- if Comm_EventReady='1' then
-- --		  Comm_EVReadAcknowledge <= '1';
		-- end if;
		-- counter := to_integer(unsigned(randombits(9 downto 0)))+30;
	  -- else 
	    -- counter := counter-1;
	  -- end if;
	-- end if;
  -- end process;


end architecture bench;
 
