library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


entity FrameReceiver is
  --Port declaration
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
    -- store or not a frame?
	DoStore : in std_logic;
    --Input stream
	Data_StreamIn : in unsigned (31 downto 0);  
	Data_ValidIn : in std_logic;
    EndOfEventIn : in std_logic;
	-- Time tags
	TSClock : in unsigned(31 downto 0);
	ClockCounter : in unsigned(47 downto 0);
    -- 
	SendPacket : in std_logic;
	-- Outputs
	-- Stream
    DataAvailable : out std_logic;
	Data_StreamOut : out unsigned (31 downto 0);  
	Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
	Data_Size : out unsigned (11 downto 0);  
    -- Time tags of the first word
	ClockCounterOut : out unsigned(47 downto 0);
	FifoFull : out std_logic
  );
end entity;


architecture algorithm of FrameReceiver is
  
  component FrameFifo
	port (
		aclr		: in std_logic ;
		clock		: in std_logic ;
		data		: in std_logic_vector (31 downto 0);
		rdreq		: in std_logic ;
		wrreq		: in std_logic ;
		almost_full	: out std_logic ;
		empty		: out std_logic ;
		full		: out std_logic ;
		q		    : out std_logic_vector (31 downto 0);
		usedw		: out std_logic_vector (12 downto 0)
	);
  end component;
  
  component FrameSizeFifo
	port (
		aclr		: in std_logic ;
		clock		: in std_logic ;
		data		: in std_logic_vector (91 downto 0);
		rdreq		: in std_logic ;
		wrreq		: in std_logic ;
		empty		: out std_logic ;
		full		: out std_logic ;
		q		    : out std_logic_vector (91 downto 0);
		usedw		: out std_logic_vector (7 downto 0)
	);
  end component;

  --
  type state is (idle,counting,ending);
  
  ----  Internal signals  ----  
  signal present_state, next_state : state := idle;
  signal counts : unsigned(11 downto 0);
  --signal countsReg : unsigned(11 downto 0);
  signal TSClockReg, TSClockOut : unsigned(31 downto 0);
  signal ClockCounterReg : unsigned(47 downto 0);
  signal DoStoreSampled : std_logic;
  signal DataWrite : std_logic;  -- to write a frame data into the fifo
  signal TSSizeData, TSSizeDataOut : std_logic_vector (91 downto 0);
  signal TSSizeWrite : std_logic;
  signal BusyFrame, BusySize, FrFifoFull : std_logic;
  signal FrSizeEmpty, FrFifoEmpty: std_logic;
  -- Output State Machine
  type stateOut is (idleSt,sizeSt,timeStampSt,lsbClkCountSt,msbClkCountSt,countingSt,endingSt);
  signal currentState, nextState : stateOut := idleSt;
  signal DataRequest, readAcknowledge : std_logic;
  signal Data_StreamInternal : std_logic_vector (31 downto 0);  
  signal Data_SizeInt, sizeSampled : unsigned(11 downto 0);
  signal locCounts : unsigned(11 downto 0);
  signal DataAvailableInt : std_logic;
begin

  ------------------------------------ Process INPUT
  -- FSM -- evaluate the next state
  
  process(present_state, Data_ValidIn, EndOfEventIn)
  begin
    case present_state is 
	 
      when idle => 
        if Data_ValidIn = '1' then     
          next_state <= counting; 
		else
 		  next_state <= idle; 
        end if;
		  
	  when counting =>
        if EndOfEventIn='1' then
          next_state <= ending;
		else
          next_state <= counting;
		end if;
		
	  when ending =>                                      
        next_state <= idle;
		
	  when others =>                                      
        next_state <= idle;
		  
    end case;
  end process;

  -- FSM Sequential part
  
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  present_state<=idle;
      counts <= x"000";
--      countsReg <= x"000";
	  TSClockReg <= (others=>'0');
	  ClockCounterReg <= (others=>'0');
      DoStoreSampled <= '0';
    elsif rising_edge(Clock) then
      present_state <= next_state;
	  if present_state = idle then
	    counts <= x"002";   -- =2 to count also the first and the last words!!
 	    TSClockReg <= TSClock;
	    ClockCounterReg <= ClockCounter;
		DoStoreSampled <= DoStore;
	  elsif present_state = counting and Data_ValidIn='1' and FrFifoFull='0' then -- check for FrFifoFull...
	    counts <= counts+1;
	  elsif present_state = ending then
