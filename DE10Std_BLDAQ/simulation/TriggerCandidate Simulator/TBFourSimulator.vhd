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

  component DetectorSimulator is
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
  --Trigger : in std_logic;
    --Outputs
	Data_Streams : out NSensStreamData_t;
	Data_Valid : out std_logic_vector(N_Sensors-1 downto 0);
    EndOfEvent : out std_logic_vector(N_Sensors-1 downto 0)
  );
  end component;

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
    CLocK: in std_logic;
    RESET: in std_logic;
    COUNT : out unsigned (nbit-1 downto 0)
  );
  end component;



  signal Clock  : std_logic;
  signal nReset, Reset, Trigger: std_logic;

  signal BCOClock : std_logic;
  signal TSClock : unsigned(31 downto 0);
  --
  -- Detector Simulator

  -- Detector simulator data
  signal NData_Streams : NSensStreamData_t;
  signal NData_Valid : std_logic_vector(N_Sensors-1 downto 0);
  signal NEndOfEvent : std_logic_vector(N_Sensors-1 downto 0);

  signal ClockCounter : unsigned(47 downto 0);
  signal ClockCounterOut : unsigned(47 downto 0);
  signal ShiftedClockCounter : unsigned(47 downto 0);
  signal DataAvailable : std_logic;
  signal SendPacket : std_logic;
  signal WaitRequest : std_logic;

  signal Data_StreamOut :  unsigned (31 downto 0);
  signal Data_ValidOut :  std_logic;
  signal EndOfEventOut :  std_logic;
  signal Data_Size :  unsigned (11 downto 0);
    -- Time tags of the first word
  signal FifoFull : std_logic_vector(N_sensors-1 downto 0);


  -- Check for the Frame with Trigger
signal TriggerCheck : std_logic := '0';
---signal EndOfEvent : std_logic := '0';




  type state is (idle, waiting, reading, ending);
  signal currentState, nextState : state := idle;
  signal EndOfWait : std_logic;
  constant WaitClockCounter : unsigned(47 downto 0):= x"0000_0000_0400";

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

  BCO_Clock: clockgen
    generic map (CLKPERIOD => 1 us, -- 1 MHz clock
                 RESETLENGTH => 750 ns)
    port map ( Clock => BCOClock, nReset => open);

  --BCO_Counter--
  --Counts the number of BCOClocks
  BCO_Counter : RisingEdge_Counter
  port map(
    clock => Clock,
    reset => Reset,
    eventToCount => BCOClock,
    counts => TSClock
  );




  -- --------------------------------------------
  --   DUT1: Detector simulator...
  -- --------------------------------------------
  DUT1: DetectorSimulator
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
  --Trigger => Trigger,
    --Outputs
	Data_Streams => NData_Streams,
	Data_Valid => NData_Valid,
    EndOfEvent => NEndOfEvent
  );

  -- --------------------------
  --  DUT2: Four Frame Receiver
  -- --------------------------
  DUT2: FourFrameReceiver
  --Port declaration
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
	--
	SensorEnableIn => "1111",
    -- store or not a frame?
	DoStore => '1', -- for the time being...
    --Input stream
	Data_StreamIn => NData_Streams,
	Data_ValidIn => NData_Valid,
    EndOfEventIn => NEndOfEvent,
	-- Time tags
	TSClock => TSClock,
	ClockCounter => ClockCounter,
    --
	SendPacket => SendPacket,
  WaitRequest => WaitRequest,
	-- Outputs
	-- Stream
    DataAvailable => DataAvailable,
	Data_StreamOut => Data_StreamOut,
	Data_ValidOut => Data_ValidOut,
    EndOfEventOut => EndOfEventOut,
	Data_Size  => Data_Size,
    -- Time tags of the first word
	ClockCounterOut  => ClockCounterOut,
	FifoFull  => FifoFull
  );


  DUT3: Trigger_Candidate
  port map (
  --Inputs
  Clock => Clock,
  Reset => Reset,

  --Input stream
  Data_Stream => Data_StreamOut,
  Data_Valid => Data_ValidOut,
  --Output
    TriggerCheck  => TriggerCheck

  );





------------------------------------ Process INPUT
  -- FSM -- evaluate the next state

  process(currentState, DataAvailable, EndOfEventOut, EndOfWait)
  begin
	SendPacket <= '0';
    case currentState is
      when idle =>
        if DataAvailable = '1' then
          nextState <= waiting;
		else
 		  nextState <= idle;
        end if;

	  when waiting =>
        if EndOfWait='1' then
	      SendPacket <= '1';
          nextState <= reading;
		else
          nextState <= waiting;
		end if;

	  when reading =>
        if EndOfEventOut='1' then
          nextState <= ending;
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
      EndOfWait <= '0';
	  ShiftedClockCounter <= (others=>'0');
	elsif rising_edge(Clock) then
      currentState <= nextState;
      EndOfWait <= '0';
	  if currentState = idle then
	    if DataAvailable='1' then
	      ShiftedClockCounter <= ClockCounterOut+WaitClockCounter;
		end if;
	  elsif currentState = waiting and ShiftedClockCounter < ClockCounter then
	    EndOfWait <= '1';
	  end if;
    end if;
  end process;

end architecture bench;
