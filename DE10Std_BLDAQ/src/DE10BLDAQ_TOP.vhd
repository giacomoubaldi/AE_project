-- -----------------------------------------------------------------------
-- SOC-FPGA system
-- -----------------------------------------------------------------------          

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.BLDAQ_Package.all;
			
-- ----------------------------------------------
entity DE10BLDAQ_TOP is
------------------------------------------------
  port (
	------------ CLOCK ------------
	CLOCK2_50       	:in    	std_logic;
	CLOCK3_50       	:in    	std_logic;
	CLOCK4_50       	:in    	std_logic;
	CLOCK_50        	:in    	std_logic;

	------------ KEY ------------
	KEY             	:in    	std_logic_vector(3 downto 0);

	------------ SW ------------
	SW              	:in    	std_logic_vector(9 downto 0);

	------------ LED ------------
	LEDR            	:out   	std_logic_vector(9 downto 0);

	------------ Seg7 ------------
	HEX0            	:out   	std_logic_vector(6 downto 0);
	HEX1            	:out   	std_logic_vector(6 downto 0);
	HEX2            	:out   	std_logic_vector(6 downto 0);
	HEX3            	:out   	std_logic_vector(6 downto 0);
	HEX4            	:out   	std_logic_vector(6 downto 0);
	HEX5            	:out   	std_logic_vector(6 downto 0);

	------------ SDRAM ------------
	DRAM_ADDR       	:out   	std_logic_vector(12 downto 0);
	DRAM_BA         	:out   	std_logic_vector(1 downto 0);
	DRAM_CAS_N      	:out   	std_logic;
	DRAM_CKE        	:out   	std_logic;
	DRAM_CLK        	:out   	std_logic;
	DRAM_CS_N       	:out   	std_logic;
	DRAM_DQ         	:inout 	std_logic_vector(15 downto 0);
	DRAM_LDQM       	:out   	std_logic;
	DRAM_RAS_N      	:out   	std_logic;
	DRAM_UDQM       	:out   	std_logic;
	DRAM_WE_N       	:out   	std_logic;

	------------ ADC ------------
	ADC_CONVST      	:out   	std_logic;
	ADC_DIN         	:out   	std_logic;
	ADC_DOUT        	:in    	std_logic;
	ADC_SCLK        	:out   	std_logic;

	------------ HPS ------------
	HPS_CONV_USB_N  	:inout 	std_logic;
	HPS_DDR3_ADDR   	:out   	std_logic_vector(14 downto 0);
	HPS_DDR3_BA     	:out   	std_logic_vector(2 downto 0);
	HPS_DDR3_CAS_N  	:out   	std_logic;
	HPS_DDR3_CKE    	:out   	std_logic;
	HPS_DDR3_CK_N   	:out   	std_logic;
	HPS_DDR3_CK_P   	:out   	std_logic;
	HPS_DDR3_CS_N   	:out   	std_logic;
	HPS_DDR3_DM     	:out   	std_logic_vector(3 downto 0);
	HPS_DDR3_DQ     	:inout 	std_logic_vector(31 downto 0);
	HPS_DDR3_DQS_N  	:inout 	std_logic_vector(3 downto 0);
	HPS_DDR3_DQS_P  	:inout 	std_logic_vector(3 downto 0);
	HPS_DDR3_ODT    	:out   	std_logic;
	HPS_DDR3_RAS_N  	:out   	std_logic;
	HPS_DDR3_RESET_N	:out   	std_logic;
	HPS_DDR3_RZQ    	:in    	std_logic;
	HPS_DDR3_WE_N   	:out   	std_logic;
	HPS_ENET_GTX_CLK	:out   	std_logic;
	HPS_ENET_INT_N  	:inout 	std_logic;
	HPS_ENET_MDC    	:out   	std_logic;
	HPS_ENET_MDIO   	:inout 	std_logic;
	HPS_ENET_RX_CLK 	:in    	std_logic;
	HPS_ENET_RX_DATA	:in    	std_logic_vector(3 downto 0);
	HPS_ENET_RX_DV  	:in    	std_logic;
	HPS_ENET_TX_DATA	:out   	std_logic_vector(3 downto 0);
	HPS_ENET_TX_EN  	:out   	std_logic;
	HPS_FLASH_DATA  	:inout 	std_logic_vector(3 downto 0);
	HPS_FLASH_DCLK  	:out   	std_logic;
	HPS_FLASH_NCSO  	:out   	std_logic;
	HPS_GSENSOR_INT 	:inout 	std_logic;
	HPS_I2C1_SCLK   	:inout 	std_logic;
	HPS_I2C1_SDAT   	:inout 	std_logic;
	HPS_I2C2_SCLK   	:inout 	std_logic;
	HPS_I2C2_SDAT   	:inout 	std_logic;
	HPS_I2C_CONTROL 	:inout 	std_logic;
	HPS_KEY         	:inout 	std_logic;
	HPS_LCM_BK      	:inout 	std_logic;
	HPS_LCM_D_C     	:inout 	std_logic;
	HPS_LCM_RST_N   	:inout 	std_logic;
	HPS_LCM_SPIM_CLK	:out   	std_logic;
	HPS_LCM_SPIM_MISO	:in    	std_logic;
	HPS_LCM_SPIM_MOSI	:out   	std_logic;
	HPS_LCM_SPIM_SS 	:out   	std_logic;
	HPS_LED         	:inout 	std_logic;
	HPS_LTC_GPIO    	:inout 	std_logic;
	HPS_SD_CLK      	:out   	std_logic;
	HPS_SD_CMD      	:inout 	std_logic;
	HPS_SD_DATA     	:inout 	std_logic_vector(3 downto 0);
	HPS_SPIM_CLK    	:out   	std_logic;
	HPS_SPIM_MISO   	:in    	std_logic;
	HPS_SPIM_MOSI   	:out   	std_logic;
	HPS_SPIM_SS     	:out   	std_logic;
	HPS_UART_RX     	:in    	std_logic;
	HPS_UART_TX     	:out   	std_logic;
	HPS_USB_CLKOUT  	:in    	std_logic;
	HPS_USB_DATA    	:inout 	std_logic_vector(7 downto 0);
	HPS_USB_DIR     	:in    	std_logic;
	HPS_USB_NXT     	:in    	std_logic;
	HPS_USB_STP     	:out   	std_logic;
	------------ GPIO, GPIO connect to GPIO Default ------------
	GPIO            	:inout 	std_logic_vector(35 downto 0)
  );
