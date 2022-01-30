library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;


entity Main_FSM is
  generic(
	-- Minimum time to wait before checking if it's possible to go to config mode
	Time_Out : unsigned (31 downto 0)
  );
  port(
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	To_State : in std_logic_vector (1 downto 0);
	-- MV: Usage of these two signals has to be checked!!
	Empty_Fifo : in std_logic;
	ReadingEvent : in std_logic;
	--State of the machine
	Status : out MainFSM_state;
	--Outputs
	DAQIsRunning : out std_logic;
	DAQ_Reset : out std_logic;
	DAQ_Config : out std_logic
  );
end entity;



architecture behavioral of Main_FSM is
  
  -- Internal Counter declaration --
  component Counter_nbit 
    generic ( nbit : natural := 8);  
    port(
      Clock: in std_logic;
      Reset: in std_logic;
      Count : out unsigned (nbit-1 downto 0)
    );
  end component;

  ----  Internal signals  ----
  signal present_state, next_state : MainFSM_state := idle;
  signal internalCounter : unsigned (31 downto 0) := (others=>'0');
  signal internalReset : std_logic := '0';
  signal counterReset : std_logic;

begin

  Status <= present_state;
  
  -- Counter instantiation --
  Counter : Counter_nbit 
  generic map( nbit => 32 ) 
  port map (
	 Clock => Clock,
	 Reset => counterReset,
	 Count => internalCounter
  ); 
  
  internalReset <= '0' when present_state = PrepareForRun or present_state = EndOfRun
                       else '1';
  counterReset <= internalReset or Reset; 

  
  ----  Process 1 : States Flow  ----
  process(present_state, To_State, Empty_Fifo, ReadingEvent, internalCounter)
  begin
    case present_state is
	   
		when Idle =>
		  if To_State = IdleToConfig then
		    next_state <= Config;
		  else
		    next_state <= Idle;
		  end if;
		  
		when Config =>
		  if To_State = ConfigToRun then
		    next_state <= PrepareForRun;
		  elsif To_state = ConfigToIdle then
		    next_state <= Idle;
		  else
		    next_state <= Config;
		  end if;
		
		when PrepareForRun =>
		  if internalCounter = to_unsigned(4,32) then
		    next_state <= Run;
		  else
		    next_state <= PrepareForRun;
		  end if;
		  
		when Run =>
		  if To_State = RunToConfig then
		    next_state <= EndOfRun;
		  else
		    next_state <= Run;
		  end if;
		  
		when EndOfRun =>
		  if readingEvent = '0' and internalCounter>= Time_Out then
		    next_state <= WaitingEmptyFifo;
		  else
		    next_state <= EndOfRun;
		  end if;
		  
		when WaitingEmptyFifo =>
		  if Empty_Fifo = '1' then
		    next_state <= Config;
		  else
		    next_state <= WaitingEmptyFifo;
		  end if;
		  
		when others =>
		  next_state<=Idle; 
		
    end case;
  end process;
  
  ----  Process 2 : Output Control Signals  ----
  process( present_state, internalCounter )
  begin
    case present_state is
	   
		when Idle =>
		  DAQIsRunning <= '0';
		  DAQ_Reset <= '0';
		  DAQ_Config <= '0';
		  
		when Config =>
		  DAQIsRunning <= '0';
		  DAQ_Reset <= '0';
		  DAQ_Config <= '1';
		  
		when PrepareForRun =>
		  if internalCounter < to_unsigned(2,32) then
		    DAQIsRunning <= '0';
		    DAQ_Reset <= '1';
		    DAQ_Config <= '0';
		  else
		    DAQIsRunning <= '0';
		    DAQ_Reset <= '0';
		    DAQ_Config <= '0';  
		  end if;
		  
		when Run =>
		  DAQIsRunning <= '1';
		  DAQ_Reset <= '0';
		  DAQ_Config <= '0'; 
		
		when EndOfRun =>
		  DAQIsRunning <= '0';
		  DAQ_Reset <= '0';
		  DAQ_Config <= '0';
		
		when WaitingEmptyFifo =>
		  DAQIsRunning <= '0';
		  DAQ_Reset <= '0';
		  DAQ_Config <= '0';
		
		when others =>
		  DAQIsRunning <= '0';
		  DAQ_Reset <= '0';
		  DAQ_Config <= '0';
    
	 end case;
  end process;

  ----  Process 3 : Sequential part  ----
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  present_state <= Idle;
    elsif rising_edge(Clock) then
      present_state <= next_state; 
    end if;
  end process;

end architecture;
  
  