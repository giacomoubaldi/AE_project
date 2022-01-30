library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

entity Local_RXTX is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	-- Configuration mode from FSM
	DAQ_Config : in std_logic;
	--- Communication with HPS ---
    iofifo_datain         : in   std_logic_vector(31 downto 0);                    -- datain
    iofifo_writereq       : in   std_logic;                                        -- writereq
    iofifo_instatus       : out    std_logic_vector(31 downto 0) := (others => 'X'); -- instatus
    iofifo_regdataout     : out   std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
    iofifo_regreadack     : in   std_logic;                                        -- regreadack
    iofifo_regoutstatus   : out    std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus	
	-- Data From Register File
	Register_DataValid : in std_logic;
	Register_DataRead : in std_logic_vector(31 downto 0);
	-- Data to Register file
	Register_Address : out natural;
	Register_Wrreq : out std_logic;
	Register_Rdreq : out std_logic;
	Register_SelectiveReset : out std_logic_vector(N_CONTROL_REGS-1 downto 0) := (others => '0');
	Register_DataWrite : out std_logic_vector(31 downto 0);
	-- FIFO RX/TX stats
	FifoRX_info : out std_logic_vector(15 downto 0);
	FifoTX_info : out std_logic_vector(15 downto 0)
  );
end entity;

architecture description of Local_RXTX is

  -- FifoRX is used to store data coming from HPS
  component FifoRX is
  port (
    aclr		: in std_logic ;
    clock		: in std_logic ;
    data		: in std_logic_vector (31 downto 0);
    rdreq		: in std_logic ;
    wrreq		: in std_logic ;
    almost_full	: out std_logic ;
    empty		: out std_logic ;
    full		: out std_logic ;
    q			: out std_logic_vector (31 downto 0);
    usedw		: out std_logic_vector (7 downto 0)
  );
  end component;
  
  -- FifoTX is used to store data to be sent to HPS
  -- Read ahead FIFO!
  component FifoTX
  port (
    aclr		: in std_logic ;
    clock		: in std_logic ;
    data		: in std_logic_vector (31 downto 0);
    rdreq		: in std_logic ;
    wrreq		: in std_logic ;
    almost_full	: out std_logic ;
    empty		: out std_logic ;
    full		: out std_logic ;
    q			: out std_logic_vector (31 downto 0);
    usedw		: out std_logic_vector (7 downto 0)
  );
  end component;

  -- Internal Counter
  component Counter_nbitEN
  generic ( nbit : natural := 12);
  port(
    Clock: in std_logic;
    Reset: in std_logic;
	Enable: in std_logic;
    Count : out unsigned (nbit-1 downto 0)
  );
  end component;
  
  -- State
  type state is (idle, waiting, lengthCapture, decode, catchState, changeState,
    writeLength, header1, header2, addressToRead, waitToread, readRegister, footer1, footer2,
    addressToWrite, dataToWrite, writing, resetRegister,readFooter, erase);
	 
  signal present_state, next_state : state := idle;
  
  -- Temporary Stored Informations
  signal sequenceLength : unsigned(6 downto 0) := (others=>'0');
  signal whatToWrite : std_logic_vector (31 downto 0) := (others=>'0');
  signal address : natural := 0;
  -- Counter Signals
  signal counterReset : std_logic;
  signal internalReset : std_logic;
  signal internalEnable : std_logic;
  signal internalCounter : unsigned (6 downto 0);
  -- FifoRX ports
  signal internalRdreq : std_logic;
  signal internalDataRX : std_logic_vector(31 downto 0);
  -- Data for FifoTX
  signal internalDataTX : std_logic_vector(31 downto 0);
  signal internalWrreq : std_logic := '0';

  signal FifoRX_Usage : std_logic_vector (7 downto 0);
  signal FifoRX_Full, FifoRX_AlmostFull, FifoRX_Empty : std_logic;
  signal FifoTX_Usage : std_logic_vector (7 downto 0);
  signal FifoTX_Full, FifoTX_AlmostFull, FifoTX_Empty : std_logic;
  signal FifoTX_PacketReady : std_logic;
  signal FifoRX_PacketReady : std_logic;

  -- Signal used temporary to store the length of the sequence read from Register to write in the LocalTX
  signal lengthToWrite : unsigned (31 downto 0);

