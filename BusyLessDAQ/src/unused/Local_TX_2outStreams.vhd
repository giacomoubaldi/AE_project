library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.BLDAQ_Package.all;

entity Local_TX is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	-- Comm request of reading stored Data
	Comm_Rdreq : in std_logic;-- event_builder fifo
	Comm_RegistersRdreq : in std_logic;-- local_RX fifo
	Comm_DataOut : out std_logic_vector (31 downto 0);-- event_builder fifo
	Comm_RegistersOut : out std_logic_vector (31 downto 0);-- local_RX fifo
	
	-- Communication with Event_Builder
	EventBuilder_Data : in unsigned (31 downto 0);
	EventBuilder_DataValid : in std_logic;
	EventBuilder_Ready : in std_logic;
	EventBuilder_OutRequest : out std_logic;
	-- Communication With Local_RX
	LocalRX_Data : in std_logic_vector (31 downto 0);
	LocalRX_Ready: in std_logic;
	LocalRX_Rdreq : out std_logic;
	
	-- Internal data_Fifo stats
	Fifo_Usage : out std_logic_vector (10 downto 0);
	Fifo_Empty : out std_logic;
	Fifo_AlmostFull : out std_logic;
	Fifo_Full : out std_logic;
	-- Fifo_RX stats
	RegsFifo_Usage : out std_logic_vector (8 downto 0);
	RegsFifo_Empty : out std_logic;
	RegsFifo_AlmostFull : out std_logic;
	RegsFifo_Full : out std_logic
  );
end entity;

architecture description of Local_TX is

  -- This FIFO is used to store data that have to be read via Comm
  component Fifo is
  port (
  	clock		: in std_logic ;
  	data		: in std_logic_vector (31 downto 0);
  	rdreq		: in std_logic ;
  	aclr		: in std_logic ;
  	wrreq		: in std_logic ;
  	almost_full	: out std_logic;
  	empty		: out std_logic ;
  	full		: out std_logic ;
  	q			: out std_logic_vector (31 downto 0);
  	usedw		: out std_logic_vector (10 downto 0)
  );
  end component;
  
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
    clk: in std_logic;
    reset: in std_logic;
    count : out unsigned (nbit-1 downto 0)
  );
  end component;
  
  -- State --
  type state_data is (idle, EventBuilder_Length, readEventBuilder);
  type state_registerData is (idle, LocalRX_Length, readLocalRX);
  signal present_state, next_state : state_data := idle;
  signal actual_state, future_state : state_registerData := idle;
  -- Internal signals --
  signal internalData : std_logic_vector (31 downto 0);
  signal internalWrreq_FifoReg : std_logic;
  signal internalWrreq : std_logic;
  signal internalAlmostFull : std_logic;
  signal internalAlmostFull_FifoReg : std_logic;
  -- Length of the sequence --
  signal sequenceLength : unsigned (11 downto 0) := (others=>'0');
  signal sequenceLength_RX : unsigned (6 downto 0) := (others=>'0');
  -- Counter Signals for Event_Builder sequence --
  signal counterReset : std_logic;
  signal internalReset : std_logic;
  signal internalCounter : unsigned (11 downto 0);
  -- Counter Signals for Local_RX sequence --
  signal counterReset_RX : std_logic;
  signal internalReset_RX : std_logic;
  signal internalCounter_RX : unsigned (6 downto 0);
  
