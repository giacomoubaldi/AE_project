library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

-- It provides a way to match the output of the EventBuilder (a continuous stream of data for each frame)
-- to the needs of the RAM interface that reads one value at a time.

entity DataTransfer is
  port (
    Clock : in std_logic;
	Reset : in std_logic;
    -- I/O to Event Builder 
	SendPacket : out std_logic;
    DataAvailable : in std_logic;
	Data_Stream : in unsigned (31 downto 0);  
	Data_Valid : in std_logic;
    EndOfEvent : in std_logic;
    -- I/O to DDR3
    data_ack    : in std_logic;
    data_event  : out std_logic_vector(31 downto 0);
    data_empty  : out std_logic;
	--
	Fifo_info : out std_logic_vector(16 downto 0)
  );
end entity;

architecture behavioural of DataTransfer is

  component TransferBuffer
	port (
	  aclr		: in std_logic ;
	  clock		: in std_logic ;
	  data		: in std_logic_vector (31 downto 0);
	  rdreq		: in std_logic ;
	  wrreq		: in std_logic ;
	  almost_full		: out std_logic ;
	  empty		: out std_logic ;
	  full		: out std_logic ;
	  q		: out std_logic_vector (31 downto 0);
	  usedw		: out std_logic_vector (12 downto 0)
	);
  end component;

  signal almost_full : std_logic;
  signal empty		: std_logic;
  signal full		: std_logic;
  signal usedw		: std_logic_vector (12 downto 0);

  type status_t is (waitstart, startTransfer, transferring, ending);
  signal current_state, next_state : status_t := waitstart;

begin

  TB: TransferBuffer
  port map (
    clock => Clock,
    aclr => Reset,
    wrreq => Data_Valid,
    data => std_logic_vector(Data_Stream),
    rdreq => data_ack,
    q => data_event,
	full => full,
	almost_full => almost_full,
	empty => empty,
	usedw => usedw
  );
  data_empty <= empty; 
  Fifo_info <= full& almost_full & empty & full & usedw; 
  
  process(current_state, DataAvailable, EndOfEvent, empty) is
  begin
    case current_state is
      when waitstart =>
  	    if DataAvailable='1' and empty='1' then
  	      next_state <= startTransfer;
  	    else 
  	      next_state <= waitstart;
  	    end if;
  	  when startTransfer =>
	    next_state <= transferring;
		
  	  when transferring => 
  	    if EndOfEvent='1' then
  	      next_state <= ending;
  	    else 
  	      next_state <= transferring;
  	    end if;
  	  
      when ending => 
  	    if empty='1' then
  	      next_state <= waitstart;
  	    else 
  	      next_state <= ending;
  	    end if;
		
	  when others => 
  	    next_state <= waitstart;
	    
	end case;
	
  end process;
  
  process(Clock, Reset) is
  begin
    if Reset='1' then
	  current_state <= waitstart;
	  SendPacket <= '0';
    elsif rising_edge(Clock) then
	  if current_state=startTransfer then
	    SendPacket <= '1';
	  else
	    SendPacket <= '0';
      end if;	  
      current_state <= next_state;
	end if;
  end process;
  
end architecture;