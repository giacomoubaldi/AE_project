<<<<<<< HEAD
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std.all;

--************************************************************************
--* @short Tester of the FrameSimulator
--************************************************************************
--/
entity TBF is
end entity;

architecture bench of TBF is


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
--  Trigger : in std_logic;

    --Outputs
	Data_Stream : out unsigned (31 downto 0);
	Data_Valid : out std_logic;
	--Is '1' if that is the last word of the sequence
    EndOfEvent : out std_logic
  );
  end component FrameSimulator;


--generate a clock
  component clockgen is
    generic (CLKPERIOD : time; RESETLENGTH : time);
    port(
      Clock : out std_logic;
      nReset : out std_logic);
  end component;


--generate a trigger randomly
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


  signal Data_Stream: unsigned (31 downto 0);
  signal Data_Valid  : std_logic;
  signal EndOfEvent  : std_logic;

-- for RandomBit
  signal Trigger : std_logic;
  signal seme: std_logic_vector(15 downto 0) := x"CA03";
  signal soglia: std_logic_vector(15 downto 0):=x"0800";





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


  -- --------------------------
    --  Random_bit: to generate a trigger
    -- --------------------------
    RB: RandomBit
   port map (
     clk => Clock,
     nReset =>nReset,
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
  --  Trigger => Trigger,
    --
	Data_Stream => Data_Stream,
	Data_Valid  => Data_Valid,
	EndOfEvent  => EndOfEvent
  );


--- process (Clk, Reset)



end architecture bench;
=======
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std.all;

--************************************************************************
--* @short Tester of the FrameSimulator
--************************************************************************
--/
entity TB is
end entity;

architecture bench of TB is


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


--generate a clock
  component clockgen is
    generic (CLKPERIOD : time; RESETLENGTH : time);
    port(
      Clock : out std_logic;
      nReset : out std_logic);
  end component;


--generate a trigger randomly
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
  signal Trigger : std_logic;

  signal Data_Stream: unsigned (31 downto 0);
  signal Data_Valid  : std_logic;
  signal EndOfEvent  : std_logic;

-- for RandomBit
  signal seme: std_logic_vector(15 downto 0) := x"CA03";
  signal soglia: std_logic_vector(15 downto 0):=x"0800";





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


  -- --------------------------
    --  Random_bit: to generate an external trigger
    -- --------------------------
    RB: RandomBit
   port map (
     clk => Clock,
     nReset =>nReset,
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


--- process (Clk, Reset)



end architecture bench;
>>>>>>> b93b57423bdf1f989ceb2dc60184dd3672bbb37b
