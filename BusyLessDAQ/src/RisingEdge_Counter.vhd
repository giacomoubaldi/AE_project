library ieee;
use ieee.std_logic_1164.all;
use IEEE.numeric_std.all;

-- Counts "eventToCount" signal rising edges


entity RisingEdge_Counter is			
  generic ( NBITS: natural := 32);
  port(
    clock    : in std_logic;
    reset    : in std_logic;
    --  Signal where to count 
    eventToCount    : in std_logic;    
    -- Output: number of rising edges found 
    counts : out unsigned (NBITS-1 downto 0) := to_unsigned(0,NBITS)
  );
end entity;


architecture behave of RisingEdge_Counter is

  signal localCounts   : unsigned (NBITS-1 downto 0) := to_unsigned(0,NBITS) ;
  signal last_EventToCount : std_logic := '0';     						
  --Indicates the last value of the Event that we want to count
  --This assures that a multi-clock pulse is considered as one event 

  begin
  -- count on rising edge of eventToCount
  -- Asyncronous Reset
  
  process (clock, reset) is
  begin
    if reset='1' then				
      localCounts <= to_unsigned(0,NBITS);
      last_EventToCount <= '0';	        
     -- counts <= to_unsigned(0,NBITS);
    elsif rising_edge(clock) then
      if eventToCount='1' and last_EventToCount= '0' then
        localCounts <= localCounts + to_unsigned(1,NBITS);  
      end if;
      last_EventToCount <= eventToCount; 
      counts <= localCounts;        
    end if;      
  end process;



end architecture behave;