begin

  lengthToWrite <= to_unsigned(0,24) & (('0' & sequenceLength)+('0' & sequenceLength)-to_unsigned(1,8)); --sequenceLength*2-1

  -- Components port map
  Fifo_RX : FifoRX
  port map (
    clock => Clock,
	aclr => Reset,
	wrreq => iofifo_writereq,
    data  => iofifo_datain,
	rdreq => internalRdreq,
	q     => internalDataRX,
	empty => FifoRX_Empty,
	almost_full => FifoRX_AlmostFull,
	full => FifoRX_Full,
	usedw => FifoRX_Usage
  );
  FifoRX_info <= FifoRX_Full & FifoRX_AlmostFull & FifoRX_Empty & '0' &
                  "000" & FifoRX_Full & FifoRX_Usage;
  iofifo_instatus <= x"000" & '0' & FifoRX_Full & FifoRX_AlmostFull & FifoRX_Empty & 
                  "0000000" & FifoRX_Full & FifoRX_Usage;
  
  Fifo_TX : FifoTX
  port map (
    clock => Clock,
    aclr  => Reset,
    wrreq => internalWrreq,
    data => internalDataTX,
    rdreq =>  iofifo_regreadack,
    q	=> iofifo_regdataout,
    empty => FifoTX_Empty, -- internalFifoOut_Empty,
	almost_full => FifoTX_AlmostFull,
	full => FifoTX_Full,
	usedw => FifoTX_Usage
  );
  FifoTX_info <= FifoTX_Full & FifoTX_AlmostFull & FifoTX_Empty & FifoTX_PacketReady &
                  "000" & FifoTX_Full & FifoTX_Usage;
  iofifo_regoutstatus <= x"000" & FifoTX_PacketReady & FifoTX_Full & FifoTX_AlmostFull & FifoTX_Empty & 
						 "0000000" & FifoTX_Full & FifoTX_Usage;
    
  Counter : Counter_nbitEN
  generic map( nbit => 7 )   
  port map (
	Clock => Clock,
	Reset => counterReset,
	Enable => internalEnable,
	Count => internalCounter
  );
  
  -- Internal Counter Controls: Enable and Reset --
  internalEnable <= '1' when present_state = waiting or present_state = writeLength or present_State = header1 or present_State = header2 or 
         present_state = addressToRead or present_state = addressToWrite or present_state = dataToWrite or present_state=resetRegister or present_state = erase or present_state = readFooter
                        else '0';
  internalReset <= '1' when present_state = idle or present_state = lengthCapture or present_state = decode
	                   else '0';
  counterReset <= internalReset or Reset;
  
  --/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////--
  
  ----  Process 1 : states flow  --
  
  -- Starts with "Idle" state
  -- When The RX Fifo is not empty waits for some cycles (constant Timeout in DAQ_Package)
  -- Then in "lengthCapture" stores the length of the sequence in the sequenceLength Register
  -- During Decode the machine decides what's the instruction to perform
  -- Case 1: Change the internal state of the FS machine
  --   During catchState the signal of the next_state is stored in "whatToWrite" register
  --   During changeState the whatToWrite register is written at the address 1 (dedicated to next_state)
  -- Case 2 : Read some registers
  --   First the length of the sequence read is stored in the "internal_Fifo"
  --   Then the header1 and the header2 (header are needed to differentiate a register sequence from an event sequence)
  --   During "addressToRead" the address to read is stored in the Address Register
  --   Then waits one clock to be sure the register file provides the right data (waitToRead)
  --   During "readRegister" the data is read and sent to the "internal_Fifo". Then if there are othere registers to read next_state=addressToRead again 
  --   This cycle continues for every address requested by the input sequence. Then writes the footer sequence and then back to idle
  -- Case 3: Write some registers
  --   AddressToWrite: the address is stored in the Address Register
  --   DataToWrite: the data is stored in the "whatToWrite" register
  --   Writing: the data is written at the provided address
  -- Case 4: Reset Registers
  --   Resets the Register at the given address
  --
  -- Then footer sequence is read from the fifo (readFooter state)... it's like erase state
  --
  -- Finally if a Write request or a Reset request arrives when DAQ_Config='0' the instruction is erased
  -- During Erase state the sequence is read till the end without performing any action
  --   
  --//////////////////////////////////////////////////////////////////////////////////////////////////////////////7//////////////////////////////-- 
  
  process(present_state, internalCounter, internalDataRX, DAQ_Config,
    Register_DataValid, sequenceLength, FifoRX_PacketReady)
  begin
    case present_state is
	 
	  when idle =>
	    if FifoRX_PacketReady='1' then-- FIFORX non vuota e Internal_Fifo vuota
	      next_state <= waiting;
	    else
	      next_state <= idle;
	    end if;
	    
	  when waiting =>
	    if internalCounter >= to_unsigned(Timeout-1,12) then
	      next_state <= lengthCapture;
	    else
	      next_state <= waiting;
	    end if;
	  
	  when lengthCapture =>-- The first 32 bit word is the lenght in words of the message
	    next_state <= decode;
	  
	  when decode =>-- It decides what's the instruction to perform based on second 32 bit word
	    if internalDataRX = Header_ChangeState then
	      next_state <= catchState;
	    elsif internalDataRX = Header_ReadRegs then
	      next_state <= writeLength;
	    elsif internalDataRX = Header_WriteRegs then
          if DAQ_Config = '1' then-- Check if the change of settings is allowed
	  	    next_state <= addressToWrite;-- Yes, go on
	      else
	  	    next_state <= erase;-- NO, I'll accept the data, but then I'll discard them all
	  	  end if;
	    elsif internalDataRX = Header_ResetRegs then
	      if DAQ_Config = '1' then-- see above
	        next_state <= resetRegister;
	      else
	  	    next_state <= erase;
	  	  end if;
	    else
	      next_state <= idle;
	    end if;
	  
	  when catchState =>
	    next_state <= changeState;
	    
	  when changeState =>
	    next_state <= readFooter;
	  
	  when writeLength =>
	    next_state <= header1;
	  
	  when header1 =>
	    next_state <= header2;
	  
	  when header2 =>
	    next_state <= addressToRead;
	  
	  when addressToRead =>
	    next_state <= waitToRead;
	  
	  when waitToRead =>
	    if Register_DataValid = '1' then
	      next_state <= readRegister;
	    else
	      next_state <= waitToRead;
	    end if;
	  
	  when readRegister =>
	    if internalCounter >= sequenceLength then
	      next_state <= footer1;
	    else
	      next_state <= addressToRead;
	    end if;
	  
	  when footer1 =>
	    next_state <= footer2;
	  
	  when footer2 =>
	    next_state <= readFooter;
	  
	  when addressToWrite =>
	    next_state <= dataToWrite;
	  
	  when dataToWrite =>
	    next_state <= writing;
	  
	  when writing =>
	    if internalCounter >= sequenceLength-to_unsigned(4,32) then
	      next_state <= readFooter;
	    else
	      next_state <= addressToWrite;
	    end if;
	  
      when resetRegister =>
	    if internalCounter >= sequenceLength-to_unsigned(4,32) then
	      next_state <= readFooter;
	    else
	      next_state <= resetRegister;
	    end if;
	  
	  when erase =>
	    if internalCounter >= sequenceLength-to_unsigned(3,32) then
	      next_state <= idle;
	    else
	      next_state <= erase;
	    end if;
	  
	  when readFooter =>
	    next_state <= idle;
	  
	  when others =>
	    next_state <= idle;
	      
    end case;
  end process;
  
  -- process 2 : It manages the signals to and from the register file and the Internal_Fifo
  process(present_State, address, Register_DataRead, Register_DataValid, whatToWrite, lengthToWrite)
  begin

	Register_Wrreq <= '0';
	Register_Rdreq <= '0';
	Register_Address <= 0;
	Register_DataWrite <= (others=>'0');
	internalDataTX <= (others=>'0');
	internalWrreq <= '0';

    case present_state is
	  
	  when idle =>
	    
	  when waiting =>  	       -- Init Sequence
	  
	  when lengthCapture =>
	    
	  when decode =>
	    
	  when catchState =>       -- Change State sequence
	   		  
	  when changeState =>
	    Register_Wrreq <= '1';
	    Register_Address <= 1;
	    Register_DataWrite <= whatToWrite;
	    
       -- Read registers sequence
	  when writeLength =>
	    internalDataTX <= std_logic_vector(lengthToWrite);
	    internalWrreq <= '1';
	  
	   when header1 =>
	    internalDataTX <= Header1_RF;
	    internalWrreq <= '1';
	  
	  when header2 =>
	    internalDataTX <= Header2_RF;
	    internalWrreq <= '1';
	  
	  when addressToRead =>
	  
	  when waitToRead =>
	    Register_Rdreq <= '1';
	    Register_Address <= address;
	    internalDataTX <= std_logic_vector(to_unsigned(address,32));
	    if Register_DataValid = '1' then
	      internalWrreq <= '1';
	    else
	      internalWrreq <= '0';
	    end if;
	  
	   when readRegister =>
	    Register_Rdreq <= '1';
	    Register_Address <= address;
	    internalDataTX <= Register_DataRead;
	    internalWrreq <= '1';
	    
	   when footer1 =>
	    internalDataTX <= Footer1_RF;
	    internalWrreq <= '1';
	  
	  when footer2 =>
	    internalDataTX <= Footer2_RF;
	    internalWrreq <= '1';
       
	  when addressToWrite =>	-- Write sequence
	    
	  when dataToWrite =>
	    
	  when writing =>
	    Register_Wrreq <= '1';
	    Register_Address <= address;
	    Register_DataWrite <= whatToWrite;
	    
      when resetRegister =>		-- Reset Registers sequence
	    
	  when erase =>				-- Erase an "impossible to perform" action 
	  
	  when readFooter =>
	  
	  when others =>
	    
	end case;
  end process;
  
  -- Before which states do I have to read my RX Fifo?
  internalRdreq <= '1' when next_state = lengthCapture or next_state = decode or next_state=catchState or next_state = addressToRead or
     next_state = addressToWrite or next_state = dataToWrite or next_state = resetRegister or next_state = erase or next_state = readFooter
	                   else '0';
	
  -- Process number 3 : sequential part	plus reset
  process(Clock,Reset)
  begin
    if Reset = '1' then
	  present_state <= idle;
	  sequenceLength <= to_unsigned(0,7);
	  whatToWrite <= (others=>'0');
	  Address <= 0;
	  
	elsif rising_edge(clock) then

	  -- During LengthCapture length can be read from FifoRx and it is stored in the sequenceLength Register
	  if present_state = lengthCapture then
	    sequenceLength <= unsigned(internalDataRX(6 downto 0));
	  end if;
	  -- The address is taken from FifoRx and stored in the Address Register before read/write
	  if present_state = addressToRead or present_state = addressToWrite then
	    Address <= to_integer(unsigned(internalDataRX));
	  end if;
      -- The data I have to write are stored in the "what to write" register before a write or a change of state
	  if present_state = dataToWrite or present_State = catchState then
	    whatToWrite <= internalDataRX;
	  end if;
      -- The address is decoded to reset the right register using Selective_Reset
	  if present_state = resetRegister then
	    for I in 0 to N_CONTROL_REGS-1 loop
	      if to_integer(unsigned(internalDataRX)) = I then
	        Register_SelectiveReset(I) <= '1';
	  	  else
	  	    Register_SelectiveReset(I) <= '0';
	      end if;
	    end loop;
	  elsif present_state = idle then
	    Register_SelectiveReset <= (others=>'0');
      end if;
      
	  present_state <= next_state;
	  
    end if;
  end process; 
  
  -- Process number 4 : define when a packet is ready in the FIFOrX
  process(Clock,Reset)
  begin
    if Reset = '1' then
	  FifoRX_PacketReady <= '0';
	elsif rising_edge(clock) then
      if present_state=idle and iofifo_writereq='1' and iofifo_datain=Instruction_Footer then
	    FifoRX_PacketReady <= '1';
	  elsif FifoRX_Empty='1' then
	    FifoRX_PacketReady <= '0';
      end if;
    end if;
  end process;  
  
  -- Process number 5 : define when a packet is ready in the FIFOTX
  process(Clock,Reset)
  begin
    if Reset = '1' then
	  FIFOTX_PacketReady <= '0';
	elsif rising_edge(clock) then
      if present_state=footer2 then
	    FIFOTX_PacketReady <= '1';
	  elsif FifoTX_Empty='1' then
	    FIFOTX_PacketReady <= '0';
      end if;
    end if;
  end process;  
  

end architecture;	 