end entity DE10BLDAQ_TOP;
	  
-----------------------------------------------------------------
architecture structural of DE10BLDAQ_TOP is
-----------------------------------------------------------------
  -- system signals
  signal Clock, Reset, nReset : std_logic := '0';
  constant nResetFixed : std_logic := '1';
  signal nColdReset, nWarmReset, nDebugReset : std_logic :='0';

  signal MainTriggerExt, MainTriggerKey, MainTrigger, LongerTrigger : std_logic;
  signal BCOClock, BCOReset, BCOClockExt, BCOResetExt : std_logic;
--  signal Comm_EventReady : std_logic;
  signal Busy_Out : std_logic;   

  -- IO HPS-FPGA
  signal stm_hwevents  : std_logic_vector(27 downto 0);
  signal ledreg  : std_logic_vector(9 downto 0);
  signal LEDST : std_logic_vector(9 downto 0);
  signal KEYE, nKEY, KEYD : std_logic_vector(3 downto 0) := "0000";   
  signal SWD : std_logic_vector(9 downto 0) := "0000000000";   
  signal leds : std_logic_vector(5 downto 0);

  -- for fifo IO
  signal iofifo_datain    :  std_logic_vector(31 downto 0);                       -- datain
  signal iofifo_writereq  :  std_logic;                                           -- writereq
  signal iofifo_instatus  :  std_logic_vector(31 downto 0) := (others => 'X');    -- instatus
  signal iofifo_dataout   :  std_logic_vector(31 downto 0) := (others => 'X');    -- dataout
  signal iofifo_readack   :  std_logic;                                           -- readack
  signal iofifo_outstatus :  std_logic_vector(31 downto 0) := (others => 'X');    -- outstatus
  signal iofifo_regdataout   :  std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
  signal iofifo_regreadack   :  std_logic;                                        -- regreadack
  signal iofifo_regoutstatus :  std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus
  
  -- for RAM module
  signal fpga_side_RAM_ctrl_reg, hps_side_RAM_ctrl_reg : std_logic_vector(31 downto 0) := (others => 'X');
  signal fifo_readack, fifo_empty : std_logic := '0';
  signal DDR3PointerHead : unsigned(31 downto 0);
  signal SendPacket : std_logic;
  constant WaitRequest : std_logic := '0';
  signal DataAvailable : std_logic;
  signal Data_StreamOut : unsigned (31 downto 0);  
  signal Data_ToRAM : std_logic_vector (31 downto 0);  
  signal Data_ValidOut : std_logic;
  signal EndOfEventOut : std_logic;
  signal Data_Size : unsigned (11 downto 0);  
  signal DDR3PointerTail : unsigned(31 downto 0);

  -- ADC
  signal adc_raw_values : adc_values_t := (others => "000000000000");
  
  -- ADC controller IP
  component ADC_controller_IP is
    port (
      CLOCK    : in  std_logic                     := 'X'; -- clk
      RESET    : in  std_logic                     := 'X'; -- reset
      CH0      : out std_logic_vector(11 downto 0);        -- CH0
      CH1      : out std_logic_vector(11 downto 0);        -- CH1
      CH2      : out std_logic_vector(11 downto 0);        -- CH2
      CH3      : out std_logic_vector(11 downto 0);        -- CH3
      CH4      : out std_logic_vector(11 downto 0);        -- CH4
      CH5      : out std_logic_vector(11 downto 0);        -- CH5
      CH6      : out std_logic_vector(11 downto 0);        -- CH6
      CH7      : out std_logic_vector(11 downto 0);        -- CH7
      ADC_SCLK : out std_logic;                            -- SCLK
      ADC_CS_N : out std_logic;                            -- CS_N
      ADC_DOUT : in  std_logic                     := 'X'; -- DOUT
      ADC_DIN  : out std_logic                             -- DIN
    );
  end component ADC_controller_IP;


  component soc_system is
  	port (
  		button_pio_external_connection_export  : in    std_logic_vector(3 downto 0)  := (others => 'X'); -- export
  		clk_clk                                : in    std_logic                     := 'X';             -- clk
  		dipsw_pio_external_connection_export   : in    std_logic_vector(9 downto 0)  := (others => 'X'); -- export
  		hps_0_f2h_cold_reset_req_reset_n       : in    std_logic                     := 'X';             -- reset_n
  		hps_0_f2h_debug_reset_req_reset_n      : in    std_logic                     := 'X';             -- reset_n
  		hps_0_f2h_stm_hw_events_stm_hwevents   : in    std_logic_vector(27 downto 0) := (others => 'X'); -- stm_hwevents
  		hps_0_f2h_warm_reset_req_reset_n       : in    std_logic                     := 'X';             -- reset_n
  		hps_0_h2f_reset_reset_n                : out   std_logic;                                        -- reset_n
  		hps_0_hps_io_hps_io_emac1_inst_TX_CLK  : out   std_logic;                                        -- hps_io_emac1_inst_TX_CLK
  		hps_0_hps_io_hps_io_emac1_inst_TXD0    : out   std_logic;                                        -- hps_io_emac1_inst_TXD0
  		hps_0_hps_io_hps_io_emac1_inst_TXD1    : out   std_logic;                                        -- hps_io_emac1_inst_TXD1
  		hps_0_hps_io_hps_io_emac1_inst_TXD2    : out   std_logic;                                        -- hps_io_emac1_inst_TXD2
  		hps_0_hps_io_hps_io_emac1_inst_TXD3    : out   std_logic;                                        -- hps_io_emac1_inst_TXD3
  		hps_0_hps_io_hps_io_emac1_inst_RXD0    : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD0
  		hps_0_hps_io_hps_io_emac1_inst_MDIO    : inout std_logic                     := 'X';             -- hps_io_emac1_inst_MDIO
  		hps_0_hps_io_hps_io_emac1_inst_MDC     : out   std_logic;                                        -- hps_io_emac1_inst_MDC
  		hps_0_hps_io_hps_io_emac1_inst_RX_CTL  : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RX_CTL
  		hps_0_hps_io_hps_io_emac1_inst_TX_CTL  : out   std_logic;                                        -- hps_io_emac1_inst_TX_CTL
  		hps_0_hps_io_hps_io_emac1_inst_RX_CLK  : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RX_CLK
  		hps_0_hps_io_hps_io_emac1_inst_RXD1    : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD1
  		hps_0_hps_io_hps_io_emac1_inst_RXD2    : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD2
  		hps_0_hps_io_hps_io_emac1_inst_RXD3    : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD3
  		hps_0_hps_io_hps_io_sdio_inst_CMD      : inout std_logic                     := 'X';             -- hps_io_sdio_inst_CMD
  		hps_0_hps_io_hps_io_sdio_inst_D0       : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D0
  		hps_0_hps_io_hps_io_sdio_inst_D1       : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D1
  		hps_0_hps_io_hps_io_sdio_inst_CLK      : out   std_logic;                                        -- hps_io_sdio_inst_CLK
  		hps_0_hps_io_hps_io_sdio_inst_D2       : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D2
  		hps_0_hps_io_hps_io_sdio_inst_D3       : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D3
  		hps_0_hps_io_hps_io_usb1_inst_D0       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D0
  		hps_0_hps_io_hps_io_usb1_inst_D1       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D1
  		hps_0_hps_io_hps_io_usb1_inst_D2       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D2
  		hps_0_hps_io_hps_io_usb1_inst_D3       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D3
  		hps_0_hps_io_hps_io_usb1_inst_D4       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D4
  		hps_0_hps_io_hps_io_usb1_inst_D5       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D5
  		hps_0_hps_io_hps_io_usb1_inst_D6       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D6
  		hps_0_hps_io_hps_io_usb1_inst_D7       : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D7
  		hps_0_hps_io_hps_io_usb1_inst_CLK      : in    std_logic                     := 'X';             -- hps_io_usb1_inst_CLK
  		hps_0_hps_io_hps_io_usb1_inst_STP      : out   std_logic;                                        -- hps_io_usb1_inst_STP
  		hps_0_hps_io_hps_io_usb1_inst_DIR      : in    std_logic                     := 'X';             -- hps_io_usb1_inst_DIR
  		hps_0_hps_io_hps_io_usb1_inst_NXT      : in    std_logic                     := 'X';             -- hps_io_usb1_inst_NXT
  		hps_0_hps_io_hps_io_spim1_inst_CLK     : out   std_logic;                                        -- hps_io_spim1_inst_CLK
  		hps_0_hps_io_hps_io_spim1_inst_MOSI    : out   std_logic;                                        -- hps_io_spim1_inst_MOSI
  		hps_0_hps_io_hps_io_spim1_inst_MISO    : in    std_logic                     := 'X';             -- hps_io_spim1_inst_MISO
  		hps_0_hps_io_hps_io_spim1_inst_SS0     : out   std_logic;                                        -- hps_io_spim1_inst_SS0
  		hps_0_hps_io_hps_io_uart0_inst_RX      : in    std_logic                     := 'X';             -- hps_io_uart0_inst_RX
  		hps_0_hps_io_hps_io_uart0_inst_TX      : out   std_logic;                                        -- hps_io_uart0_inst_TX
  		hps_0_hps_io_hps_io_i2c0_inst_SDA      : inout std_logic                     := 'X';             -- hps_io_i2c0_inst_SDA
  		hps_0_hps_io_hps_io_i2c0_inst_SCL      : inout std_logic                     := 'X';             -- hps_io_i2c0_inst_SCL
  		hps_0_hps_io_hps_io_i2c1_inst_SDA      : inout std_logic                     := 'X';             -- hps_io_i2c1_inst_SDA
  		hps_0_hps_io_hps_io_i2c1_inst_SCL      : inout std_logic                     := 'X';             -- hps_io_i2c1_inst_SCL
  		hps_0_hps_io_hps_io_gpio_inst_GPIO09   : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO09
  		hps_0_hps_io_hps_io_gpio_inst_GPIO35   : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO35
  		hps_0_hps_io_hps_io_gpio_inst_GPIO40   : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO40
  		hps_0_hps_io_hps_io_gpio_inst_GPIO53   : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO53
  		hps_0_hps_io_hps_io_gpio_inst_GPIO54   : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO54
  		hps_0_hps_io_hps_io_gpio_inst_GPIO61   : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO61
  		hps_fpga_ram_sync_gp_in                : in    std_logic_vector(31 downto 0) := (others => 'X'); -- gp_in
  		hps_fpga_ram_sync_gp_out               : out   std_logic_vector(31 downto 0);                    -- gp_out
  		iofifo_datain                          : out   std_logic_vector(31 downto 0);                    -- datain
  		iofifo_writereq                        : out   std_logic;                                        -- writereq
  		iofifo_instatus                        : in    std_logic_vector(31 downto 0) := (others => 'X'); -- instatus
  		iofifo_dataout                         : in    std_logic_vector(31 downto 0) := (others => 'X'); -- dataout
  		iofifo_readack                         : out   std_logic;                                        -- readack
  		iofifo_outstatus                       : in    std_logic_vector(31 downto 0) := (others => 'X'); -- outstatus
  		iofifo_regdataout                      : in    std_logic_vector(31 downto 0) := (others => 'X'); -- regdataout
  		iofifo_regreadack                      : out   std_logic;                                        -- regreadack
  		iofifo_regoutstatus                    : in    std_logic_vector(31 downto 0) := (others => 'X'); -- regoutstatus
  		led_pio_external_connection_export     : out   std_logic_vector(9 downto 0);                     -- export
  		memory_mem_a                           : out   std_logic_vector(14 downto 0);                    -- mem_a
  		memory_mem_ba                          : out   std_logic_vector(2 downto 0);                     -- mem_ba
  		memory_mem_ck                          : out   std_logic;                                        -- mem_ck
  		memory_mem_ck_n                        : out   std_logic;                                        -- mem_ck_n
  		memory_mem_cke                         : out   std_logic;                                        -- mem_cke
  		memory_mem_cs_n                        : out   std_logic;                                        -- mem_cs_n
  		memory_mem_ras_n                       : out   std_logic;                                        -- mem_ras_n
  		memory_mem_cas_n                       : out   std_logic;                                        -- mem_cas_n
  		memory_mem_we_n                        : out   std_logic;                                        -- mem_we_n
  		memory_mem_reset_n                     : out   std_logic;                                        -- mem_reset_n
  		memory_mem_dq                          : inout std_logic_vector(31 downto 0) := (others => 'X'); -- mem_dq
  		memory_mem_dqs                         : inout std_logic_vector(3 downto 0)  := (others => 'X'); -- mem_dqs
  		memory_mem_dqs_n                       : inout std_logic_vector(3 downto 0)  := (others => 'X'); -- mem_dqs_n
  		memory_mem_odt                         : out   std_logic;                                        -- mem_odt
  		memory_mem_dm                          : out   std_logic_vector(3 downto 0);                     -- mem_dm
  		memory_oct_rzqin                       : in    std_logic                     := 'X';             -- oct_rzqin
  		reset_reset_n                          : in    std_logic                     := 'X';             -- reset_n
  		to_ram_ctrl_fpga_side_reg              : out   std_logic_vector(31 downto 0);                    -- fpga_side_reg
  		to_ram_ctrl_hps_side_reg               : in    std_logic_vector(31 downto 0) := (others => 'X'); -- hps_side_reg
  		to_ram_ctrl_enable                     : in    std_logic                     := 'X';             -- enable
  		to_ram_fifo_data_ack                   : out   std_logic;                                        -- data_ack
  		to_ram_fifo_data_event                 : in    std_logic_vector(31 downto 0) := (others => 'X'); -- data_event
  		to_ram_fifo_data_empty                 : in    std_logic                     := 'X';             -- data_empty
  		version_pio_external_connection_export : in    std_logic_vector(31 downto 0) := (others => 'X')  -- export
  	);
  end component soc_system;
  
  component BLDAQ_Module is
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
	--
    --External clock 
    TSClockIn : in std_logic;      
    TSResetIn : in std_logic;    
    --Trigger signal from GPIO
    TriggerIn : in std_logic;     
    Busy_Out : out std_logic;    	
	-- ADC --
	adc_raw_values : in adc_values_t;
	-- External data IN
	ExtData_Streams : in NSensStreamData_t;  
	ExtData_Valid : in std_logic_vector(N_Sensors-1 downto 0);
    ExtEndOfEvent : in std_logic_vector(N_Sensors-1 downto 0); 	
	-- I/O to RAM Manager 
    DDR3PointerHead : in unsigned(31 downto 0);
	SendPacket : in std_logic;
	WaitRequest : in std_logic;
    DataAvailable : out std_logic;
	Data_StreamOut : out unsigned (31 downto 0);  
	Data_ValidOut : out std_logic;
    EndOfEventOut : out std_logic;
	Data_Size : out unsigned (11 downto 0);  
    DDR3PointerTail : out unsigned(31 downto 0);
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
	--
	leds : out std_logic_vector(5 downto 0);
	-- For simulation only!!
	Comm_EventReady : out std_logic
  );
  end component;

  component DataTransfer is
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
  end component;

  component DetectorSimulator is
  port (
    --Inputs
    Clock : in std_logic;
	Reset : in std_logic;
    --Outputs
	Data_Streams : out NSensStreamData_t;  
	Data_Valid : out std_logic_vector(N_Sensors-1 downto 0);
    EndOfEvent : out std_logic_vector(N_Sensors-1 downto 0)	 
  );
  end component;
  signal ExtData_Streams : NSensStreamData_t;  
  signal ExtData_Valid : std_logic_vector(N_Sensors-1 downto 0);
  signal ExtEndOfEvent : std_logic_vector(N_Sensors-1 downto 0); 

  component TimeStampGenerator is  
  port (
    Clock : in std_logic;
    Reset : in std_logic;
	DAQRunning : in std_logic;
    TSReset    : out std_logic;
    TSClock    : out std_logic
  );
  end component;
  signal GenTSReset, GenTSClock : std_logic;

