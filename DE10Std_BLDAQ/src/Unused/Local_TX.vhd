library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.BLDAQ_Package.all;

entity Local_TX is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	-- Comm request of reading stored Data
	Comm_RegistersRdreq : in std_logic;-- local_TX fifo
	Comm_RegistersOut : out std_logic_vector (31 downto 0);-- local_TX fifo
	
	-- Communication With Local_RX
	LocalRX_Data : in std_logic_vector (31 downto 0);
	LocalRX_Ready: in std_logic;
	LocalRX_Rdreq : out std_logic;
	
	-- Fifo_TX stats
	RegsFifo_Usage : out std_logic_vector (8 downto 0);
	RegsFifo_Empty : out std_logic;
	RegsFifo_AlmostFull : out std_logic;
	RegsFifo_Full : out std_logic
  );
end entity;

architecture description of Local_TX is
  
  ---- Data from Registers that has to be read through Comm ----
  component Fifo_TX_Regs is
  port (
    clock		: in std_logic ;
    data		: in std_logic_vector (31 downto 0);
    rdreq		: in std_logic ;
    aclr		: in std_logic ;
    wrreq		: in std_logic ;
    almost_full	: out std_logic ;
    empty		: out std_logic ;
    full		: out std_logic ;
    q			: out std_logic_vector (31 downto 0);
    usedw		: out std_logic_vector (8 downto 0)
  );
  end component;
  
  -- Internal Counter
  component Counter_nbit
  generic ( nbit : natural := 12);
  port(
    Clock: in std_logic;
    Reset: in std_logic;
    Count : out unsigned (nbit-1 downto 0)
  );
  end component;
  
  -- State --
  type state_registerData is (idle, LocalTX_Length, readLocalTX);
  signal present_state, next_state : state_registerData := idle;
  -- Internal signals --
  signal internalWrreq_FifoReg : std_logic;
  signal internalAlmostFull_FifoReg : std_logic;
  -- Length of the sequence --
  signal sequenceLength_TX : unsigned (6 downto 0) := (others=>'0');
  -- Counter Signals for Event_Builder sequence --
  signal counterReset : std_logic;
  signal internalReset : std_logic;
  signal internalCounter : unsigned (11 downto 0);
  -- Counter Signals for Local_TX sequence --
  signal counterReset_TX : std_logic;
  signal internalReset_TX : std_logic;
  signal internalCounter_TX : unsigned (6 downto 0);
  
begin
  
  RegsFifo_AlmostFull <= internalAlmostFull_FifoReg;
    
  -- Gets data from the local_TX
  TX_Fifo : Fifo_TX_Regs
  port map (
    clock => Clock,
    data => LocalRX_Data,
	rdreq => Comm_RegistersRdreq,
	aclr => Reset,
	wrreq => internalWrreq_FifoReg,
	almost_full => internalAlmostFull_FifoReg,
	empty => RegsFifo_Empty,
	full => RegsFifo_Full,
	q => Comm_RegistersOut,
	usedw => RegsFifo_Usage
  );
    
  Counter_TX : Counter_nbit
  generic map( nbit => 7)   
  port map (
	Clock => Clock,
	Reset => counterReset_TX,
	Count => internalCounter_TX
  );
  
  -- Reset Counter
  internalReset <= '1' when present_state = idle 
	                   else '0';
  counterReset <= internalReset or Reset;
  
  -- Reset Counter_TX
  internalReset_TX <= '1' when present_state = idle or present_state = LocalTX_Length
	                      else '0';
  counterReset_TX <= internalReset_TX or Reset;
  
  
  -- Process 2 : Reading Local_TX states
  process(present_state, internalAlmostFull_FifoReg, LocalRX_Ready,
    internalCounter_TX, sequenceLength_TX)
  begin
    case present_state is
	 
	  when idle =>
	    if internalAlmostFull_FifoReg = '0' then-- If fifo_TX is not almost full I can read new data
	  	  if LocalRX_Ready = '1' then-- local RX has available data from registers
	  	    next_state <= LocalTX_Length;
	  	  else
	  	    next_state <= idle;
	  	  end if;
	    else
	      next_state <= idle;
	    end if;
	  
	  when LocalTX_Length =>
	    next_state <= readLocalTX;
	  
	  when readLocalTX =>
	    if internalCounter_TX >= sequenceLength_TX-to_unsigned(2,12) then
          next_state <= idle;
	    else
	      next_state <= readLocalTX;
	    end if;
	    
	  when others =>
	    next_state <= idle;
	    
    end case;
  end process;
   
  -- Fifo Register write request
  internalWrreq_FifoReg <= '1' when present_state = LocalTX_length or present_state = readLocalTX
	                           else '0';
  
  -- Out Request for Local_RX
  LocalRX_Rdreq <= '1' when next_state = LocalTX_length or next_state = readLocalTX
	                   else '0';

  ---- Process number 4 : sequential part	plus reset  ----
  process(Clock,Reset)
  begin
    if Reset = '1' then
	  present_state <= idle;
	  sequenceLength_TX <= to_unsigned(0,7);

	elsif rising_edge(Clock) then

	  if present_state = LocalTX_Length then
	    sequenceLength_TX <= unsigned(LocalRX_Data(6 downto 0));
	  end if;
	  
	  present_state<=next_state;

	end if;
  end process;
  
end architecture;
	 