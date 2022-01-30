library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity TimeStampGenerator is  
  port (
    Clock : in std_logic;
    Reset : in std_logic;
	DAQRunning : in std_logic;
    TSReset    : out std_logic;
    TSClock    : out std_logic
  );
end TimeStampGenerator;

architecture behavioural of TimeStampGenerator is

  constant HALFTSPERIOD : natural range 0 to 255 := 40;
  constant WAITPERIOD : natural range 0 to 255 := 160;
  constant RESETLENGTHSTART:  natural := 15;
  signal TScounter : natural range 0 to 255 := 160;
  signal oldrunningbit, toggle : std_logic := '0';
  signal resetlength : natural  range 0 to 15 := 15;
  
begin
  
  -- Provide a global status depending on few key signals:
  -- nReset, RunningBit, LoadingBit
  --------------------------------------------------------
  -- At the end of the run, for a while it keeps RunningBit
  -- high to allow for fifo flushing
  
  process(Clock,Reset) is
  begin
    if Reset='1' then
      TSReset <= '1';
      TSClock <= '0';
      toggle <='0';
      resetlength <= RESETLENGTHSTART;
      TScounter <= WAITPERIOD;
    elsif rising_edge(Clock) then

      if resetlength>0 then
        TSReset <= '1';
        TSClock <= '0';
        toggle <='0';
        resetLength <= resetLength-1;
        TScounter <= WAITPERIOD;
      elsif  DAQRunning='1' and oldrunningbit='0' then
        TSReset <= '1';
        TSClock <= '0';
        toggle <='0';     
        resetlength <=RESETLENGTHSTART;
        TScounter <= WAITPERIOD;
      elsif TScounter>0 then
        TSReset <= '0';
        TSClock <= toggle;
        TScounter <= TScounter -1;
      else
        TSReset <= '0';
        TSClock <= toggle;
        TScounter <= HALFTSPERIOD;
        toggle <=not toggle;     
      end if;

      oldrunningbit <= DAQRunning;

    end if;
  end process;

end behavioural;

