//******************************************************************************
// @file:    ad5940_controller.v
// @author:  FPGA Development Team
// @brief:   Top-level controller for AD5940 electrochemical workstation
// @version: 1.0
// @date:    2026-03-23
//------------------------------------------------------------------------------
// Copyright (c) 2026 Electrochemical Workstation Project
//
// Main FPGA controller for single-channel electrochemical workstation
// Features:
// - Control AD5940 via SPI
// - Support multiple electrochemical methods
// - Configurable measurement parameters
// - Data acquisition and processing
//******************************************************************************

module ad5940_controller #(
    parameter CLK_FREQ      = 50_000_000,   // System clock frequency
    parameter SPI_FREQ      = 10_000_000,   // SPI clock frequency
    parameter FIFO_DEPTH    = 1024          // Data FIFO depth
)(
    // System interface
    input  wire                     clk,            // System clock
    input  wire                     rst_n,          // Active-low reset

    // Host interface (UART, USB, or parallel bus)
    input  wire [7:0]               host_cmd,       // Command from host
    input  wire [31:0]              host_data,      // Data from host
    input  wire                     host_cmd_valid, // Command valid
    output reg  [31:0]              host_rd_data,   // Read data to host
    output reg                      host_rd_valid,  // Read data valid
    output reg                      host_busy,      // Controller busy

    // SPI interface to AD5940
    output wire                     ad5940_cs_n,    // Chip select
    output wire                     ad5940_sclk,    // SPI clock
    output wire                     ad5940_mosi,    // SPI MOSI
    input  wire                     ad5940_miso,    // SPI MISO

    // GPIO control
    output reg                      ad5940_reset_n, // AD5940 hardware reset
    input  wire                     ad5940_int0,    // Interrupt 0 from AD5940
    input  wire                     ad5940_int1,    // Interrupt 1 from AD5940

    // Status LEDs
    output reg                      led_busy,       // Busy indicator
    output reg                      led_ready,      // Ready indicator
    output reg                      led_error       // Error indicator
);

    // Host command definitions
    localparam CMD_RESET            = 8'h01;    // Reset AD5940
    localparam CMD_INIT             = 8'h02;    // Initialize AD5940
    localparam CMD_CONFIG_CV        = 8'h10;    // Configure Cyclic Voltammetry
    localparam CMD_CONFIG_DPV       = 8'h11;    // Configure Differential Pulse Voltammetry
    localparam CMD_CONFIG_CA        = 8'h12;    // Configure Chronoamperometry
    localparam CMD_CONFIG_POT       = 8'h13;    // Configure Potentiometry
    localparam CMD_START_MEAS       = 8'h20;    // Start measurement
    localparam CMD_STOP_MEAS        = 8'h21;    // Stop measurement
    localparam CMD_READ_DATA        = 8'h30;    // Read measurement data
    localparam CMD_READ_STATUS      = 8'h31;    // Read status
    localparam CMD_WRITE_REG        = 8'h40;    // Write AD5940 register
    localparam CMD_READ_REG         = 8'h41;    // Read AD5940 register

    // Controller states
    localparam STATE_IDLE           = 4'h0;
    localparam STATE_RESET          = 4'h1;
    localparam STATE_INIT           = 4'h2;
    localparam STATE_CONFIG         = 4'h3;
    localparam STATE_MEASURE        = 4'h4;
    localparam STATE_READ_DATA      = 4'h5;
    localparam STATE_ERROR          = 4'hF;

    // Internal registers
    reg [3:0]               state;
    reg [3:0]               next_state;
    reg [7:0]               error_code;
    reg [31:0]              config_reg[0:15];   // Configuration registers

    // SPI interface signals
    reg                     spi_start;
    reg                     spi_rw;
    reg [15:0]              spi_addr;
    reg [31:0]              spi_wr_data;
    wire [31:0]             spi_rd_data;
    wire                    spi_busy;
    wire                    spi_done;

    // Measurement configuration
    reg [7:0]               meas_type;          // Measurement type
    reg                     meas_active;        // Measurement active flag
    reg [31:0]              data_count;         // Number of data points collected

    // FIFO signals
    reg                     fifo_wr_en;
    reg [31:0]              fifo_wr_data;
    wire                    fifo_full;
    wire                    fifo_empty;
    wire [9:0]              fifo_count;

    // SPI Master instantiation
    ad5940_spi_master #(
        .CLK_FREQ(CLK_FREQ),
        .SPI_FREQ(SPI_FREQ),
        .ADDR_WIDTH(16),
        .DATA_WIDTH(32)
    ) spi_master (
        .clk(clk),
        .rst_n(rst_n),
        .start(spi_start),
        .rw(spi_rw),
        .addr(spi_addr),
        .wr_data(spi_wr_data),
        .rd_data(spi_rd_data),
        .busy(spi_busy),
        .done(spi_done),
        .spi_cs_n(ad5940_cs_n),
        .spi_sclk(ad5940_sclk),
        .spi_mosi(ad5940_mosi),
        .spi_miso(ad5940_miso)
    );

    // Main state machine
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= STATE_IDLE;
            ad5940_reset_n <= 1'b0;
            host_busy <= 1'b0;
            host_rd_valid <= 1'b0;
            led_busy <= 1'b0;
            led_ready <= 1'b0;
            led_error <= 1'b0;
            error_code <= 8'h00;
            meas_active <= 1'b0;
            data_count <= 32'h0;
            spi_start <= 1'b0;
        end else begin
            state <= next_state;

            // Handle host commands
            if (host_cmd_valid && !host_busy) begin
                case (host_cmd)
                    CMD_RESET: begin
                        next_state <= STATE_RESET;
                        host_busy <= 1'b1;
                    end

                    CMD_INIT: begin
                        next_state <= STATE_INIT;
                        host_busy <= 1'b1;
                    end

                    CMD_CONFIG_CV,
                    CMD_CONFIG_DPV,
                    CMD_CONFIG_CA,
                    CMD_CONFIG_POT: begin
                        meas_type <= host_cmd;
                        next_state <= STATE_CONFIG;
                        host_busy <= 1'b1;
                    end

                    CMD_START_MEAS: begin
                        meas_active <= 1'b1;
                        next_state <= STATE_MEASURE;
                        host_busy <= 1'b1;
                    end

                    CMD_STOP_MEAS: begin
                        meas_active <= 1'b0;
                        next_state <= STATE_IDLE;
                    end

                    CMD_READ_DATA: begin
                        next_state <= STATE_READ_DATA;
                        host_busy <= 1'b1;
                    end

                    CMD_READ_STATUS: begin
                        host_rd_data <= {24'h0, state, error_code[3:0]};
                        host_rd_valid <= 1'b1;
                    end

                    CMD_WRITE_REG: begin
                        spi_start <= 1'b1;
                        spi_rw <= 1'b0;
                        spi_addr <= host_data[31:16];
                        spi_wr_data <= host_data[15:0];
                    end

                    CMD_READ_REG: begin
                        spi_start <= 1'b1;
                        spi_rw <= 1'b1;
                        spi_addr <= host_data[31:16];
                    end

                    default: begin
                        error_code <= 8'h01; // Unknown command
                    end
                endcase
            end

            // Update status LEDs
            led_busy <= (state != STATE_IDLE);
            led_ready <= (state == STATE_IDLE) && !led_error;
        end
    end

    // State transition logic
    always @(*) begin
        next_state = state;

        case (state)
            STATE_IDLE: begin
                // Wait for host commands
            end

            STATE_RESET: begin
                // Perform reset sequence
                if (!spi_busy) begin
                    next_state = STATE_IDLE;
                end
            end

            STATE_INIT: begin
                // Initialize AD5940
                if (!spi_busy) begin
                    next_state = STATE_IDLE;
                end
            end

            STATE_CONFIG: begin
                // Configure measurement parameters
                if (!spi_busy) begin
                    next_state = STATE_IDLE;
                end
            end

            STATE_MEASURE: begin
                // Perform measurement
                if (!meas_active) begin
                    next_state = STATE_IDLE;
                end
            end

            STATE_READ_DATA: begin
                // Read data from FIFO
                if (fifo_empty) begin
                    next_state = STATE_IDLE;
                end
            end

            STATE_ERROR: begin
                // Error state
                next_state = STATE_IDLE;
            end

            default: begin
                next_state = STATE_IDLE;
            end
        endcase
    end

    // Data FIFO (simplified - would typically use block RAM)
    // This is a placeholder - actual implementation would use FPGA-specific FIFO IP
    reg [31:0] data_fifo [0:FIFO_DEPTH-1];
    reg [9:0] fifo_wr_ptr;
    reg [9:0] fifo_rd_ptr;

    assign fifo_full = (fifo_wr_ptr + 1'b1 == fifo_rd_ptr);
    assign fifo_empty = (fifo_wr_ptr == fifo_rd_ptr);
    assign fifo_count = fifo_wr_ptr - fifo_rd_ptr;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            fifo_wr_ptr <= 10'h0;
            fifo_rd_ptr <= 10'h0;
        end else begin
            if (fifo_wr_en && !fifo_full) begin
                data_fifo[fifo_wr_ptr] <= fifo_wr_data;
                fifo_wr_ptr <= fifo_wr_ptr + 1'b1;
            end

            if (state == STATE_READ_DATA && !fifo_empty) begin
                host_rd_data <= data_fifo[fifo_rd_ptr];
                host_rd_valid <= 1'b1;
                fifo_rd_ptr <= fifo_rd_ptr + 1'b1;
            end
        end
    end

endmodule
