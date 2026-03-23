//******************************************************************************
// @file:    ad5940_spi_master.v
// @author:  FPGA Development Team
// @brief:   SPI Master interface for AD5940 electrochemical AFE
// @version: 1.0
// @date:    2026-03-23
//------------------------------------------------------------------------------
// Copyright (c) 2026 Electrochemical Workstation Project
//
// SPI Master controller for AD5940 communication
// - Supports Mode 0 (CPOL=0, CPHA=0)
// - Configurable clock divider
// - 8-bit/16-bit/32-bit transaction support
// - Full register read/write capability
//******************************************************************************

module ad5940_spi_master #(
    parameter CLK_FREQ      = 50_000_000,   // System clock frequency in Hz
    parameter SPI_FREQ      = 10_000_000,   // SPI clock frequency in Hz (max 16MHz for AD5940)
    parameter ADDR_WIDTH    = 16,           // AD5940 address width
    parameter DATA_WIDTH    = 32            // AD5940 data width
)(
    // System interface
    input  wire                     clk,            // System clock
    input  wire                     rst_n,          // Active-low reset

    // Control interface
    input  wire                     start,          // Start SPI transaction
    input  wire                     rw,             // Read(1)/Write(0)
    input  wire [ADDR_WIDTH-1:0]    addr,           // Register address
    input  wire [DATA_WIDTH-1:0]    wr_data,        // Write data
    output reg  [DATA_WIDTH-1:0]    rd_data,        // Read data
    output reg                      busy,           // Transaction in progress
    output reg                      done,           // Transaction complete

    // SPI interface to AD5940
    output reg                      spi_cs_n,       // Chip select (active low)
    output reg                      spi_sclk,       // SPI clock
    output reg                      spi_mosi,       // Master out, slave in
    input  wire                     spi_miso        // Master in, slave out
);

    // Calculate clock divider
    localparam CLK_DIV = CLK_FREQ / (2 * SPI_FREQ);

    // State machine states
    localparam IDLE         = 3'b000;
    localparam SEND_CMD     = 3'b001;
    localparam SEND_ADDR    = 3'b010;
    localparam SEND_DATA    = 3'b011;
    localparam RECV_DATA    = 3'b100;
    localparam FINISH       = 3'b101;

    // Registers
    reg [2:0]               state;
    reg [7:0]               bit_cnt;
    reg [15:0]              clk_cnt;
    reg [DATA_WIDTH-1:0]    shift_reg;
    reg [ADDR_WIDTH-1:0]    addr_reg;
    reg                     rw_reg;
    reg                     spi_clk_en;

    // AD5940 SPI Command format
    // Bit[15]: R/W (1=Read, 0=Write)
    // Bit[14:13]: Reserved (00)
    // Bit[12:0]: Address
    localparam CMD_WIDTH = 16;

    // Clock divider for SPI clock generation
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            clk_cnt <= 16'd0;
            spi_sclk <= 1'b0;
        end else begin
            if (spi_clk_en) begin
                if (clk_cnt >= CLK_DIV - 1) begin
                    clk_cnt <= 16'd0;
                    spi_sclk <= ~spi_sclk;
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end else begin
                clk_cnt <= 16'd0;
                spi_sclk <= 1'b0;
            end
        end
    end

    // Main state machine
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            busy <= 1'b0;
            done <= 1'b0;
            spi_cs_n <= 1'b1;
            spi_mosi <= 1'b0;
            bit_cnt <= 8'd0;
            shift_reg <= {DATA_WIDTH{1'b0}};
            addr_reg <= {ADDR_WIDTH{1'b0}};
            rw_reg <= 1'b0;
            spi_clk_en <= 1'b0;
            rd_data <= {DATA_WIDTH{1'b0}};
        end else begin
            case (state)
                IDLE: begin
                    done <= 1'b0;
                    spi_cs_n <= 1'b1;
                    spi_clk_en <= 1'b0;

                    if (start && !busy) begin
                        busy <= 1'b1;
                        addr_reg <= addr;
                        rw_reg <= rw;
                        shift_reg <= wr_data;
                        spi_cs_n <= 1'b0;
                        bit_cnt <= 8'd0;
                        state <= SEND_CMD;
                        spi_clk_en <= 1'b1;
                    end
                end

                SEND_CMD: begin
                    // Send 16-bit command (R/W bit + address)
                    if (spi_sclk && clk_cnt == 0) begin  // Rising edge of SPI clock
                        if (bit_cnt < CMD_WIDTH) begin
                            // MSB first: send R/W bit followed by address
                            if (bit_cnt == 0)
                                spi_mosi <= rw_reg;
                            else if (bit_cnt < CMD_WIDTH)
                                spi_mosi <= addr_reg[CMD_WIDTH - 1 - bit_cnt];

                            bit_cnt <= bit_cnt + 1'b1;
                        end else begin
                            bit_cnt <= 8'd0;
                            if (rw_reg)
                                state <= RECV_DATA;
                            else
                                state <= SEND_DATA;
                        end
                    end
                end

                SEND_DATA: begin
                    // Send 32-bit data for write operation
                    if (spi_sclk && clk_cnt == 0) begin
                        if (bit_cnt < DATA_WIDTH) begin
                            spi_mosi <= shift_reg[DATA_WIDTH - 1 - bit_cnt];
                            bit_cnt <= bit_cnt + 1'b1;
                        end else begin
                            state <= FINISH;
                        end
                    end
                end

                RECV_DATA: begin
                    // Receive 32-bit data for read operation
                    if (!spi_sclk && clk_cnt == 0) begin  // Falling edge of SPI clock
                        if (bit_cnt < DATA_WIDTH) begin
                            shift_reg[DATA_WIDTH - 1 - bit_cnt] <= spi_miso;
                            bit_cnt <= bit_cnt + 1'b1;
                        end else begin
                            rd_data <= shift_reg;
                            state <= FINISH;
                        end
                    end
                end

                FINISH: begin
                    spi_cs_n <= 1'b1;
                    spi_clk_en <= 1'b0;
                    done <= 1'b1;
                    busy <= 1'b0;
                    state <= IDLE;
                end

                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
