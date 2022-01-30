library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std.all;

--************************************************************************
--* @short Tester of the InSignControl
--************************************************************************
--/
entity TB is
end entity;

architecture bench of TB is


  component InSignControl is
  port (
    --Inputs 
    Clock : in std_logic;          
    Reset : in std_logic;                
    --External clock 
    BCOClockIn : in std_logic;      
    BCOResetIn : in std_logic;    
    --Trigger signal from GPIO
    TriggerIn : in std_logic;       
     
    --Outputs
    BCOClockOut : out std_logic;      
    BCOResetOut : out std_logic;    
    --Trigger signal received
    TriggerOut  : out std_logic;       

    --BCOCounter Counts the number of BCOClocks      
    TSCounter      : out unsigned (31 downto 0);  
    --TriggerCounter Counts the number of external trigger received (not those accepted!)   
    trigRecCounter : out unsigned (31 downto 0)
  );
  end component InSignControl;
  

  component clockgen is
    generic (CLKPERIOD : time; RESETLENGTH : time);
    port(
      Clock : out std_logic;
      nReset : out std_logic);
  end component;

  signal Clk  : std_logic;
  signal nReset, Reset : std_logic;
  signal BCOClock, BCOReset : std_logic;

--**************************************************************************
--  Main config variables
  constant VetoIsOn : std_logic := '1'; 
  ----------------------------------------------------------
  signal randombits : std_logic_vector(15 downto 0) :=(others=>'0');
  -------------------------------------------------
    -- Trigger sources
  signal Trigger : std_logic := '0';
    -- Outputs
  signal BCOCounter : unsigned (31 downto 0);      -- Counts the number of BCOClocks 
  signal trigRecCounter : unsigned (31 downto 0);  -- Counts the number of Main_Trigger since DAQIsRunning passed from 0 to 1
	--Int_Trigger 
  signal BCOClockOut, BCOResetOut, TriggerOut : std_logic;      
  signal Clock : std_logic;

begin

	-- --------------------------
    --  Clocks
    -- --------------------------  
  mainClock: clockgen
    generic map (CLKPERIOD => 20 ns, -- 50 MHz clock
                 RESETLENGTH => 240 ns)
    port map ( Clock => Clk, nReset => nReset);
  Reset <= not nReset;
  Clock <= Clk;
	-- --------------------------
    --  Device Under Test: Trigger_Control
    -- --------------------------  

  DUT_TS: InSignControl
  port map(
    clock   => Clk,
    Reset  	=> Reset,
   --External clock 
    BCOClockIn => BCOClock,      
    BCOResetIn => BCOReset,    
    --Trigger signal from GPIO
    TriggerIn  => Trigger,       
     
    --Outputs
    BCOClockOut => BCOClockOut,
    BCOResetOut => BCOResetOut,    
    --Trigger signal received
    TriggerOut  => TriggerOut,

    --BCOCounter Counts the number of BCOClocks      
    TSCounter   => BCOCounter,
    --TriggerCounter Counts the number of external trigger received (not those accepted!)   
    trigRecCounter => trigRecCounter
  );


  
-----------------------------------------
--- Simulate BCOClock
-----------------------------------------
	BCO : process(Clock)
        variable countdownRes : natural := 100;
		variable countdownClk : natural := 50;
      begin
		if rising_edge(Clock) then
		   -- generate BCOReset
		   if Reset='1'  then
			  BCOReset <='1';
			  countdownRes :=100;
			elsif countdownRes<5 then
			  BCOReset<='0';
			else
			  BCOReset <='0';
			  countdownRes := countdownRes -1;
			end if;
			-- generate BCOClock  2 us time period
			if countdownRes>4 then
			   BCOClock <= '0';
			   countdownClk := 50;
			elsif countdownClk>0 then
			   countdownClk := countdownClk-1;
			else
			   countdownClk := 50;
			   BCOClock <= not(BCOClock);
			end if;			  
		end if;
	end process;

-----------------------------------------
-- define random Trigger
-----------------------------------------

  stimulus: process(Clk) is
    variable counter : natural := 0;
 begin
    if nReset='0' then
	  counter := 200;
      Trigger <= '0';
    elsif( rising_edge(Clk) ) then
	    if counter=0 then
		  Trigger <= '1';
		  counter := 13+to_integer(unsigned(randombits(3 downto 0)));
		else
		  Trigger <= '0';
		  counter := counter-1;
		end if;
      --
    end if;
  end process;
  
  
    -- find a new 16 bits random value
  rnd: process( clk, nReset) is
    variable randomnumber : std_logic_vector(15 downto 0) :=  x"C0A1";
  begin
	if(nReset = '0') then
      randombits <= (others=>'0');
	  randomnumber :=  x"C0A1";
	elsif rising_edge(Clk) then
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


end architecture bench;
 
