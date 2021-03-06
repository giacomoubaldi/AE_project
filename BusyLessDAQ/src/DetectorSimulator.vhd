library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

--Entity Random_HeaderFooter generates a stream of 32bit words
--When the trigger signal goes from 1 to 0 the machine goes to running mode
--First a constant sequence (header) is generated
--Then a 32bit packet containing the length in words of the total sequence considering also header and footer
--Now the output is a random 32bit number (random stream)
--The sequence concludes with a constant ending part (footer)

entity DetectorSimulator is
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
end entity;



architecture algorithm of DetectorSimulator is

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


  -- for RandomBit trigger
  --signal nReset <= not Reset;
  signal seme: std_logic_vector(15 downto 0) := x"CA03";
  signal soglia: std_logic_vector(15 downto 0):=x"0060";
  signal Trigger : std_logic := '0';





  subtype sensorTag_t is unsigned(15 downto 0);
  type sensTagArr_t is array (N_Sensors-1 downto 0) of sensorTag_t;
  constant tags : sensTagArr_t := ( x"0303", x"0202", x"0101", x"0000");

  

begin

  Sens:
  for i in 0 to N_Sensors-1 generate
    FS: FrameSimulator
        generic map( sensorTag=> tags(i), periodicity=> 200+i+i)
        port map (
		  Clock => Clock,
	      Reset => Reset,
        Trigger => Trigger,
	      Data_Stream => Data_Streams(i),
	      Data_Valid => Data_Valid(i),
	      EndOfEvent => EndOfEvent(i)
	    );
  end generate;



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
           


end architecture;