begin
  
  -- Main clock and resets			
  Clock <= CLOCK_50; -- From external part		
  Reset <= not(nReset);  -- From HPS
  
  --      debounce KEY and SWITCH signals
  --  
  -- debounce KEY      NOT NECESSARY, THERE ARE 74AUC17 ICs
  nKEY <= not(KEY);
  dbc0: entity work.Debounce  
    port map( Clk=>Clock, Reset=>Reset,   button=>nKEY(0),   pulse=>KEYD(0) );
  dbc1: entity work.Debounce  
    port map( Clk=>Clock, Reset=>Reset,   button=>nKEY(1),   pulse=>KEYD(1) );
  
  -- debounce switches
  dbc2: entity work.Debounce  
    port map(Clk=> Clock, Reset=>Reset,   button=>SW(0),     pulse=>SWD(0) );
  dbc3: entity work.Debounce  
    port map(Clk=> Clock, Reset=>Reset,   button=>SW(1),     pulse=>SWD(1) );
  dbc4: entity work.Debounce  
    port map(Clk=> Clock, Reset=>Reset,   button=>SW(2),     pulse=>SWD(2) );
  dbc5: entity work.Debounce  
    port map(Clk=> Clock, Reset=>Reset,   button=>SW(3),     pulse=>SWD(3) );

  process(Clock)
  begin
    if rising_edge(clock) then
      MainTriggerExt <= '0';--GPIO(Trigger_Pin);
      BCOResetExt       <= GPIO(BCOReset_Pin);
      BCOClockExt       <= GPIO(BCOClock_Pin);
	end if;
  end process;
  
  TSGen: TimeStampGenerator 
  port map (
    Clock => Clock, Reset => Reset,
	DAQRunning => ledst(5),
    TSReset => GenTSReset,
    TSClock => GenTSClock
  );

  BCOReset <=  BCOResetExt when SWD(0)='0' 
               else GenTSReset;
  BCOClock <=  BCOClockExt when SWD(0)='0' 
               else GenTSClock;
  
  -- Trigger from KEY
  process(Clock)
    variable oldKey0 : std_logic := '0';
  begin
    if rising_edge(Clock)  then
	  if KEYD(0)='1' and oldkey0='0' then
	    MainTriggerKey <= '1';
	  else
	    MainTriggerKey <= '0';
	  end if;
	  oldKey0 := KEYD(0);
	end if;
  end process;
  
   -- Maintrigger can be sent via a key (debug) or via an external pin
  MainTrigger <= MainTriggerKey or MainTriggerExt;

  u0 : soc_system
  port map (
    clk_clk                               => Clock,                                              --                            clk.clk
    reset_reset_n                         => nResetFixed,                                        --                          reset.reset_n
    hps_0_f2h_cold_reset_req_reset_n      => nColdReset,                                         --       hps_0_f2h_cold_reset_req.reset_n
    hps_0_f2h_debug_reset_req_reset_n     => nDebugReset,                                        --      hps_0_f2h_debug_reset_req.reset_n
    hps_0_f2h_stm_hw_events_stm_hwevents  => stm_hwevents,                                       --        hps_0_f2h_stm_hw_events.stm_hwevents
    hps_0_f2h_warm_reset_req_reset_n      => nWarmReset,                                         --       hps_0_f2h_warm_reset_req.reset_n
    hps_0_h2f_reset_reset_n               => nReset,                                             --                hps_0_h2f_reset.reset_n
	 -- HPS Ethernet
    hps_0_hps_io_hps_io_emac1_inst_TX_CLK => HPS_ENET_GTX_CLK,                                   --                   hps_0_hps_io.hps_io_emac1_inst_TX_CLK
    hps_0_hps_io_hps_io_emac1_inst_TXD0   => HPS_ENET_TX_DATA(0),                                --                               .hps_io_emac1_inst_TXD0
    hps_0_hps_io_hps_io_emac1_inst_TXD1   => HPS_ENET_TX_DATA(1),                                --                               .hps_io_emac1_inst_TXD1
    hps_0_hps_io_hps_io_emac1_inst_TXD2   => HPS_ENET_TX_DATA(2),                                --                               .hps_io_emac1_inst_TXD2
    hps_0_hps_io_hps_io_emac1_inst_TXD3   => HPS_ENET_TX_DATA(3),                                --                               .hps_io_emac1_inst_TXD3
    hps_0_hps_io_hps_io_emac1_inst_RXD0   => HPS_ENET_RX_DATA(0),                                --                               .hps_io_emac1_inst_RXD0
    hps_0_hps_io_hps_io_emac1_inst_MDIO   => HPS_ENET_MDIO,                                      --                               .hps_io_emac1_inst_MDIO
    hps_0_hps_io_hps_io_emac1_inst_MDC    => HPS_ENET_MDC,                                       --                               .hps_io_emac1_inst_MDC
    hps_0_hps_io_hps_io_emac1_inst_RX_CTL => HPS_ENET_RX_DV,                                     --                               .hps_io_emac1_inst_RX_CTL
    hps_0_hps_io_hps_io_emac1_inst_TX_CTL => HPS_ENET_TX_EN,                                     --                               .hps_io_emac1_inst_TX_CTL
    hps_0_hps_io_hps_io_emac1_inst_RX_CLK => HPS_ENET_RX_CLK,                                    --                               .hps_io_emac1_inst_RX_CLK
    hps_0_hps_io_hps_io_emac1_inst_RXD1   => HPS_ENET_RX_DATA(1),                                --                               .hps_io_emac1_inst_RXD1
    hps_0_hps_io_hps_io_emac1_inst_RXD2   => HPS_ENET_RX_DATA(2),                                --                               .hps_io_emac1_inst_RXD2
    hps_0_hps_io_hps_io_emac1_inst_RXD3   => HPS_ENET_RX_DATA(3),                                --                               .hps_io_emac1_inst_RXD3
	 -- HPS SD Card
    hps_0_hps_io_hps_io_sdio_inst_CMD     => HPS_SD_CMD,                                         --                               .hps_io_sdio_inst_CMD
    hps_0_hps_io_hps_io_sdio_inst_D0      => HPS_SD_DATA(0),                                     --                               .hps_io_sdio_inst_D0
    hps_0_hps_io_hps_io_sdio_inst_D1      => HPS_SD_DATA(1),                                     --                               .hps_io_sdio_inst_D1
    hps_0_hps_io_hps_io_sdio_inst_CLK     => HPS_SD_CLK,                                         --                               .hps_io_sdio_inst_CLK
    hps_0_hps_io_hps_io_sdio_inst_D2      => HPS_SD_DATA(2),                                     --                               .hps_io_sdio_inst_D2
    hps_0_hps_io_hps_io_sdio_inst_D3      => HPS_SD_DATA(3),                                     --                               .hps_io_sdio_inst_D3
	 -- HPS USB
    hps_0_hps_io_hps_io_usb1_inst_D0      => HPS_USB_DATA(0),                                    --                               .hps_io_usb1_inst_D0
    hps_0_hps_io_hps_io_usb1_inst_D1      => HPS_USB_DATA(1),                                    --                               .hps_io_usb1_inst_D1
    hps_0_hps_io_hps_io_usb1_inst_D2      => HPS_USB_DATA(2),                                    --                               .hps_io_usb1_inst_D2
    hps_0_hps_io_hps_io_usb1_inst_D3      => HPS_USB_DATA(3),                                    --                               .hps_io_usb1_inst_D3
    hps_0_hps_io_hps_io_usb1_inst_D4      => HPS_USB_DATA(4),                                    --                               .hps_io_usb1_inst_D4
    hps_0_hps_io_hps_io_usb1_inst_D5      => HPS_USB_DATA(5),                                    --                               .hps_io_usb1_inst_D5
    hps_0_hps_io_hps_io_usb1_inst_D6      => HPS_USB_DATA(6),                                    --                               .hps_io_usb1_inst_D6
    hps_0_hps_io_hps_io_usb1_inst_D7      => HPS_USB_DATA(7),                                    --                               .hps_io_usb1_inst_D7
    hps_0_hps_io_hps_io_usb1_inst_CLK     => HPS_USB_CLKOUT,                                     --                               .hps_io_usb1_inst_CLK
    hps_0_hps_io_hps_io_usb1_inst_STP     => HPS_USB_STP,                                        --                               .hps_io_usb1_inst_STP
    hps_0_hps_io_hps_io_usb1_inst_DIR     => HPS_USB_DIR,                                        --                               .hps_io_usb1_inst_DIR
    hps_0_hps_io_hps_io_usb1_inst_NXT     => HPS_USB_NXT,                                        --                               .hps_io_usb1_inst_NXT
	 -- HPS SPI
    hps_0_hps_io_hps_io_spim1_inst_CLK    => HPS_SPIM_CLK,                                       --                               .hps_io_spim1_inst_CLK
    hps_0_hps_io_hps_io_spim1_inst_MOSI   => HPS_SPIM_MOSI,                                      --                               .hps_io_spim1_inst_MOSI
    hps_0_hps_io_hps_io_spim1_inst_MISO   => HPS_SPIM_MISO,                                      --                               .hps_io_spim1_inst_MISO
    hps_0_hps_io_hps_io_spim1_inst_SS0    => HPS_SPIM_SS,                                        --                               .hps_io_spim1_inst_SS0
	 -- HPS UART
    hps_0_hps_io_hps_io_uart0_inst_RX     => HPS_UART_RX,                                        --                               .hps_io_uart0_inst_RX
    hps_0_hps_io_hps_io_uart0_inst_TX     => HPS_UART_TX,                                        --                               .hps_io_uart0_inst_TX
	 -- HPS I2C0
    hps_0_hps_io_hps_io_i2c0_inst_SDA     => HPS_I2C1_SDAT,                                      --                               .hps_io_i2c0_inst_SDA
    hps_0_hps_io_hps_io_i2c0_inst_SCL     => HPS_I2C1_SCLK,                                      --                               .hps_io_i2c0_inst_SCL
	 -- HPS I2C1
    hps_0_hps_io_hps_io_i2c1_inst_SDA     => HPS_I2C2_SDAT,                                      --                               .hps_io_i2c1_inst_SDA
    hps_0_hps_io_hps_io_i2c1_inst_SCL     => HPS_I2C2_SCLK,                                      --                               .hps_io_i2c1_inst_SCL
	 -- GPIO 
    hps_0_hps_io_hps_io_gpio_inst_GPIO09  => HPS_CONV_USB_N,                                     --                               .hps_io_gpio_inst_GPIO09
    hps_0_hps_io_hps_io_gpio_inst_GPIO35  => HPS_ENET_INT_N,                                     --                               .hps_io_gpio_inst_GPIO35
    hps_0_hps_io_hps_io_gpio_inst_GPIO40  => HPS_LTC_GPIO,                                       --                               .hps_io_gpio_inst_GPIO40
    hps_0_hps_io_hps_io_gpio_inst_GPIO53  => HPS_LED,                                            --                               .hps_io_gpio_inst_GPIO53
    hps_0_hps_io_hps_io_gpio_inst_GPIO54  => HPS_KEY,                                            --                               .hps_io_gpio_inst_GPIO54
    hps_0_hps_io_hps_io_gpio_inst_GPIO61  => HPS_GSENSOR_INT,                                    --                               .hps_io_gpio_inst_GPIO61
	
	 -- HPS DDR3
    memory_mem_a                          => HPS_DDR3_ADDR,                                      --                         memory.mem_a
    memory_mem_ba                         => HPS_DDR3_BA,                                        --                               .mem_ba
    memory_mem_ck                         => HPS_DDR3_CK_P,                                      --                               .mem_ck
    memory_mem_ck_n                       => HPS_DDR3_CK_N,            				             --                               .mem_ck_n
    memory_mem_cke                        => HPS_DDR3_CKE,                                       --                               .mem_cke
    memory_mem_cs_n                       => HPS_DDR3_CS_N,                                      --                               .mem_cs_n
    memory_mem_ras_n                      => HPS_DDR3_RAS_N,                                     --                               .mem_ras_n
    memory_mem_cas_n                      => HPS_DDR3_CAS_N ,                                    --                               .mem_cas_n
    memory_mem_we_n                       => HPS_DDR3_WE_N,                                      --                               .mem_we_n
    memory_mem_reset_n                    => HPS_DDR3_RESET_N,                                   --                               .mem_reset_n
    memory_mem_dq                         => HPS_DDR3_DQ,                                        --                               .mem_dq
    memory_mem_dqs                        => HPS_DDR3_DQS_P,                                     --                               .mem_dqs
    memory_mem_dqs_n                      => HPS_DDR3_DQS_N,                                     --                               .mem_dqs_n
    memory_mem_odt                        => HPS_DDR3_ODT,                                       --                               .mem_odt
    memory_mem_dm                         => HPS_DDR3_DM,                                        --                               .mem_dm
    memory_oct_rzqin                      => HPS_DDR3_RZQ,                                       --                               .oct_rzqin
	
	 -- HPS Parallel I/O and user modules
    button_pio_external_connection_export => KEYE,      -- button_pio_external_connection.export
    dipsw_pio_external_connection_export  => SWD,       --  dipsw_pio_external_connection.export
    led_pio_external_connection_export    => ledreg,    --    led_pio_external_connection.export
	version_pio_external_connection_export => Firmware_Version,  -- fixed value!!
	 -- HPS Fifo control
    iofifo_datain                         => iofifo_datain, -- datain
    iofifo_writereq                       => iofifo_writereq,
    iofifo_instatus                       => iofifo_instatus,
    iofifo_dataout                        => iofifo_dataout,
    iofifo_readack                        => iofifo_readack,
    iofifo_outstatus                      => iofifo_outstatus,
    iofifo_regdataout                     => iofifo_regdataout,
    iofifo_regreadack                     => iofifo_regreadack,
    iofifo_regoutstatus                   => iofifo_regoutstatus,
	 -- event_FIFO to DDR3 manager------------------------------------------------------------------
    to_ram_ctrl_fpga_side_reg  => fpga_side_RAM_ctrl_reg,      --     to_ram_ctrl.fpga_side_reg -- this is the buffer HEAD (where to write)
    to_ram_ctrl_hps_side_reg   => hps_side_RAM_ctrl_reg,       --    .hps_side_reg -- This is the buffer TAIL (oldest values)
    to_ram_ctrl_enable         => '1',                         --      .enable
    to_ram_fifo_data_ack       => fifo_readack,                --       to_ram_fifo.data_ack
    to_ram_fifo_data_event     => Data_ToRAM,                  --          .data_event
    to_ram_fifo_data_empty     => fifo_empty,                  --     .data_empty
    hps_fpga_ram_sync_gp_in    => hps_side_RAM_ctrl_reg,       --       hps_fpga_ram_sync.gp_in
    hps_fpga_ram_sync_gp_out   => open ---fpga_side_RAM_ctrl_reg        --     .gp_out
  );
  DDR3PointerHead <= unsigned(fpga_side_RAM_ctrl_reg);
  hps_side_RAM_ctrl_reg <= std_logic_vector(DDR3PointerTail);
    
  -- --------------------------------------------
  --   BusyLessDAQ_Module 
  -- --------------------------------------------
  DUT: BLDAQ_Module
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
    TSClockIn => BCOClock,     
    TSResetIn => BCOReset, 
    --Trigger signal from GPIO
    TriggerIn => MainTrigger,
	Busy_Out => Busy_Out,
	-- ADC --
	adc_raw_values => adc_raw_values,
		-- External data IN
	ExtData_Streams => ExtData_Streams,
	ExtData_Valid => ExtData_Valid,
    ExtEndOfEvent => ExtEndOfEvent,
	-- I/O to RAM Manager 
    DDR3PointerHead => DDR3PointerHead,
	SendPacket => SendPacket,
	WaitRequest => WaitRequest,
    DataAvailable => DataAvailable,
	Data_StreamOut => Data_StreamOut, 
	Data_ValidOut => Data_ValidOut,
    EndOfEventOut => EndOfEventOut,
	Data_Size => Data_Size,
    DDR3PointerTail => DDR3PointerTail,
	--- Communication with HPS ---
    iofifo_datain                         => iofifo_datain, 
    iofifo_writereq                       => iofifo_writereq,
    iofifo_instatus                       => iofifo_instatus,
    iofifo_dataout                        => iofifo_dataout,
    iofifo_readack                        => iofifo_readack,
    iofifo_outstatus                      => iofifo_outstatus,
    iofifo_regdataout                     => iofifo_regdataout,
    iofifo_regreadack                     => iofifo_regreadack,
    iofifo_regoutstatus                   => iofifo_regoutstatus,
	leds => leds,
	Comm_EventReady => open -- Comm_EventReady
  );
  -- -----------------------------------------------------------
  --   Transfer of data from Event builder to DDR3
  -- -----------------------------------------------------------

  DT: DataTransfer 
  port map (
    Clock => Clock,
	Reset => Reset,
    -- I/O to Event Builder 
	SendPacket => SendPacket,
    DataAvailable => DataAvailable,
	Data_Stream => Data_StreamOut, 
	Data_Valid => Data_ValidOut,
    EndOfEvent => EndOfEventOut,
    -- I/O to DDR3
    data_ack    => fifo_readack,
    data_event  => Data_ToRAM,
    data_empty  => fifo_empty,
	--
	Fifo_info => open
  );

  -- -----------------------------------------------------------
  --   Detector simulator... to be replaced with real detectors!
  -- -----------------------------------------------------------
  DS: DetectorSimulator
  port map (
    --Inputs
    Clock => Clock,
	Reset => Reset,
    --Outputs
	Data_Streams => ExtData_Streams,
	Data_Valid => ExtData_Valid,
    EndOfEvent => ExtEndOfEvent	 
  );
  
  ---------------------------------- End Communication
    --
  --    Other outputs to HPS
  --  
  -- This contains keys and most important FIFO bits
  KEYE <= KEYD;-- key extended:
  -- stm_hwevents:  how these information is used??
  stm_hwevents <=  "00000000000000" & swd & KEYD;  
			
  --
  --    : provide some output signals to LED and GPIO
  --  

  -- make some signals longer so that they become suitable for LED pulses
  lp1: entity work.LongerPulse
       port map( Clk=>Clock, Reset=>Reset,  pulse=>MainTrigger,   longPulse=>longerTrigger );
 	
  -- Out to LEDs
  ledst <=  Busy_Out & fifo_empty & DataAvailable & longerTrigger & leds;
  LEDR <= ledst;-- or ledreg;
	 
  -- output of some key signals to GPIO
  GPIO(BusyOut_Pin) <= Busy_Out;
  GPIO(BusyOut_Pin+2) <= MainTrigger;
  GPIO(BusyOut_Pin+4) <= Clock;
  -- beam 
