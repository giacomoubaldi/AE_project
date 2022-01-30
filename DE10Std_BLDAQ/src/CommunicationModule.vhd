library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;

entity CommunicationModule is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	--- Communication with DAQMOdule
	DAQ_Config : in std_logic;
	Monitor_Registers_Bus : in MONITOR_REGS_T;
	Control_Registers_Bus : out CONTROL_REGS_T := default_CtrlRegisters;
	--- Communication with HPS ---
    iofifo_datain         : in   std_logic_vector(31 downto 0);                    -- datain
    iofifo_writereq       : in   std_logic;                                        -- writereq
    iofifo_instatus       : out    std_logic_vector(31 downto 0) := (others => 'X'); -- instatus
    iofifo_dataout        : out    std_logic_vector(31 downto 0) := (others => 'X'); -- dataout
    iofifo_readack        : in   std_logic;                                        -- readack
    iofifo_outstatus      : out    std_logic_vector(31 downto 0) := (others => 'X'); -- outstatus
    iofifo_regdataout     : out   std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
    iofifo_regreadack     : in   std_logic;                                        -- regreadack
    iofifo_regoutstatus   : out    std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus
	-- Data To/From Event Builder
	FifoEB_info : in std_logic_vector(15 downto 0);
	EVReadAcknowledge : out std_logic;
	-- FIFO RX/TX stats
	FifoRX_info : out std_logic_vector(15 downto 0);
	FifoTX_info : out std_logic_vector(15 downto 0);
	--- Errors ---
	Reset_Errors : in std_logic;
	Invalid_Address : out std_logic := '0';
	INOut_Both_Active : out std_logic := '0'
  );
end entity;

architecture behavior of CommunicationModule is
  
  component Register_File is
  port(
    Clock : in std_logic;
	Reset : in std_logic;
	--- Communication with Local Ethernet Interace ---
	Address : in natural;
	Rdreq : in std_logic;
	Wrreq : in std_logic;
	DataIn : in std_logic_vector (31 downto 0);
	Data_ValidOut : out std_logic := '0';
	DataOut: out std_logic_vector (31 downto 0) := (others => '0');
	--- Selective Reset contains a Reset std_logic line for every Control Register ---
	Selective_Reset : in std_logic_vector(N_CONTROL_REGS-1 downto 0);
	--- Communication with Main_FSM ---
	Monitor_RegistersIn : in MONITOR_REGS_T;
	Control_RegistersOut : out CONTROL_REGS_T := default_CtrlRegisters;
	--- Errors ---
	Reset_Errors : in std_logic;
	Invalid_Address : out std_logic := '0';
	INOut_Both_Active : out std_logic := '0'
  );
  end component;
  
  
  component Local_RXTX is
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
  end component;

  -- Registers signals --
  signal Register_Address : natural;
  signal Register_Wrreq : std_logic:='0';
  signal Register_DataWrite : std_logic_vector(31 downto 0);
  signal Register_Rdreq : std_logic:='0';
  signal Register_DataValid : std_logic;
  signal Register_DataRead : std_logic_vector(31 downto 0);
  signal Reset_DAQErrors : std_logic;
  signal Register_SelectiveReset : std_logic_vector(N_CONTROL_REGS-1 downto 0);

begin
  
  -- --------------------------------------------
  -- Register file
  -- --------------------------------------------
  Registers : Register_File
  port map(
    Clock => Clock,
    Reset => Reset,
    Address => Register_Address,
    Rdreq => Register_Rdreq,
    Wrreq => Register_Wrreq,
    DataIn => Register_DataWrite,
    Data_ValidOut => Register_DataValid,
    DataOut => Register_DataRead,
    Selective_Reset => Register_SelectiveReset,
    Monitor_RegistersIn => Monitor_Registers_Bus,
    Control_RegistersOut => Control_Registers_Bus,
    Reset_Errors => Reset_Errors,
    Invalid_Address => Invalid_Address,
    INOut_Both_Active => INOut_Both_Active
  );

  -- --------------------------------------------
  -- Register Receiver from/to HPS
  -- --------------------------------------------
   LocalCommRXTX : Local_RXTX
   port map (
    Clock => Clock,
    Reset => Reset,
    DAQ_Config => DAQ_config,
	-- In/out from/to HPS
    iofifo_datain     => iofifo_datain, 
    iofifo_writereq   => iofifo_writereq, 
    iofifo_instatus   => iofifo_instatus, 
    iofifo_regdataout => iofifo_regdataout, 
    iofifo_regreadack => iofifo_regreadack,
    iofifo_regoutstatus => iofifo_regoutstatus,
	-- In/out from/to Reg file
    Register_DataValid => Register_DataValid,
    Register_DataRead => Register_DataRead,
    Register_Address => Register_Address,
    Register_Wrreq => Register_Wrreq,
    Register_Rdreq => Register_Rdreq,
    Register_SelectiveReset  => Register_SelectiveReset,
    Register_DataWrite =>  Register_DataWrite,
    -- FIFO stats --
	FifoRX_info => FifoRX_info,
	FifoTX_info => FifoTX_info
  );
  
  -- "Event Fifo" output 
  iofifo_dataout <= x"0000" & FifoEB_info;
  iofifo_outstatus <= x"000" & '0' & FifoEB_info(15 downto 13) & "000" & FifoEB_info(12 downto 0);

  process(Clock, reset) is
    variable oldack : std_logic := '0';
  begin 
    if reset='1' then
	  oldack := '0';
	  EVReadAcknowledge <= '0';
	elsif rising_edge(clock) then
	  if iofifo_readack='1' and oldack='0' then
	    EVReadAcknowledge <= '1';
	  else
	    EVReadAcknowledge <= '0';
      end if;
      oldack := iofifo_readack;
	end if;
  end process;
  
end architecture;