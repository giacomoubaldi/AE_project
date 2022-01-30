library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

--
-- FramePointerMemory Entity
--
-- 1. It stores in a DPRam the (ClockCounter, DataSize and DDR3Pointer)
--    for each frame (note: ClockCounter should come in a strictly ascending order)
-- 2. It gets a ClockCounter range to check and
-- 3. it provides the min and max DDR3Pointer for the frames with a ClockCounter value 
--    INSIDE the provided range. 
--    Note: DDR3PointerMin points to the first entry, 
--          DDR3PointerMax points to the value AFTER the last valid entry
--    Note2: DDR3PointerMax might point after the valid memory range. In this case, wrap it up!
-- 3'. It provides also the total Data Size.
-- 4. Unused entries in the DPRam (those with ClockCounter<ClockCounterMin) are removed
-- 5. When MemoryFull='1' the oldest entry is removed; writing is always safe!

entity FramePointerMemory is
  port (
    Clock : in std_logic;
	Reset : in std_logic;
	
    -- Interface with FourFrameReceiver
	-- Writing Frame tags (ClockCounter, DataSize and DDR3Pointer) to Fifo
	MemoryEmpty : out std_logic;
	MemoryFull  : out std_logic;
    ClockCounterIn : in unsigned (47 downto 0);   	
	Data_SizeIn : in unsigned (11 downto 0);  
    DDR3Pointer : in unsigned(31 downto 0);
	WriteRequest : in std_logic;
    DDR3PointerMinRam : out unsigned(31 downto 0);
    -- Interface with TriggerControl
    -- Compare with the trigger ClockCounter (-preTrigger +afterTrigger)
	EvaluateRange : in std_logic;
    ClockCounterMin : in unsigned (47 downto 0);   	
    ClockCounterMax : in unsigned (47 downto 0);   	
	RangeReady : out std_logic;
    DDR3PointerMin : out unsigned(31 downto 0);
    DDR3PointerMax : out unsigned(31 downto 0);
	RangeSize : out unsigned (15 downto 0);
	--
	FPMDebug : out std_logic_vector(15 downto 0)
  );	
end entity;


architecture algorithm of FramePointerMemory is
 
  component FrameDPRam is
	port (
		clock		: in std_logic  := '1';
		data		: in std_logic_vector (91 downto 0);
		rdaddress		: in std_logic_vector (10 downto 0);
		wraddress		: in std_logic_vector (10 downto 0);
		wren		: in std_logic  := '0';
		q		: out std_logic_vector (91 downto 0)
	);
  end component FrameDPRam;
  signal DataToWrite : std_logic_vector(91 downto 0);
  signal wraddress : std_logic_vector (10 downto 0);
  signal rdaddress : std_logic_vector (10 downto 0);
  signal nextRdaddress : std_logic_vector (10 downto 0);
  signal tailAddress : std_logic_vector (10 downto 0);
  signal wrWrap, rdWrap, nextRdWrap, tlWrap : std_logic;
  signal DataRead : std_logic_vector(91 downto 0);
  signal ClockCounterRd : unsigned(47 downto 0);
  signal Data_SizeRd : unsigned(15 downto 0);
  signal DDR3PointerRd : unsigned(31 downto 0);
  signal DDR3PointerMaxInt : unsigned(31 downto 0);
  signal DDR3PointerRdOld : unsigned(31 downto 0);
  signal Data_SizeRdOld : unsigned(15 downto 0);
  constant LASTADDRESS : std_logic_vector(10 downto 0) := "11111111111";
  signal minFound, maxFound : std_logic;
  signal noMoreData,doRead : std_logic;
  type State_t is (idle, searchmin, searchmax, ending, noData);
  signal currentState, nextState : State_t;
  signal EvalRange, EvalRangeOld : std_logic;
  signal MemoryFullInt : std_logic;
  signal RangeSizeInt : unsigned (15 downto 0);
  signal MemoryEmptyInt : std_logic;
  signal RangeReadyInt : std_logic;
  signal stateVal : std_logic_vector(2 downto 0) := (others=>'0');

