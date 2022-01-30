library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

entity FourFrameReceiver is
  --Port declaration
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	--
	SensorEnableIn : in std_logic_vector(N_sensors-1 downto 0);
    -- store or not a frame?
	DoStore : in std_logic;
    --Input stream
	Data_StreamIn : in NSensStreamData_t;
	Data_ValidIn : in std_logic_vector(N_sensors-1 downto 0);
    EndOfEventIn : in std_logic_vector(N_sensors-1 downto 0);
	-- Time tags
	TSClock : in unsigned(31 downto 0);
	ClockCounter : in unsigned(47 downto 0);
    --
	SendPacket : in std_logic;
	--WaitRequest : in std_logic;
	-- Outputs
	-- Stream
    DataAvailable : out std_logic;
	Data_StreamOut : out unsigned (31 downto 0);
	Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
	Data_Size : out unsigned (11 downto 0);
    -- Time tags of the first word
	ClockCounterOut : out unsigned(47 downto 0);
	FifoFull : out std_logic_vector(N_sensors-1 downto 0)
  );
end entity;


architecture structural of FourFrameReceiver is

  component FrameReceiver is
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
  end component;

  component StreamSelector is
  port(
    Clock: in std_logic;
    Reset: in std_logic;
    -- Input TS
	TSValid : in std_logic_vector(3 downto 0);
	TSBX0 : in unsigned(47 downto 0);
	TSBX1 : in unsigned(47 downto 0);
	TSBX2 : in unsigned(47 downto 0);
	TSBX3 : in unsigned(47 downto 0);
	-- Output
	SelectionValid : out std_logic;
	Selector : out std_logic_vector(1 downto 0);
	TSBXSelected : out unsigned(47 downto 0)
  );
  end component StreamSelector;


  signal SendPacketInt : std_logic_vector(N_Sensors-1 downto 0);
  signal DataAvailableInt : std_logic_vector(N_Sensors-1 downto 0);
  signal Data_StreamInt : NSensStreamData_t;
  signal Data_ValidInt : std_logic_vector(N_Sensors-1 downto 0);
  signal EndOfEventInt : std_logic_vector(N_Sensors-1 downto 0);
  signal DoStoreV : std_logic_vector(N_Sensors-1 downto 0);
  signal Data_ValidInMasked : std_logic_vector(N_Sensors-1 downto 0);

  signal TSValid : std_logic_vector(N_Sensors-1 downto 0);
  signal SelectionValidInt : std_logic;
  signal SelectorInt, SelectorReg : std_logic_vector(1 downto 0);
  signal TSBXSelectedInt : unsigned(47 downto 0);
  signal ClockCounterInt : NSensTSLong_t;
  type DataSizArr_t is array(N_Sensors-1 downto 0) of unsigned(11 downto 0);
  signal Data_SizeInt : DataSizArr_t;

  -- FSM: register the start of transfer and do not change selection till the end of packet
  type trasferState_t is (idle, transferring, ending);
  signal currentState, nextState : trasferState_t := idle;
  signal EndOfEventReg : std_logic;

begin

  doStoreV <= SensorEnableIn when DoStore='1' else
              "0000";
  Data_ValidInMasked <= Data_ValidIn and SensorEnableIn;

  Sens:
  for i in 0 to N_Sensors-1 generate
    FS: FrameReceiver
	  port map (
        clock   => Clock,
        Reset  	=> Reset,
        --
    	DoStore => DoStoreV(i),
    	--
    	Data_StreamIn => Data_StreamIn(i),
    	Data_ValidIn  => Data_ValidInMasked(i),
    	EndOfEventIn  => EndOfEventIn(i),
    	-- Time tags
    	TSClock => TSClock,
    	ClockCounter => ClockCounter,
    	-- Outputs
    	SendPacket => SendPacketInt(i),
    	DataAvailable => DataAvailableInt(i),
    	Data_StreamOut => Data_StreamInt(i),
    	Data_ValidOut => Data_ValidInt(i),
        EndOfEventOut => EndOfEventInt(i),
    	Data_Size => Data_SizeInt(i),
    	ClockCounterOut => ClockCounterInt(i),
    	FifoFull => FifoFull(i)
	  );
    end generate;

  TSValid <= DataAvailableInt and SensorEnableIn;

  SS: StreamSelector
  port map(
    clock   => Clock,
    Reset  	=> Reset,
    -- Input TS
	TSValid => TSValid,
	TSBX0 => ClockCounterInt(0),
	TSBX1 => ClockCounterInt(1),
	TSBX2 => ClockCounterInt(2),
	TSBX3 => ClockCounterInt(3),
	-- Output
	SelectionValid => SelectionValidInt,
	Selector => SelectorInt,
	TSBXSelected => TSBXSelectedInt
  );

  ------------------------------------ Process INPUT
  -- FSM -- evaluate the next state

  process(currentState, SendPacket, EndOfEventReg)
  begin
    case currentState is

      when idle =>
        if SendPacket = '1' then
          nextState <= transferring;
		else
 		  nextState <= idle;
        end if;

	  when transferring =>
        if EndOfEventReg='1' then
          nextState <= ending;
		else
          nextState <= transferring;
		end if;

	  when ending =>
        nextState <= idle;

	  when others =>
        nextState <= idle;

    end case;
  end process;

  -- FSM Sequential part

  process(Clock, Reset)
  begin
    if Reset = '1' then
	  currentState <= idle;
	  SelectorReg <= "00";
	  ClockCounterOut <= (others=>'0');
      DataAvailable  <= '0';
    elsif rising_edge(Clock) then
      currentState <= nextState;
	  if currentState = idle then
	    SelectorReg <= SelectorInt;
        ClockCounterOut <= TSBXSelectedInt;
		if SelectionValidInt='1' then
          DataAvailable  <= DataAvailableInt(to_integer(unsigned(SelectorReg)));
		end if;
	  end if;
	  if currentState = transferring or currentState = ending then
        DataAvailable  <= '0';
      end if;
    end if;
  end process;

  -- Switches
  with SelectorReg select
  SendPacketInt <= "000" & SendPacket       when "00",
                   "00" & SendPacket & '0'  when "01",
				   '0' & SendPacket & "00"  when "10",
				        SendPacket & "000"  when "11",
				   "0000" when others;

  Data_StreamOut <= Data_StreamInt(to_integer(unsigned(SelectorReg)));
  Data_ValidOut  <= Data_ValidInt(to_integer(unsigned(SelectorReg)));
  Data_Size      <= Data_SizeInt(to_integer(unsigned(SelectorReg)));
  EndOfEventReg  <= EndOfEventInt(to_integer(unsigned(SelectorReg)));
  EndOfEventOut  <= EndOfEventReg;

end architecture;
