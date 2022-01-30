-- ---------------------- --
-- Event_data_to_DDR3.vhd --
-- ---------------------- --
-- Author: Andrea Savarese --

library IEEE;
  use IEEE.std_logic_1164.all;
  use ieee.numeric_std.all;

entity Event_data_to_DDR3 is
  generic (
    BASE_ADDRESS_RESERVED_MEMORY : unsigned(31 downto 0) := x"00C0_0000";
	AMOUNT_OF_RESERVED_MEMORY    : unsigned(31 downto 0) := x"0100_0000"
  );
  port(
    CLK_DDR3 : in std_logic;
	 reset    : in std_logic;
	 -- Enable
	 enable_writing : in std_logic;
	 -- From the event FIFO
	 FIFOevents_empty : in std_logic;
	 FIFOevents_data  : in std_logic_vector(31 downto 0);
	 FIFO_ack : out std_logic;
	 -- Sync signals from and to HPS
	 fpga_side_RAM_ctrl_reg : out std_logic_vector(31 downto 0);
	 hps_side_RAM_ctrl_reg  : in std_logic_vector(31 downto 0);
	 -- DDR3 IP interface
	 addr_DDR3            : out std_logic_vector(31 downto 0);
	 read_DDR3            : out std_logic;                      -- not used as reading is not required
	 read_data_DDR3       : in std_logic_vector(31  downto 0);  -- not used as reading is not required
	 read_data_valid_DDR3 : in std_logic;                       -- not used as reading is not required
	 write_DDR3           : out std_logic;
	 write_data_DDR3      : out std_logic_vector(31  downto 0);
	 byte_enable_DDR3     : out std_logic_vector(3 downto 0);
	 wait_request_DDR3    : in std_logic;
	 burstcount_DDR3      : out std_logic_vector(7 downto 0)
  );
end Event_data_to_DDR3;

architecture structural of Event_data_to_DDR3 is
  
  constant MAXMEM : unsigned(31 downto 0) := x"3FFF_FFFF";

  --FSM's signals
  type state is (IDLE, WRITING);
  signal current_state, next_state  : state;
  
  -- Control signals
  signal addr_DDR3_int : unsigned(31 downto 0) := BASE_ADDRESS_RESERVED_MEMORY;
  signal next_addr     : std_logic;
  
  -- Sync signals
  signal wrapped_around_fpga, wrapped_around_hps, p_in_different_page, out_of_mem : std_logic := '0';
  signal p_fpga, p_hps : unsigned(29 downto 0) := BASE_ADDRESS_RESERVED_MEMORY(29 downto 0);


  
begin
  
  -- Unfold and fold control signals
  fpga_side_RAM_ctrl_reg <= '0' & wrapped_around_fpga & std_logic_vector(p_fpga);
  
  p_hps <= unsigned(hps_side_RAM_ctrl_reg(29 downto 0));
  wrapped_around_hps  <= hps_side_RAM_ctrl_reg(30);
  
  -- Fixed avalon signals
  burstcount_DDR3 <= "0000000" & '1';
  byte_enable_DDR3 <= "1111";
  
  -- FSM
  process(current_state, FIFOevents_empty, enable_writing, out_of_mem, wait_request_DDR3)
  begin
    
    write_DDR3 <= '0';
    read_DDR3 <= '0';
    FIFO_ack <= '0';
    next_addr <= '0';
    next_state <= current_state;
    
    case current_state is
      
      when IDLE =>-- Write only if the FIFO has data, the enable is active and we have avaliable RAM
        if ( ((not(FIFOevents_empty))='1') and (enable_writing = '1') and (out_of_mem = '0') ) then
    	  next_state <= WRITING;
    	end if;
      
      when WRITING =>-- Avalon transaction
        write_DDR3 <= '1';
    	if (wait_request_DDR3 = '1') then-- if waitrequest is active, keep write asserted
    	  next_state <= current_state;
    	else
    	  FIFO_ack <= '1';-- the write is going to be performed in the next cycle, so prepare
    	next_addr <= '1';-- next address and send aknoweldge to FIFO
    	  next_state <= IDLE;
    	end if;
    	 
      when others =>
        next_state <= IDLE;
    end case;
	
  end process;
  
  write_data_DDR3 <= FIFOevents_data;
  
  -- Addresses generation
  process(CLK_DDR3, reset)
  begin
    if (reset = '1') then
      addr_DDR3_int <= BASE_ADDRESS_RESERVED_MEMORY;
      wrapped_around_fpga <= '0';
    elsif (rising_edge(CLK_DDR3)) then
      if (next_addr='1') then
        -- Find next address to write to
        if ( addr_DDR3_int >= (BASE_ADDRESS_RESERVED_MEMORY + AMOUNT_OF_RESERVED_MEMORY - 4) ) then
    	   addr_DDR3_int <= BASE_ADDRESS_RESERVED_MEMORY;
    		wrapped_around_fpga <= not(wrapped_around_fpga);
    	 else
          addr_DDR3_int <= addr_DDR3_int + 4;
        end if;
      end if;
    end if;
  end process;
	
  addr_DDR3 <= std_logic_vector(addr_DDR3_int);
  p_fpga <= addr_DDR3_int(29 downto 0);
  
  -- p_in_different_page signals that only one pointer has wrapped around
  p_in_different_page <= wrapped_around_fpga xor wrapped_around_hps;
  
  -- If the poiter of the fpga reaches the pointer of hps, then I am out of memory
  -- I need to wait for the hps to read data and free some space
  out_of_mem <= '1' when ( (p_hps = p_fpga) and (p_in_different_page = '1') ) else '0';
 	
  -- FSM's Flip-Flop
  process(CLK_DDR3, reset)
  begin
    if reset = '1' then
      current_state <= IDLE;
    elsif rising_edge(CLK_DDR3) then
      current_state <= next_state;
    end if; 
  end process;
  
  assert ( (to_integer(BASE_ADDRESS_RESERVED_MEMORY) + to_integer(AMOUNT_OF_RESERVED_MEMORY)-4 ) <= to_integer(MAXMEM) ) 
    report "Not enough memory! Check BASE_ADDRESS_RESERVED_MEMORY and AMOUNT_OF_RESERVED_MEMORY variables." severity failure;
  
end architecture;
