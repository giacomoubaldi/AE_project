library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

--************************************************************************
--*
--*     @short Random bit generation to simulate PMT responces
--*
--*     @port Clk      Input clock
--*     @port nReset   Negated input reset
--*     @port seme     Input seed for random generation (sampled at Reset)
--*     @port soglia   Input thresholds for turning on the PMT
--*     @port onoff    Output bit
--*
--************************************************************************

entity RandomBit is
  port(
    clk          : in std_logic;                 
    nReset       : in std_logic;                 
    seme         : in std_logic_vector(15 downto 0);                 
    soglia       : in std_logic_vector(15 downto 0);                 
    onoff        : out std_logic
  );         
end entity RandomBit;

architecture a of RandomBit is
  signal randomnumber : std_logic_vector(15 downto 0);
  signal counter : unsigned(15 downto 0) := x"0000";
begin
  
  process(clk, nReset,seme)is 
  begin
    if nReset='0' then
      randomnumber <= seme;
      counter <= x"0000";
    elsif rising_edge(clk) then
      
      if counter=unsigned(seme) then
        randomnumber <= seme;
        counter <= x"0000";        
      else            
        randomnumber <=   randomnumber(14 downto 10)
                          & (not randomnumber(15) xor randomnumber(9))
                          &       randomnumber(8 downto 5)
                          & (not randomnumber(15) xor randomnumber(4))
                          & randomnumber(3 downto 2)
                          & (not randomnumber(15) xor randomnumber(1))
                          & randomnumber(0)
                          & (not randomnumber(15) xor randomnumber(2));      
        counter <= counter + 1;
      end if;
    end if;
  end process;
  
  process(clk, nReset) is 
  begin
    if nReset='0' then
      onoff <= '0';
    elsif rising_edge(clk) then
      if to_integer(unsigned(randomnumber))<to_integer(unsigned(soglia)) then
        onoff<='1';
      else
        onoff<='0';
      end if;   
    end if;
  end process;
  
end architecture a;
		
