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
entity clockgen is
	generic (CLKPERIOD : time :=25 ns ; RESETLENGTH : time :=100 ns);
	port(
		Clock : out std_logic;
		nReset : out std_logic);
end clockgen;

architecture behavioral of clockgen is
begin

  resetdrv: process
  begin  
	nReset <= '0', '1' after RESETLENGTH;    
	wait;    
  end process;

	
  clockdrv: process
  begin
    Clock <= '1';
    wait for CLKPERIOD/2;
    loop
      Clock <= '0', '1' after CLKPERIOD/2;
      wait for CLKPERIOD;
    end loop;
  end process;
  
end architecture behavioral;   
