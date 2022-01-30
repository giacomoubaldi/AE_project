library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

-- Input signal control

entity InSignControl is
  port (
    --Inputs 
    Clock : in std_logic;          
    Reset : in std_logic;                
    --External clock 
    TSClockIn : in std_logic;      
    TSResetIn : in std_logic;    
    --Trigger signal from GPIO
    TriggerIn : in std_logic;       
     
    --Outputs
    TSClockOut : out std_logic;      
    TSResetOut : out std_logic;    
    --Trigger signal received
    TriggerOut  : out std_logic
  );
end entity;


   
   ----Architecture of the InSignControl ----
  
architecture Structural of InSignControl is

  --Declaration of components--
  --Used for TSClockCounter and TriggerCounter
  component RisingEdge_Counter 
    generic ( NBITS: natural := 32);
    port(
      clock    : in std_logic;
      reset    : in std_logic;
      eventToCount    : in std_logic;   
      --Output 
      counts : out unsigned (NBITS-1 downto 0)
    );
    end Component;

  ----  Internal Signals  ----	 
    --External clock 
  signal TSClockR, TSClockR1, TSClockR2,TSClock0 : std_logic:= '0';      
    --Trigger signal from GPIO
  signal TriggerR, Trigger0 : std_logic := '0';       

  -- Used if TSReset acts like Reset
  signal TSCounterReset : std_logic;                  
  
begin            --Structure description--
                                                  
  sync_signals : process (Clock, Reset)  
    variable TSResetLong : std_logic_vector(7 downto 0):= (others=>'1');
  begin
    if Reset = '1' then
      TSCounterReset <= '1'; 
	  TriggerR <= '0';
	  TSClockR <= '0';
	  Trigger0 <= '0';
	  TSClock0 <= '0';
	  TSResetLong := x"FF";
	elsif rising_edge(Clock) then   

	  -- synching resets
      if( TSResetLong = x"FF" )then	  
        TSCounterReset <= '1'; -- On when TSReset lasts for 8*20= 160 ns
	  else
        TSCounterReset <= '0';   
	  end if;
	  -- registering external signals
	  TriggerR <= Trigger0;
	  TSClockR2 <= TSClockR1;
	  TSClockR1 <= TSClockR;
	  TSClockR <= TSClock0;
	  Trigger0 <= TriggerIn;
	  TSClock0 <= TSClockIn;
	  TSResetLong := TSResetLong(6 downto 0) &  TSResetIn;
	end if;
  end process;
  
  TSResetOut <= TSCounterReset;
  TSClockOut <= TSClockR2;
  TriggerOut  <= TriggerR;
                                                    
end architecture;
     

     