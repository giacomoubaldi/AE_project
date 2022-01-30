library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity StreamSelector is
  port(
    Clock: in std_logic;
    Reset: in std_logic;
    -- Input TS
	TSValid : in std_logic_vector(3 downto 0);
	TSBX0 : in unsigned(47 downto 0);
	TSBX1 : in unsigned(47 downto 0);
	TSBX2 : in unsigned(47 downto 0);
	TSBX3 : in unsigned(47 downto 0);
	-- Output
	SelectionValid : out std_logic;
	Selector : out std_logic_vector(1 downto 0);
	TSBXSelected : out unsigned(47 downto 0)
  );
end StreamSelector;


architecture behavior of StreamSelector is

  signal TSBX01, TSBX23 : unsigned(47 downto 0);
  signal valid01, valid23: std_logic; 
  signal selector01, selector23 : std_logic; 

begin 

  selector01 <= '0' when TSValid(0)='1' and TSValid(1)='1' and TSBX0<=TSBX1 else
                '1' when TSValid(0)='1' and TSValid(1)='1' and TSBX0>TSBX1  else
                '0' when TSValid(0)='1' and TSValid(1)='0' else
                '1' when TSValid(0)='0' and TSValid(1)='1' else
			    '0';
  valid01 <= TSValid(0) or TSValid(1);
  
  selector23 <= '0' when TSValid(2)='1' and TSValid(3)='1' and TSBX2<=TSBX3  else
                '1' when TSValid(2)='1' and TSValid(3)='1' and TSBX2>TSBX3   else
                '0' when TSValid(2)='1' and TSValid(3)='0' else
                '1' when TSValid(2)='0' and TSValid(3)='1' else
			    '0';
  valid23 <= TSValid(2) or TSValid(3);

  TSBX01 <= TSBX0 when selector01='0'
            else TSBX1;
  TSBX23 <= TSBX2 when selector23='0'
            else TSBX3;

  process(Clock, Reset)  
  begin					
    if Reset = '1' then
	  SelectionValid <= '0';
	  Selector <= "00";
	  TSBXSelected <= (others=>'0');
	elsif rising_edge(Clock) then
      SelectionValid <= valid01 or valid23;
	  if TSBX01<=TSBX23 and valid01='1' and valid23='1' then
		Selector <= '0' & selector01;
		TSBXSelected <= TSBX01;
	  elsif TSBX01>TSBX23 and valid01='1' and valid23='1' then
		Selector <= '1' & selector23;
		TSBXSelected <= TSBX23;
      elsif valid01='1' then
		Selector <= '0' & selector01;
		TSBXSelected <= TSBX01;
      elsif valid23='1' then
		Selector <= '1' & selector23;
		TSBXSelected <= TSBX23;
	  else 
		Selector <= "00";
		TSBXSelected <= (others=>'0');
	  end if;
    end if;
  end process;

end architecture;