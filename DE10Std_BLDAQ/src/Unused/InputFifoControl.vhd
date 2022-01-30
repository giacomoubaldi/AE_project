-- -----------------------------------------------------------------------
-- InputFifoControl
-- -----------------------------------------------------------------------          
--
-- It receives a packet of commands from HPS
-- It stores received data into a local fifo till the end marker
-- At that point it sends out the packet.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.BLDAQ_Package.all;

entity InputFifoControl is
  generic(TIMEOUT : natural := 5000000); -- 100 ms
  port(
    clk    : in  std_logic;
    reset  : in  std_logic;
	 -- HPS signal
    iofifo_datain      : in    std_logic_vector(31 downto 0);                    -- datain
    iofifo_writereq    : in    std_logic;                                        -- writereq
    iofifo_instatus    : out   std_logic_vector(31 downto 0);                    -- instatus
    -- To DAQ Module
	RX_almostFullFlag  : in    std_logic;-- from local rx, almost full of fifo rx
    Comm_Wrreq     : out   std_logic;-- To DAQ_OnFPGA, to local rx
	Comm_DataIn    : out   std_logic_vector(31 downto 0)-- To DAQ_OnFPGA, to local rx
  );
end entity InputFifoControl;

architecture logic of InputFifoControl is

  constant ENDMARKER : std_logic_vector(31 downto 0) := Instruction_Footer;
  -- fifo signals
  signal readreq, empty, almost_full, full, busy : std_logic := '0';
  signal usedw : std_logic_vector(7 downto 0 );

  signal idletime : natural := TIMEOUT;
  signal lastwordreceived, done : std_logic := '0';
  signal firstword : natural;
  
  type inStatus_t is (IDLE, WAITFIRST, WAITEND, BUSYSTATE, WAITING);
  signal nextState, previousState, oldState : instatus_t := IDLE;

begin

  CmdFifo: entity work.CommandFifo 
  port map (
	 clock	    => Clk,
	 data		=> iofifo_datain,
	 wrreq		=> iofifo_writereq,
	 rdreq		=> readreq,
	 q		    => Comm_DataIn,
	 almost_full => almost_full,
	 empty		=> empty,
	 full		=> full,
	 usedw		=> usedw
  );

  iofifo_instatus <= x"000"& busy & full & almost_full & empty & "0000000" & full & usedw;

 -- count time since last data in
  process(clk, reset)
  begin
	if( reset='1') then
	  idletime <= TIMEOUT;
    elsif rising_edge(clk) then
      if( iofifo_writereq='1' ) then
	    idletime <= 0;
	  elsif idletime> 5000000 then
  	    idletime <= TIMEOUT;
	  else
	    idletime <= idletime+1;
	  end if;	  
    end if;
  end process;
  
  -- get the first and last words in a transaction
  process(clk, reset)
  begin
	if( reset='1') then
      nextState<=IDLE; 	
	  busy <= '0';
    elsif rising_edge(clk) then
      nextState<=previousState;-- nextState and oldState are registered
      lastwordreceived <='0';
		
      case previousState is             
        when IDLE => 
		  busy <= RX_almostFullFlag;		
          nextState <= WAITFIRST; 
			 
        when WAITFIRST =>-- try to find the first word in a command
          if iofifo_writereq='1' then 
            if( iofifo_datain(31 downto 8)=x"0000_00" and to_integer(unsigned(iofifo_datain))<N_MONITOR_REGS+N_CONTROL_REGS+4 and to_integer(unsigned(iofifo_datain))>2 ) then
 	 	      nextState <= WAITEND;
              firstword <= to_integer(unsigned(iofifo_datain))-1;-- first word is the lenght and it has been read just now
			else 
 	 	      nextState <= WAITING;			
			end if;
		  end if;
			 
	    when WAITEND =>    -- try to find the last word in a command
          if iofifo_writereq = '1' then 
            firstword <= firstword - 1;-- first word is the lenght, so if wrreq still active, -1
		    if( iofifo_datain = ENDMARKER and firstword < 2 ) then-- final word found
 	 	      nextState <= BUSYSTATE;
			  lastwordreceived <= '1';
			elsif (firstword=1) then
			  nextState <= WAITING;			
			end if;
		  elsif idletime>2000 then -- wait 100 us
	        nextState <= WAITING;			
		  end if;
			
		when BUSYSTATE =>   -- be busy writing out the data
		  firstword <= 0;
          busy <= '1';		
		  lastwordreceived <= '0';
		  if idletime+1>=TIMEOUT or done='1' then
  	 	    busy <= '0';		
   	        nextState<=IDLE;
		  end if;
		  
		when WAITING =>     -- ENDMARKER not found or timeout reached
		  busy <= '1';		
		  lastwordreceived <= '1';
		  if idletime+1>=TIMEOUT or (idletime>500 and empty='1') then -- wait full timeout or 10 us and empty fifo
  	 	    busy <= '0';		
   	        nextState<=IDLE;
		  end if;
		  
        when others =>  
		  busy <= '0';		
		  lastwordreceived <= '1';
          nextState<=IDLE;           
      end case;

    end if;
  end process;

  -- at the last word received, start the transfer
  process(clk, reset)
  begin
	if( reset='1') then
      readreq <= '0';
	  done <= '0';
    elsif rising_edge(clk) then

	  done <= empty and readreq;
	  if previousState=BUSYSTATE then
        Comm_Wrreq <= readreq and not(empty);-- writing to fifo rx in local rx, reading from cmd fifo
	  else 
	    Comm_Wrreq <= '0'; -- do not forward bad requests!
	  end if;
	  if( lastwordreceived='1' or (not(empty)='1' and idletime+1>=TIMEOUT) )then-- if I received the last word or a lot of time passed,
        readreq <= not(empty); 
	  elsif empty='1' then
        readreq <= '0';
	  end if;
    end if;
  end process;
  
  -- move to the next state
  process (reset,nextState) 
  begin
    if reset='1' then 
      previousState<=IDLE;
    else
      previousState<=nextState;
    end if;
  end process;
  
end architecture logic;