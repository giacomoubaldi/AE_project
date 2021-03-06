library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity Counter_nbit is
  generic ( nbit : natural := 16);
  port(
    Clock: in std_logic;
    Reset: in std_logic;
    Count : out unsigned (nbit-1 downto 0)
  );
end Counter_nbit;


architecture behavior of Counter_nbit is

  signal value_temp : unsigned (nbit-1 downto 0) := ( others =>'0'); --

begin

  process(Clock, Reset)
  begin
    if Reset = '1' then
      value_temp <= ( others =>'0');
	elsif rising_edge(Clock) then
      value_temp <= value_temp + to_unsigned(1,nbit);
    end if;
  end process;

  Count <= value_temp;

end architecture;
