library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

--************************************************************************
--* @short Tester of the DetectorSimulator
--************************************************************************
--/
entity TBD is
end entity;

architecture bench of TBD is


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



-- For DetectorSimulator
  signal Clk, Clock  : std_logic;
  signal nReset, Reset : std_logic;


  signal Data_Streams : NSensStreamData_t;
  signal Data_Valid  : std_logic_vector(N_Sensors-1 downto 0);
  signal EndOfEvent  : std_logic_vector(N_Sensors-1 downto 0);

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

  DUT1: DetectorSimulator

  port map(
    clock   => Clk,
    Reset  	=> Reset,
  --  Trigger => Trigger,
    --
	Data_Streams => Data_Streams,
	Data_Valid  => Data_Valid,
	EndOfEvent  => EndOfEvent
  );


--- process (Clk, Reset)



end architecture bench;