begin

 FDPRam: FrameDPRam 
  port map (
    clock => Clock,
	data  => DataToWrite,
	wren => WriteRequest, 
	wraddress => wraddress, 
	q		  => DataRead,
	rdaddress => rdaddress
  );

  DataToWrite <=  std_logic_vector(Data_SizeIn) & std_logic_vector(ClockCounterIn) & std_logic_vector(DDR3Pointer);		
  MemoryEmptyInt <= '1' when wraddress=tailAddress and wrWrap=tlWrap
                 else '0';
  MemoryEmpty <= MemoryEmptyInt;
  MemoryFullInt <=  '1' when (wraddress=tailAddress) and not(wrWrap=tlWrap)
                 else '0';				 
  MemoryFull <= MemoryFullInt;

  Data_SizeRd <= "0000" & unsigned(DataRead(91 downto 80));
  ClockCounterRd <= unsigned(DataRead(79 downto 32));
  DDR3PointerRd <= unsigned(DataRead(31 downto 0));
  


  WRProc: -- Update writing address
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  wrWrap <= '0';
	  wraddress <= (others=>'0');
    elsif rising_edge(Clock) then
      if WriteRequest='1' then
	    if wraddress= LASTADDRESS then
		  wraddress <= (others=>'0');
          wrWrap <= not wrWrap;
		else
		  wraddress <= std_logic_vector(unsigned(wraddress)+1);
		end if;
	  end if;

    end if;	
  end process;
     
   -- FSM -- evaluate the next state
  process(currentState, EvalRange, MemoryEmptyInt, minFound, maxFound, noMoreData)
  begin
    case currentState is 
	 
      when idle => 
	    stateVal <= "000";
        if EvalRange = '1' then     
          if MemoryEmptyInt='0' then
            nextState <= searchMin; 
		  else
		    nextState <= noData;
		  end if;
		else
 		  nextState <= idle; 
        end if;

	  when searchMin =>
	    stateVal <= "001";
        if minFound = '1' then     
          nextState <= searchMax; 
		elsif noMoreData='1' then
		  nextState <= ending;
		else
 		  nextState <= searchMin; 
        end if;
		  
      when searchMax =>
	    stateVal <= "010";
        if maxFound = '1' or noMoreData='1' then     
          nextState <= ending; 
		else
 		  nextState <= searchMax; 
        end if;

      when ending =>
	    stateVal <= "011";
	    nextState <= idle;

      when noData =>
	    stateVal <= "100";
	    nextState <= idle;

      when others =>
	    stateVal <= "101";
	    nextState <= idle;

	end case;
  end process;
 
  noMoreData <= '1' when rdaddress=wraddress and rdWrap=wrWrap else '0';

   -- FSM Sequential part
   -- read pointer
  
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  rdaddress <= (others=>'0');
	  rdWrap <= '0';
	  doRead <= '0';
	  
    elsif rising_edge(Clock) then

      currentState <= nextState;
      doRead <= '0';

	  if currentState = idle then

		rdWrap <= tlWrap;
		rdaddress <= tailAddress;
		nextRdWrap <= tlWrap;
		nextRdaddress <= tailAddress;
        if WriteRequest='1' and MemoryFullInt='1' then
	      if tailAddress= LASTADDRESS then
		    rdaddress <= (others=>'0');
            rdWrap <= not tlWrap;
		    nextRdaddress <= "00000000001";
            nextRdWrap <= not tlWrap;
		  else
		    rdaddress <= std_logic_vector(unsigned(tailAddress) +1);
            if std_logic_vector(unsigned(tailAddress)+1)=LASTADDRESS then
			  nextRdaddress <= (others=>'0');
              nextRdWrap <= not tlWrap;
            else 
			  nextRdaddress <= std_logic_vector(unsigned(tailAddress) +2);
			end if; 
		  end if;
        end if;
      elsif (currentState = searchMin or currentState = searchMax) and maxFound='0' then
        doRead <= '1';
	    if nextRdAddress=wrAddress and not(nextRdWrap=wrWrap) then 
		  -- no more data to read....
		  --....