--        countsReg <= counts;		
	  end if;

    end if;
  end process;


  -- Data Storage in FIFOs
  FrFifo: FrameFifo
	port map (
		aclr => Reset,
		clock => Clock,
		wrreq => DataWrite, 
		data  => std_logic_vector(Data_StreamIn),
		rdreq => DataRequest, --		: in std_logic ;
		q	  => Data_StreamInternal,
		almost_full	=> BusyFrame,
		empty		=> FrFifoEmpty,
		full		=> FrFifoFull,
		usedw		=> open --: out std_logic_vector (12 downto 0)
	);
  DataWrite <= Data_ValidIn and DoStoreSampled;

  -- Store Size, ClockCounter and TimeStamp
  FrSizeFifo: FrameSizeFifo
	port map (
		aclr => Reset,
		clock => Clock,
		wrreq => TSSizeWrite, 
		data  => TSSizeData,
		rdreq => readAcknowledge,
		q	  => TSSizeDataOut,
		empty => FrSizeEmpty,
		full  => BusySize,
		usedw => open --	: out std_logic_vector (7 downto 0)
	);


  -- Manage output stream and output data
  
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  TSSizeData <= (others=>'0');
	  TSSizeWrite <= '0';
	  
    elsif rising_edge(Clock) then
	  TSSizeWrite <= '0';
      if EndOfEventIn='1' then
	    TSSizeData <=  std_logic_vector(ClockCounterReg & counts & TSClockReg);
		TSSizeWrite <= DoStoreSampled;  
	  end if;
    end if;
  end process;
  
  Data_SizeInt <= unsigned(TSSizeDataOut(43 downto 32))+x"004";
  TSClockOut <= unsigned(TSSizeDataOut(31 downto 0)); 
  ClockCounterOut <= unsigned(TSSizeDataOut(91 downto 44));
  Data_Size <= Data_SizeInt;
  
  FifoFull <= BusyFrame or BusySize;
  
  -- Extracting the data!
  ------------------------------------ Process OUTPUT
  -- FSM -- evaluate the next state
  process(currentState, SendPacket, DataRequest, Data_StreamInternal, Data_SizeInt, TSSizeDataOut, TSClockOut)
  begin
    case currentState is 
	 
      when idleSt => 
        if SendPacket = '1' then     
          nextState <= sizeSt; 
		  Data_ValidOut <= '1';
		  Data_StreamOut <= x"0000_0" & Data_SizeInt;
		else
 		  nextState <= idleSt; 
		  Data_ValidOut <= '0';
		  Data_StreamOut <= (others=>'0');
        end if;

	  when sizeSt =>
	    nextState <= timeStampSt;
        Data_ValidOut <= '1';
		Data_StreamOut <= TSClockOut;
		  
	  when timeStampSt =>
	    nextState <= lsbClkCountSt;
        Data_ValidOut <= '1';
		Data_StreamOut <= unsigned(TSSizeDataOut(75 downto 44));
		
	  when lsbClkCountSt =>
	    nextState <= msbClkCountSt;
        Data_ValidOut <= '1';
		Data_StreamOut <= unsigned(x"0000" & TSSizeDataOut(91 downto 76));
		
	  when msbClkCountSt =>
	    nextState <= countingSt;
        Data_ValidOut <= '1';
		Data_StreamOut <= unsigned(Data_StreamInternal);
		
	  when countingSt =>
        if DataRequest='0' then  -- ???
          nextState <= endingSt;
          Data_ValidOut <= '1';
		  Data_StreamOut <= unsigned(Data_StreamInternal);
		else
          nextState <= countingSt;
          Data_ValidOut <= '1';
		  Data_StreamOut <= unsigned(Data_StreamInternal);
		end if;
		
	  when endingSt =>                                      
        nextState <= idleSt;
        Data_ValidOut <= '0';
		Data_StreamOut <= (others=>'0');
		
	  when others =>                                      
        nextState <= idleSt;
        Data_ValidOut <= '0';
		  
    end case;
  end process;

  -- FSM OUT Sequential part
  
  process(Clock, Reset)
    variable DataRequestOld : std_logic;
  begin
    if Reset = '1' then
	  currentState<=idleSt;
      locCounts <= x"000";
	  EndOfEventOut <= '0';
	  readAcknowledge <= '0';
	  DataRequest <= '0';
	  sizeSampled <= (others=>'0');
	  DataAvailableInt <= '0';
	  DataAvailable <= '0';
	  DataRequestOld := '0';
    elsif rising_edge(Clock) then
      currentState <= nextState;
	  if currentState = idleSt then
	    locCounts <= x"007";   -- =.... to count also the first and the last words!!
		sizeSampled <= Data_SizeInt;
	  elsif currentState = countingSt then -- check for FrFifoFull...
	    locCounts <= locCounts+1;
	  end if;
	  
	  if ((currentState=timeStampSt) or (currentState=msbClkCountSt) or (currentState=lsbClkCountSt) or 
	     ((currentState=countingSt) and (locCounts<sizeSampled) )) and FrFifoEmpty='0' then
	    DataRequest <= '1';
	  else 
	    DataRequest <= '0';
	  end if;
	  
	  -- provide the fifo size acknowledge while reading....
	  if currentState = msbClkCountSt then  
        readAcknowledge <= '1';  
	  else 
        readAcknowledge <= '0';	  
	  end if;

	  -- send the info that a frame has been read fully
      if DataRequestOld='1' and DataRequest='0' then
        EndOfEventOut <= '1';  
	  else 
        EndOfEventOut <= '0';	  
	  end if;
	  
	  DataAvailable <= DataAvailableInt;
      DataAvailableInt <= not FrSizeEmpty;	  
      DataRequestOld := DataRequest;
    end if;
  end process;
  
end architecture;