begin
  
  Fifo_AlmostFull <= internalAlmostFull;
  RegsFifo_AlmostFull <= internalAlmostFull_FifoReg;
  
  -- Components port map
  -- Gets data from the event_builder
  Data_Fifo : Fifo
  port map (
    clock => Clock,
    data => internalData,
	rdreq => Comm_Rdreq,
	aclr => Reset,
	wrreq => internalWrreq,
	almost_full => internalAlmostFull,
	empty => Fifo_Empty,
	full => Fifo_Full,
	q => Comm_DataOut,
	usedw => Fifo_Usage
  );
  
  -- Gets data from the local_RX
  RX_Fifo : Fifo_TX_Regs
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
  
  Counter_EB : Counter_nbit
  generic map( nbit => 12)   
  port map (
	CLK => Clock,
	RESET => counterReset,
	COUNT => internalCounter
  );
  
  Counter_RX : Counter_nbit
  generic map( nbit => 7)   
  port map (
	CLK => Clock,
	RESET => counterReset_RX,
	COUNT => internalCounter_RX
  );
  
  -- Reset Counter
  internalReset <= '1' when present_state = idle or present_state = EventBuilder_Length
	                   else '0';
  counterReset <= internalReset or Reset;
  
  -- Reset Counter_RX
  internalReset_RX <= '1' when actual_state = idle or actual_state = LocalRX_Length
	                      else '0';
  counterReset_RX <= internalReset_RX or Reset;
  
  -- Process 1 : Read Event_Builder fsm (it manages fifo_data)
  process(present_state, internalAlmostFull, EventBuilder_Ready,
    EventBuilder_DataValid, internalCounter, sequenceLength)
  begin
    case present_state is
	 
	  when idle =>
	    if internalAlmostFull = '0' then-- If fifo_data is not almost full I can read new data
	  	  if EventBuilder_Ready = '1' then-- event builder has available events
	  	    next_state <= EventBuilder_Length;
	  	  else
	  	    next_state <= idle;
	  	  end if;
	    else
	      next_state <= idle;
	    end if;
	  
	  when EventBuilder_Length =>-- Out Request for Event Builder
	    if EventBuilder_DataValid = '1' then-- Waits DataValid to go high to catch the length of the sequence
	      next_state <= readEventBuilder;
	    else
	      next_state <= EventBuilder_Length;
	    end if;
	  
	  when readEventBuilder =>
	    if internalCounter >= sequenceLength-to_unsigned(2,12) then
          next_state <= idle;
	    else
	      next_state <= readEventBuilder;
	    end if;
	    
	  when others =>
	    next_state <= idle;
	    
    end case;
  end process;
  
  
  -- Process 2 : Reading Local_RX states
  process(actual_state, internalAlmostFull_FifoReg, LocalRX_Ready,
    internalCounter_RX, sequenceLength_RX)
  begin
    case actual_state is
	 
	  when idle =>
	    if internalAlmostFull_FifoReg = '0' then-- If fifo_RX is not almost full I can read new data
	  	  if LocalRX_Ready = '1' then-- local RX has available data from registers
	  	    future_state <= LocalRX_Length;
	  	  else
	  	    future_state <= idle;
	  	  end if;
	    else
	      future_state <= idle;
	    end if;
	  
	  when LocalRX_Length =>
	    future_state <= readLocalRX;
	  
	  when readLocalRX =>
	    if internalCounter_RX >= sequenceLength_RX-to_unsigned(2,12) then
          future_state <= idle;
	    else
	      future_state <= readLocalRX;
	    end if;
	    
	  when others =>
	    future_state <= idle;
	    
    end case;
  end process;
  
  -- Process 3 : Decides which data put in the data_fifo from event builder (it manages fifo_data)
  process(present_state, EventBuilder_DataValid, EventBuilder_Data)
  begin
    case present_state is
	 
	  when idle =>
	    internalWrreq <= '0';
	    internalData <= (others=>'0');
	  
	  when EventBuilder_length =>
	    if EventBuilder_DataValid = '1' then
	      internalWrreq <= '1';
	      internalData <= std_logic_vector(EventBuilder_Data);
	    else
	      internalWrreq <= '0';
	      internalData <= (others=>'0');
	    end if;
	  
	  when readEventBuilder =>
	    internalWrreq <= '1';
	    internalData <= std_logic_vector(EventBuilder_Data);
	  
	  when others =>
	    internalWrreq <= '0';
	    internalData <= (others=>'0');

    end case;
  end process;
 
  -- Fifo Register write request
  internalWrreq_FifoReg <= '1' when actual_state = LocalRX_length or actual_state = readLocalRX
	                           else '0';
  
  -- Out Request for Event_Builder
  EventBuilder_OutRequest <= '1' when next_state = EventBuilder_Length
	                             else '0';
  -- Out Request for Local_RX
  LocalRX_Rdreq <= '1' when future_state = LocalRX_length or future_state = readLocalRX
	                   else '0';

  ---- Process number 4 : sequential part	plus reset  ----
  process(Clock,Reset)
  begin
    if Reset = '1' then
	  p\resent_state <= idle;
	  actual_state <= idle;
	  sequenceLength <= to_unsigned(0,12);
	  sequenceLength_RX <= to_unsigned(0,7);

	elsif rising_edge(Clock) then

	  if present_state = EventBuilder_Length then
	    sequenceLength <= EventBuilder_Data(11 downto 0);
	  end if;
	  if actual_state = LocalRX_Length then
	    sequenceLength_RX <= unsigned(LocalRX_Data(6 downto 0));
	  end if;
	  present_state <= next_state;
	  actual_state<=future_state;

	end if;
  end process;
  
end architecture;
	 