--??		  noMoreData <= '1';---??
		elsif noMoreData = '0' then 
		  rdAddress <= nextRdAddress;
		  rdWrap <= nextRdWrap;
		  
		  if nextRdAddress= LASTADDRESS then
		    nextRdaddress <= (others=>'0');
            nextRdWrap <= not nextRdWrap;
		  else
		    nextRdaddress <= std_logic_vector(unsigned(nextRdaddress) +1);
		  end if;
		end if;
      elsif currentState=ending then
	    rdWrap <= tlWrap;
		rdaddress <= tailAddress;
	  end if;

    end if;
  end process;
  
  -- summing up data
  process(Clock, Reset)
  begin
    if Reset = '1' then
	  minFound <= '0';
	  maxFound <= '0';
	  RangeReadyInt <= '0';
	  DDR3PointerMin <= (others=>'0');
	  DDR3PointerMax <= (others=>'0');
	  DDR3PointerRdOld <= (others=>'0');
	  Data_SizeRdOld <= (others=>'0');
	  RangeSizeInt <= (others=>'0');
      EvalRange <= '0';
      EvalRangeOld <= '0';	   
    elsif rising_edge(Clock) then
 
      if currentState = idle then
	    RangeReadyInt <= '0';
        if EvalRange = '1' then
	      RangeSizeInt <= (others=>'0');	  
		end if;
	  elsif currentState=ending then
        RangeReadyInt <= '1';
        if maxFound='0' then --??
	      DDR3PointerMaxInt <= DDR3PointerMaxInt+RangeSizeInt;
	      DDR3PointerMax <= DDR3PointerRdOld+Data_SizeRdOld;
		end if;
		minFound <='0';
		maxFound <='0';
      elsif currentState=noData then
        RangeReadyInt <= '1';
		DDR3PointerMin  <= (others=>'0');
        DDR3PointerMax <= (others=>'0');
	    DDR3PointerMaxInt <= (others=>'0');
		minFound <='0';
		maxFound <='0';
	  elsif doRead='1' and maxFound='0' then
	    if ClockCounterRd>=ClockCounterMin and ClockCounterRd<ClockCounterMax then
		  RangeSizeInt <= RangeSizeInt+Data_SizeRd; 
          if minFound='0' then
		    DDR3PointerMin <= DDR3PointerRd;
		    minFound <='1';
		    DDR3PointerMaxInt <= DDR3PointerRd;
          end if;		  
		elsif ClockCounterRd>=ClockCounterMax and minFound='1' then
		  maxFound <='1';
	      DDR3PointerMaxInt <= DDR3PointerMaxInt+RangeSizeInt;
	      DDR3PointerMax <= DDR3PointerRdOld+Data_SizeRdOld;
		elsif noMoreData='1' then
		  maxFound <='1';
		end if;
	  end if;
	  
	  if EvaluateRange='1' and EvalRangeOld='0' then
	    EvalRange <= '1';
	  else 
	    EvalRange <= '0';
	  end if;
	  EvalRangeOld <= EvaluateRange;

  	  DDR3PointerRdOld <= DDR3PointerRd;
      Data_SizeRdOld <= Data_SizeRd;

    end if;	
  end process;
  
  RangeSize <= RangeSizeInt;
  RangeReady <= RangeReadyInt;
  
  -- Update the tail pointer
  -- keep the DPRAM updated with the minimum admount of entries
  -- i.e. remove old entries
  
  process(Clock, Reset)
    variable tailAddressCand0, tailAddressCand1 : std_logic_vector (10 downto 0);
    variable tlWrapCand0, tlWrapCand1 : std_logic;
  begin
    if Reset = '1' then
	  tailAddress <= (others=>'0');
	  tlWrap <= '0';
	  tailAddressCand0 := (others=>'0');
	  tlWrapCand0 := '0';
	  tailAddressCand1 := (others=>'0');
	  tlWrapCand1 := '0';
	  
    elsif rising_edge(Clock) then

      if WriteRequest='1' and MemoryFullInt='1' then
        if tailAddress= LASTADDRESS then
          tailAddress <= (others=>'0');
          tlWrap <= not tlWrap;
	    else
	      tailAddress <= std_logic_vector(unsigned(tailAddress) +1);
	    end if;
	  elsif minFound='0' and currentState=searchMin then
		tailAddress <= tailAddressCand1;
		tlWrap <= tlWrapCand1;
      end if;

	  tailAddressCand1 := tailAddressCand0;
      tlWrapCand1 := tlWrapCand0;
	  tailAddressCand0 := rdAddress;
	  tlWrapCand0 := rdWrap;

    end if;
  end process;
  
  -- --------------------------------------------
  -- Define DDR3PointerMinRam
  -- --------------------------------------------
  -- when running equal to the min of the current event out
  -- when no events equal to the min of the DPRam
  --
   process(Clock, Reset, DDR3Pointer)
    variable tailAddressCand0, tailAddressCand1 : std_logic_vector (10 downto 0);
    variable tlWrapCand0, tlWrapCand1 : std_logic;
  begin
    if Reset = '1' then
	  DDR3PointerMinRam <= DDR3Pointer;
	  
    elsif rising_edge(Clock) then

      if MemoryEmptyInt='1' then
	    DDR3PointerMinRam <= DDR3Pointer;
	  elsif currentstate=idle and nextState=idle and tailAddress=rdAddress then
        DDR3PointerMinRam <= DDR3PointerRd;
      end if;

    end if;
  end process;
  
  FPMDebug(12 downto 0)  <= wrWrap  
       & rdWrap & tlWrap & noMoreData & doRead 
	   & MemoryFullInt & MemoryEmptyInt & maxFound & minFound 
	   & EvaluateRange & stateVal;
  -- Verification part
  FPMDebug(13) <= '1' when EvaluateRange='1' and MemoryEmptyInt='1' else '0';
  FPMDebug(14) <= '1' when EvaluateRange='1' and  ClockCounterMin>=ClockCounterMax else '0';
--  errors(2) <= '1' when RangeReadyInt='1' and DDR3PointerMin>=DDR3PointerMax else '0';
--  errors(3) <= '1' when RangeReadyInt='1' and RangeSize=(others=>'0') else '0';
--  errors(4) <= '1' when RangeReadyInt='1' and DDR3PointerMin+RangeSize\=DDR3PointerMax else '0';
  FPMDebug(15) <= '1' when doRead='1' and EvalRange='1' else '0';
  
end architecture;
 