/*
*  SPISPeripheral.sv
*  Mode 0 SPI Peripheral only
*/

module SPIPeripheral
  #(// NO PARAMS
  )(// Control/data signals
    input  var logic i_rst // Active high reset
  , input  var logic i_clk // Main clk
  , input  var logic i_txDataValid  // Tx Data valid pulse
  , input  var logic [7:0] i_txData // Byte to SPI Controller
  , output var logic o_rxDataValid  // Rx Data valid pulse
  , output var logic [7:0] o_rxData // Byte from SPI Controller
  // SPI interface to top ports
  , input  var logic i_SPI_CLK
  , input  var logic i_SPI_PICO
  , input  var logic i_SPI_CS_n
  , inout  var logic b_SPI_POCI
  );

  logic spiPociMux;

  logic [2:0] rxBitCount_rg;
  logic [2:0] txBitCount_rg;
  logic [7:0] tempRxByte_rg;
  logic [7:0] rxByte_rg;
  logic [7:0] txByte_rg;
  logic spiPociBit_rg;
  logic preloadPoci_rg;

  // CDC Sync flops
  logic rxDone_rg;
  logic rxDoneMeta_rg;
  logic rxDoneSync_rg;
  logic rxDonePe;

  // {{{ Receive Rx byte in SPI clock domain
  // Rx bit counter
  always_ff @( posedge i_SPI_CLK, posedge i_SPI_CS_n)
    if ( i_SPI_CS_n ) rxBitCount_rg <= '0;
    else rxBitCount_rg <= rxBitCount_rg + '1;

  // Rx done
  always_ff @( posedge i_SPI_CLK, posedge i_SPI_CS_n )
    if ( i_SPI_CS_n ) rxDone_rg <= '0;
    else rxDone_rg <= &rxBitCount_rg;

  // Shift in bits
  always_ff @( posedge i_SPI_CLK, posedge i_SPI_CS_n)
    if ( i_SPI_CS_n ) tempRxByte_rg <= tempRxByte_rg;
    else tempRxByte_rg <= {tempRxByte_rg[6:0], i_SPI_PICO};

  always_ff @( posedge i_SPI_CLK, posedge i_SPI_CS_n )
    if ( i_SPI_CS_n ) rxByte_rg <= rxByte_rg;
    else rxByte_rg <= &rxBitCount_rg ? {tempRxByte_rg[6:0], i_SPI_PICO}
                                     : rxByte_rg;
  // }}} Receive Rx byte in SPI clock domain

  // {{{ CDC
  // Sync to main clock domain via 2xSync flops
  // generate data valid pulse
  always_ff @( posedge i_clk, posedge i_rst )
    if ( i_rst ) rxDoneMeta_rg <= '0;
    else rxDoneMeta_rg <= rxDone_rg;

  always_ff @( posedge i_clk, posedge i_rst )
    if ( i_rst ) rxDoneSync_rg <= '0;
    else rxDoneSync_rg <= rxDoneMeta_rg;

  always_comb
    rxDonePe =
      rxDoneMeta_rg && !rxDoneSync_rg;

  // Flag Rx complete
  always_ff @( posedge i_clk, posedge i_rst )
    if ( i_rst ) o_rxDataValid <= '0;
    else o_rxDataValid <= rxDonePe;

  // Load Rx byte
  always_ff @( posedge i_clk, posedge i_rst )
    if ( i_rst ) o_rxData <= '0;
    else o_rxData <= rxDonePe ? rxByte_rg : o_rxData;
  // }}} CDC to main clock domain

  // {{{ Transmit Tx byte
  always_ff @( posedge i_SPI_CLK, posedge i_SPI_CS_n )
    if ( i_SPI_CS_n ) txBitCount_rg <= 3'b111;
    else txBitCount_rg <= txBitCount_rg - '1;

  always_ff @( posedge i_SPI_CLK )
    spiPociBit_rg <= txByte_rg[txBitCount_rg];

  always_ff @( posedge i_clk, posedge i_rst )
    if ( i_rst ) txByte_rg <= '0;
    else txByte_rg <= i_txDataValid ? i_txData : txByte_rg;

  always_ff @( posedge i_SPI_CLK, posedge i_SPI_CS_n )
    if ( i_SPI_CS_n ) preloadPoci_rg <= '1;
    else preloadPoci_rg <= '0;

  always_comb
    spiPociMux =
      preloadPoci_rg ? txByte_rg[7] : spiPociBit_rg;

  always_comb
    b_SPI_POCI =
      i_SPI_CS_n ? 1'bZ : spiPociMux;
  // }}} Transmit Tx byte
endmodule
