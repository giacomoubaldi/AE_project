library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std.all;

--************************************************************************
--* @short Tester of the clockgen
--************************************************************************
--/
entity TB is
end entity;

architecture bench of TB is
component RandomBit is
  port(
    clk          : in std_logic;
    nReset       : in std_logic;
    seme         : in std_logic_vector(15 downto 0);
    soglia       : in std_logic_vector(15 downto 0);
    onoff        : out std_logic
  );
end component RandomBit;



  component  RisingEdge_Counter is
  generic ( NBITS: natural := 32);
  port(
    clock    : in std_logic;
    reset    : in std_logic;
    --  Signal where to count
    eventToCount    : in std_logic;
    -- Output: number of rising edges found
    counts : out unsigned (NBITS-1 downto 0) := to_unsigned(0,NBITS)
  );
  end component;

  component clockgen is
    generic (CLKPERIOD : time; RESETLENGTH : time);
    port(
      Clock : out std_logic;
      nReset : out std_logic);
  end component;

  signal Clock  : std_logic;
  signal nReset, Reset : std_logic;

  signal TSReset : std_logic := '1';
  signal TSClock : std_logic := '0';
  signal seme: std_logic_vector(15 downto 0) := x"CA03";
  signal soglia: std_logic_vector(15 downto 0):=x"0800";
   -- Outputs
  signal TSCounter : unsigned (31 downto 0);  -- Counts the number of TSClocks
 signal onoff: std_logic;

begin

  -- --------------------------
  --  Clocks
  -- --------------------------
  mainClock: clockgen
    generic map (CLKPERIOD => 20 ns, -- 50 MHz clock
                 RESETLENGTH => 240 ns)
    port map ( Clock => Clock, nReset => nReset);
  Reset <= not nReset;

  -- --------------------------
  --  Device Under Test:
  -- --------------------------
  Dut: RisingEdge_Counter
  port map (
    clock => Clock,
    reset => Reset,
    eventToCount => onoff,
    Counts => TScounter);
-- ----------------------------
-- Random Bit:
-- ----------------------------
 RB: RandomBit
port map (
  clk => Clock,
  nReset =>nReset,
  seme =>seme,
  soglia =>soglia,
  onoff => onoff);




-----------------------------------------
--- Simulate TSClock
-----------------------------------------
  TS : process(Clock)
    variable countdownRes : natural := 100;
    variable countdownClk : natural := 49;
  begin
    if rising_edge(Clock) then
      -- generate TSReset
      if Reset='1'  then
        TSReset <='1';
        countdownRes :=100;
      elsif countdownRes<5 then
        TSReset<='0';
      else
        TSReset <='1';
        countdownRes := countdownRes -1;
      end if;
      -- generate TSClock  2 us time period
      if countdownRes>4 then
        TSClock <= '0';
        countdownClk := 49;
      elsif countdownClk>0 then
        countdownClk := countdownClk-1;
      else
        countdownClk := 49;
        TSClock <= not(TSClock);
      end if;
    end if;
  end process;

end architecture bench;
