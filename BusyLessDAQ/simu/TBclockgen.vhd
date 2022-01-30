library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

--************************************************************************
--* @short Clock and reset generator for tests
--*
--*  @port Clock   - clock with period "CLKPERIOD"
--*
--*  @port nReset  - Reset active at the beginning for RESETLENGTH ns
--*
--*  Status: OK - Working as expected
--*
--*  @author $Author: villa $
--* @version $Revision: 1.1 $
--*    @date $Date: 2007/10/02 17:17:24 $
--************************************************************************
--/

entity TB is
end entity;

architecture bench of TB is

component clockgen is
	generic (CLKPERIOD : time :=25 ns ; RESETLENGTH : time :=300 ns);
	port(
		Clock : out std_logic;
		nReset : out std_logic);
end component clockgen;

  signal  Clock, nReset  : std_logic;

begin

DUT: clockgen
port map(
Clock   => Clock,
nReset  	=> nReset
);


-- wait for 1000000000 ns;

end architecture bench;
