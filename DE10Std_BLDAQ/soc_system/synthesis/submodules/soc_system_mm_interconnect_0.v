// soc_system_mm_interconnect_0.v

// This file was auto-generated from altera_mm_interconnect_hw.tcl.  If you edit it your changes
// will probably be lost.
// 
// Generated using ACDS version 18.1 625

`timescale 1 ps / 1 ps
module soc_system_mm_interconnect_0 (
		input  wire        clk_0_clk_clk,                                                      //                                                    clk_0_clk.clk
		input  wire        DDR3_manager_0_reset_reset_bridge_in_reset_reset,                   //                   DDR3_manager_0_reset_reset_bridge_in_reset.reset
		input  wire        hps_0_f2h_sdram0_data_translator_reset_reset_bridge_in_reset_reset, // hps_0_f2h_sdram0_data_translator_reset_reset_bridge_in_reset.reset
		input  wire [31:0] DDR3_manager_0_avalon_master_address,                               //                                 DDR3_manager_0_avalon_master.address
		output wire        DDR3_manager_0_avalon_master_waitrequest,                           //                                                             .waitrequest
		input  wire [7:0]  DDR3_manager_0_avalon_master_burstcount,                            //                                                             .burstcount
		input  wire [3:0]  DDR3_manager_0_avalon_master_byteenable,                            //                                                             .byteenable
		input  wire        DDR3_manager_0_avalon_master_read,                                  //                                                             .read
		output wire [31:0] DDR3_manager_0_avalon_master_readdata,                              //                                                             .readdata
		output wire        DDR3_manager_0_avalon_master_readdatavalid,                         //                                                             .readdatavalid
		input  wire        DDR3_manager_0_avalon_master_write,                                 //                                                             .write
		input  wire [31:0] DDR3_manager_0_avalon_master_writedata,                             //                                                             .writedata
		output wire [29:0] hps_0_f2h_sdram0_data_address,                                      //                                        hps_0_f2h_sdram0_data.address
		output wire        hps_0_f2h_sdram0_data_write,                                        //                                                             .write
		output wire        hps_0_f2h_sdram0_data_read,                                         //                                                             .read
		input  wire [31:0] hps_0_f2h_sdram0_data_readdata,                                     //                                                             .readdata
		output wire [31:0] hps_0_f2h_sdram0_data_writedata,                                    //                                                             .writedata
		output wire [7:0]  hps_0_f2h_sdram0_data_burstcount,                                   //                                                             .burstcount
		output wire [3:0]  hps_0_f2h_sdram0_data_byteenable,                                   //                                                             .byteenable
		input  wire        hps_0_f2h_sdram0_data_readdatavalid,                                //                                                             .readdatavalid
		input  wire        hps_0_f2h_sdram0_data_waitrequest                                   //                                                             .waitrequest
	);

	wire         ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_waitrequest;   // hps_0_f2h_sdram0_data_translator:uav_waitrequest -> DDR3_manager_0_avalon_master_translator:uav_waitrequest
	wire  [31:0] ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_readdata;      // hps_0_f2h_sdram0_data_translator:uav_readdata -> DDR3_manager_0_avalon_master_translator:uav_readdata
	wire         ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_debugaccess;   // DDR3_manager_0_avalon_master_translator:uav_debugaccess -> hps_0_f2h_sdram0_data_translator:uav_debugaccess
	wire  [31:0] ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_address;       // DDR3_manager_0_avalon_master_translator:uav_address -> hps_0_f2h_sdram0_data_translator:uav_address
	wire         ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_read;          // DDR3_manager_0_avalon_master_translator:uav_read -> hps_0_f2h_sdram0_data_translator:uav_read
	wire   [3:0] ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_byteenable;    // DDR3_manager_0_avalon_master_translator:uav_byteenable -> hps_0_f2h_sdram0_data_translator:uav_byteenable
	wire         ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_readdatavalid; // hps_0_f2h_sdram0_data_translator:uav_readdatavalid -> DDR3_manager_0_avalon_master_translator:uav_readdatavalid
	wire         ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_lock;          // DDR3_manager_0_avalon_master_translator:uav_lock -> hps_0_f2h_sdram0_data_translator:uav_lock
	wire         ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_write;         // DDR3_manager_0_avalon_master_translator:uav_write -> hps_0_f2h_sdram0_data_translator:uav_write
	wire  [31:0] ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_writedata;     // DDR3_manager_0_avalon_master_translator:uav_writedata -> hps_0_f2h_sdram0_data_translator:uav_writedata
	wire   [9:0] ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_burstcount;    // DDR3_manager_0_avalon_master_translator:uav_burstcount -> hps_0_f2h_sdram0_data_translator:uav_burstcount

	altera_merlin_master_translator #(
		.AV_ADDRESS_W                (32),
		.AV_DATA_W                   (32),
		.AV_BURSTCOUNT_W             (8),
		.AV_BYTEENABLE_W             (4),
		.UAV_ADDRESS_W               (32),
		.UAV_BURSTCOUNT_W            (10),
		.USE_READ                    (1),
		.USE_WRITE                   (1),
		.USE_BEGINBURSTTRANSFER      (0),
		.USE_BEGINTRANSFER           (0),
		.USE_CHIPSELECT              (0),
		.USE_BURSTCOUNT              (1),
		.USE_READDATAVALID           (1),
		.USE_WAITREQUEST             (1),
		.USE_READRESPONSE            (0),
		.USE_WRITERESPONSE           (0),
		.AV_SYMBOLS_PER_WORD         (4),
		.AV_ADDRESS_SYMBOLS          (1),
		.AV_BURSTCOUNT_SYMBOLS       (0),
		.AV_CONSTANT_BURST_BEHAVIOR  (1),
		.UAV_CONSTANT_BURST_BEHAVIOR (1),
		.AV_LINEWRAPBURSTS           (0),
		.AV_REGISTERINCOMINGSIGNALS  (0)
	) ddr3_manager_0_avalon_master_translator (
		.clk                    (clk_0_clk_clk),                                                                   //                       clk.clk
		.reset                  (DDR3_manager_0_reset_reset_bridge_in_reset_reset),                                //                     reset.reset
		.uav_address            (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_address),       // avalon_universal_master_0.address
		.uav_burstcount         (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_burstcount),    //                          .burstcount
		.uav_read               (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_read),          //                          .read
		.uav_write              (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_write),         //                          .write
		.uav_waitrequest        (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_waitrequest),   //                          .waitrequest
		.uav_readdatavalid      (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_readdatavalid), //                          .readdatavalid
		.uav_byteenable         (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_byteenable),    //                          .byteenable
		.uav_readdata           (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_readdata),      //                          .readdata
		.uav_writedata          (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_writedata),     //                          .writedata
		.uav_lock               (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_lock),          //                          .lock
		.uav_debugaccess        (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_debugaccess),   //                          .debugaccess
		.av_address             (DDR3_manager_0_avalon_master_address),                                            //      avalon_anti_master_0.address
		.av_waitrequest         (DDR3_manager_0_avalon_master_waitrequest),                                        //                          .waitrequest
		.av_burstcount          (DDR3_manager_0_avalon_master_burstcount),                                         //                          .burstcount
		.av_byteenable          (DDR3_manager_0_avalon_master_byteenable),                                         //                          .byteenable
		.av_read                (DDR3_manager_0_avalon_master_read),                                               //                          .read
		.av_readdata            (DDR3_manager_0_avalon_master_readdata),                                           //                          .readdata
		.av_readdatavalid       (DDR3_manager_0_avalon_master_readdatavalid),                                      //                          .readdatavalid
		.av_write               (DDR3_manager_0_avalon_master_write),                                              //                          .write
		.av_writedata           (DDR3_manager_0_avalon_master_writedata),                                          //                          .writedata
		.av_beginbursttransfer  (1'b0),                                                                            //               (terminated)
		.av_begintransfer       (1'b0),                                                                            //               (terminated)
		.av_chipselect          (1'b0),                                                                            //               (terminated)
		.av_lock                (1'b0),                                                                            //               (terminated)
		.av_debugaccess         (1'b0),                                                                            //               (terminated)
		.uav_clken              (),                                                                                //               (terminated)
		.av_clken               (1'b1),                                                                            //               (terminated)
		.uav_response           (2'b00),                                                                           //               (terminated)
		.av_response            (),                                                                                //               (terminated)
		.uav_writeresponsevalid (1'b0),                                                                            //               (terminated)
		.av_writeresponsevalid  ()                                                                                 //               (terminated)
	);

	altera_merlin_slave_translator #(
		.AV_ADDRESS_W                   (30),
		.AV_DATA_W                      (32),
		.UAV_DATA_W                     (32),
		.AV_BURSTCOUNT_W                (8),
		.AV_BYTEENABLE_W                (4),
		.UAV_BYTEENABLE_W               (4),
		.UAV_ADDRESS_W                  (32),
		.UAV_BURSTCOUNT_W               (10),
		.AV_READLATENCY                 (0),
		.USE_READDATAVALID              (1),
		.USE_WAITREQUEST                (1),
		.USE_UAV_CLKEN                  (0),
		.USE_READRESPONSE               (0),
		.USE_WRITERESPONSE              (0),
		.AV_SYMBOLS_PER_WORD            (4),
		.AV_ADDRESS_SYMBOLS             (0),
		.AV_BURSTCOUNT_SYMBOLS          (0),
		.AV_CONSTANT_BURST_BEHAVIOR     (0),
		.UAV_CONSTANT_BURST_BEHAVIOR    (0),
		.AV_REQUIRE_UNALIGNED_ADDRESSES (0),
		.CHIPSELECT_THROUGH_READLATENCY (0),
		.AV_READ_WAIT_CYCLES            (1),
		.AV_WRITE_WAIT_CYCLES           (0),
		.AV_SETUP_WAIT_CYCLES           (0),
		.AV_DATA_HOLD_CYCLES            (0)
	) hps_0_f2h_sdram0_data_translator (
		.clk                    (clk_0_clk_clk),                                                                   //                      clk.clk
		.reset                  (hps_0_f2h_sdram0_data_translator_reset_reset_bridge_in_reset_reset),              //                    reset.reset
		.uav_address            (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_address),       // avalon_universal_slave_0.address
		.uav_burstcount         (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_burstcount),    //                         .burstcount
		.uav_read               (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_read),          //                         .read
		.uav_write              (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_write),         //                         .write
		.uav_waitrequest        (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_waitrequest),   //                         .waitrequest
		.uav_readdatavalid      (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_readdatavalid), //                         .readdatavalid
		.uav_byteenable         (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_byteenable),    //                         .byteenable
		.uav_readdata           (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_readdata),      //                         .readdata
		.uav_writedata          (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_writedata),     //                         .writedata
		.uav_lock               (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_lock),          //                         .lock
		.uav_debugaccess        (ddr3_manager_0_avalon_master_translator_avalon_universal_master_0_debugaccess),   //                         .debugaccess
		.av_address             (hps_0_f2h_sdram0_data_address),                                                   //      avalon_anti_slave_0.address
		.av_write               (hps_0_f2h_sdram0_data_write),                                                     //                         .write
		.av_read                (hps_0_f2h_sdram0_data_read),                                                      //                         .read
		.av_readdata            (hps_0_f2h_sdram0_data_readdata),                                                  //                         .readdata
		.av_writedata           (hps_0_f2h_sdram0_data_writedata),                                                 //                         .writedata
		.av_burstcount          (hps_0_f2h_sdram0_data_burstcount),                                                //                         .burstcount
		.av_byteenable          (hps_0_f2h_sdram0_data_byteenable),                                                //                         .byteenable
		.av_readdatavalid       (hps_0_f2h_sdram0_data_readdatavalid),                                             //                         .readdatavalid
		.av_waitrequest         (hps_0_f2h_sdram0_data_waitrequest),                                               //                         .waitrequest
		.av_begintransfer       (),                                                                                //              (terminated)
		.av_beginbursttransfer  (),                                                                                //              (terminated)
		.av_writebyteenable     (),                                                                                //              (terminated)
		.av_lock                (),                                                                                //              (terminated)
		.av_chipselect          (),                                                                                //              (terminated)
		.av_clken               (),                                                                                //              (terminated)
		.uav_clken              (1'b0),                                                                            //              (terminated)
		.av_debugaccess         (),                                                                                //              (terminated)
		.av_outputenable        (),                                                                                //              (terminated)
		.uav_response           (),                                                                                //              (terminated)
		.av_response            (2'b00),                                                                           //              (terminated)
		.uav_writeresponsevalid (),                                                                                //              (terminated)
		.av_writeresponsevalid  (1'b0)                                                                             //              (terminated)
	);

endmodule
