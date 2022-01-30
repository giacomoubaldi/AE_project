library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std.all;

--************************************************************************
--* @short Tester of the FrameSimulator
--************************************************************************
--/
entity TBS is
end entity;

architecture bench of TBS is


  component FrameSimulator is
  generic (
    headerWord : unsigned (31 downto 0) := x"a0a0_0000";
	footerWord : unsigned (31 downto 0) := x"e0e0_0000";
	sensorTag  : unsigned (15 downto 0) := x"0101";
	periodicity : natural := 200
  );
  --Port declaration
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
  Trigger : in std_logic;

    --Outputs
	Data_Stream : out unsigned (31 downto 0);
	Data_Valid : out std_logic;
	--Is '1' if that is the last word of the sequence
    EndOfEvent : out std_logic
  );
  end component FrameSimulator;


  component FrameReceiver is
  --Port declaration
  port (
    --Inputs
    Clock : in std_logic;
  Reset : in std_logic;
    -- store or not a frame?
  DoStore : in std_logic;
    --Input stream
  Data_StreamIn : in unsigned (31 downto 0);
  Data_ValidIn : in std_logic;
    EndOfEventIn : in std_logic;
  -- Time tags
  TSClock : in unsigned(31 downto 0);
  ClockCounter : in unsigned(47 downto 0);
    --
  SendPacket : in std_logic;
  -- Outputs
  -- Stream
    DataAvailable : out std_logic;
  Data_StreamOut : out unsigned (31 downto 0);
  Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
  Data_Size : out unsigned (11 downto 0);
    -- Time tags of the first word
  ClockCounterOut : out unsigned(47 downto 0);
  FifoFull : out std_logic
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
  TriggerCheck : out std_logic
  );

end component;







--generate a clock
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
    reset: in std_logic;
    count : out unsigned (nbit-1 downto 0)
  );
  end component;



  -- to generate a random trigger inside FrameSimulator
  component RandomBit is
    port(
      clk          : in std_logic;
      nReset       : in std_logic;
      seme         : in std_logic_vector(15 downto 0);
      soglia       : in std_logic_vector(15 downto 0);
      onoff        : out std_logic
    );
  end component RandomBit;






-- For FrameSimulator
  signal Clk, Clock  : std_logic;
  signal nReset, Reset : std_logic;
  signal Trigger: std_logic;


  signal Data_Stream: unsigned (31 downto 0);
  signal Data_Valid  : std_logic;
  signal EndOfEvent  : std_logic;



-- for FrameReceiver
  signal BCOClock : std_logic;
  signal Data_StreamOut : unsigned (31 downto 0);
  signal Data_ValidOut  : std_logic;
  signal EndOfEventOut  : std_logic;
  signal Data_Size : unsigned(11 downto 0);

  -- ClockCounter
  signal TSClock : unsigned(31 downto 0);
  signal ClockCounter, ClockCounterOut : unsigned(47 downto 0);

  signal SendPacket : std_logic;
  signal DataAvailable : std_logic;
  signal FifoFull : std_logic;

  type state is (idle, waiting, reading, ending);
  signal currentState, nextState : state := idle;
  signal EndOfWait : std_logic;


  -- Check for the Frame with Trigger
--signal TriggerCounterOut : unsigned (11 downto 0)  := x"000";
signal TriggerCheck : std_logic := '0';
signal Trigger_Candidate_Counter : std_logic( 15 downto 0 ) := '0';
---signal EndOfEvent : std_logic := '0';


-- for RandomBit trigger
  --signal nReset <= not Reset;

  signal seme: std_logic_vector(15 downto 0) := x"CA03";
  signal soglia: std_logic_vector(15 downto 0):=x"0060";


begin

	-- --------------------------
    --  Clocks: to generate an external clock
    -- --------------------------
  mainClock: clockgen
    generic map (CLKPERIOD => 20 ns, -- 50 MHz clock
                 RESETLENGTH => 240 ns)
    port map ( Clock => Clk, nReset => nReset);
  Reset <= not nReset;
  Clock <= Clk;



  BXCounter : Counter_nbit
  generic map( nbit => 48)
  port map(
    clock => Clk,
    reset => Reset,
    count => ClockCounter
  );




-- for internal clock

     BCO_Clock: clockgen
       generic map (CLKPERIOD => 1 us, -- 1 MHz clock
                    RESETLENGTH => 500 ns)
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




           -- --------------------------
             --  Random_bit: to generate a trigger
             -- --------------------------
             RB: RandomBit
            port map (
              clk => Clock,
              nReset => "not"(Reset),
              seme =>seme,
              soglia =>soglia,
              onoff => Trigger);




	-- --------------------------
    --  Device Under Test 1: FrameSimulator
    -- --------------------------

  DUT1: FrameSimulator
  generic map (
	sensorTag  => x"0404",
	periodicity => 300
  )
  port map(
    clock   => Clk,
    Reset  	=> Reset,
    Trigger => Trigger,

    --
	Data_Stream => Data_Stream,
	Data_Valid  => Data_Valid,
	EndOfEvent  => EndOfEvent
  );

  -- --------------------------
    --  Device Under Test 2: FrameReceiver
    -- --------------------------
  DUT2: FrameReceiver
  port map(
    clock   => Clk,
    Reset  	=> Reset,
    --
  DoStore => '1',
  --
  Data_StreamIn => Data_Stream,
  Data_ValidIn  => Data_Valid,
  EndOfEventIn  => EndOfEvent,
  -- Time tags
  TSClock => TSClock,              -- clock of TS
  ClockCounter => ClockCounter,    --general clock
  -- Outputs
  SendPacket => SendPacket,
  DataAvailable => DataAvailable,
  Data_StreamOut => Data_StreamOut,
  Data_ValidOut => Data_ValidOut,
    EndOfEventOut => EndOfEventOut,
  Data_Size => Data_Size,
  ClockCounterOut => ClockCounterOut,
  FifoFull => FifoFull
  );


  -- --------------------------
    --  Device Under Test 2: TriggerCandidate
    -- --------------------------

  DUT3: Trigger_Candidate
  port map (
  --Inputs
  Clock => Clk,
  Reset => Reset,

  --Input stream
  Data_Stream => Data_StreamOut,
  Data_Valid => Data_ValidOut,
  --Output
  TriggerCheck  => TriggerCheck

  );


  Trigger_Candidate_Counter: RisingEdge_Counter
  port map(
    clock => Clock,
    reset => Reset,
    eventToCount => TriggerCheck,
    counts => Trigger_Candidate_Counter
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
      variable counts, countsMax : natural;
    begin
      if Reset = '1' then
  	  currentState<=idle;
        EndOfWait <= '0';
        counts := 0;
  	  countsMax := 20;
  	elsif rising_edge(Clock) then
        currentState <= nextState;
  	  if currentState = waiting then
  	    counts := counts+1;
  	  elsif currentState = ending and countsMax<1000 then
  	   -- countsMax := countsMax+countsMax;
  	  elsif currentState = ending then
          counts := 0;
  	  end if;
        if counts>= countsMax then
  	    EndOfWait <= '1';
  	  else
  	    EndOfWait <= '0';
  	  end if;

      end if;
    end process;







end architecture bench;