--  GPIO(BeamOut_Pin) <= Beam;
--  GPIO(BeamStartOut_Pin) <= BeamStart;
--  GPIO(BeamEndOut_Pin) <= BeamEnd;
  --
--  GPIO(34 downto 33) <= Errors & MainTrigger;
  
  -- Set to '0' all unused  pins
  GPIO(0) <= '0';  GPIO(2) <= '0';  GPIO(3) <= '0';  GPIO(4) <= '0'; 
  GPIO(6) <= '0';  GPIO(7) <= '0';  GPIO(8) <= '0';  GPIO(9) <= '0';
  
  GPIO(10) <= '0'; GPIO(12) <= '0'; GPIO(14) <= '0'; 
  GPIO(16) <= '0'; GPIO(18) <= '0';
  GPIO(20) <= '0'; GPIO(22) <= '0'; GPIO(23) <= '0'; GPIO(24) <= '0';
  
  GPIO(26) <= '0'; GPIO(27) <= '0'; GPIO(28) <= '0'; GPIO(29) <= '0';
  GPIO(30) <= '0'; GPIO(32) <= '0'; GPIO(34) <= '0';

  
  -- ADC Controller
  adc_0 : component ADC_controller_IP
    port map (
      CLOCK    => Clock,    --                clk.clk
      RESET    => Reset,    --              reset.reset
		-- Analog values
      CH0      => adc_raw_values(0),      --           readings.CH0
	  CH1      => adc_raw_values(1),      --                   .CH1
	  CH2      => adc_raw_values(2),      --                   .CH2
	  CH3      => adc_raw_values(3),      --                   .CH3
	  CH4      => adc_raw_values(4),      --                   .CH4
	  CH5      => adc_raw_values(5),      --                   .CH5
	  CH6      => adc_raw_values(6),      --                   .CH6
	  CH7      => adc_raw_values(7),      --                   .CH7
	  -- FPGA pins
	  ADC_SCLK => ADC_SCLK,   -- external_interface.SCLK
	  ADC_CS_N => ADC_CONVST, --                   .CS_N
	  ADC_DOUT => ADC_DOUT,   --                   .DOUT
	  ADC_DIN  => ADC_DIN     --                   .DIN
  );

  -- 
  --  Cold, Warm and Debug RESETs
  --
  process(Clock)
    variable oldRes : std_logic;
	variable counter : natural := 0;
  begin
    if rising_edge(Clock) then
	 -- Generate different resets based on counter (see below)
      if counter>1 and counter<4 then
	    nWarmReset <= '0';-- short
	  else
	    nWarmReset <= '1';
	  end if;
      if counter>1 and counter<8 then
	    nColdReset <= '0';-- longer
	  else
	    nColdReset <= '1';
	  end if;
      if counter>1 and counter<32 then
	    nDebugReset <= '0';-- longest
	  else
	    nDebugReset <= '1';
	  end if;
	  -- When rising edge on reset, start counting to 100, then reset and wait
      if Reset='1' and oldRes='0' then
	    counter :=1;
	  elsif counter > 0 and counter<100 then
        counter := counter+1;
	  else 
	   counter := 0;
	  end if;
	  oldRes := Reset;
	end if;
  end process;
  		
end architecture structural;