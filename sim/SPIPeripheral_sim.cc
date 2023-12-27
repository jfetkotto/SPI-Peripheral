#include <sys/types.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "../verilated/VSPIPeripheral.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#define MAX_TICKS 2000

vluint64_t simTicks = 0;
uint8_t spiPosEdges = 0;
uint8_t bytesTransferred = 0;

void mainTick(VSPIPeripheral *dut) {
    if (simTicks % 2 == 0) {
        dut->i_clk ^= 1;
    }
}

void spiTick(VSPIPeripheral *dut) {
    static bool oldClock = false;
    oldClock = dut->i_SPI_CLK;

    if (simTicks % 10 == 0) {
        dut->i_SPI_CLK ^= 1;
    }

    if (!oldClock && dut->i_SPI_CLK) {
        spiPosEdges++;
    }
}

void sendBytes(VSPIPeripheral *dut, uint8_t txBytes[], uint8_t maxBytes) {
    static uint8_t i = 0;

    if (simTicks % 10 == 0 && bytesTransferred < maxBytes) {
        i = spiPosEdges % 8;

        dut->i_SPI_CS_n = 0;

        if ((txBytes[bytesTransferred] << i) & 0x80) {
            dut->i_SPI_PICO = 1;
        } else {
            dut->i_SPI_PICO = 0;
        }
    }
}

void handleCs(VSPIPeripheral *dut) {
    static uint8_t i = 0;
    uint8_t iOld = 0;

    if (simTicks % 10 == 0) {
        iOld = i;
        i = spiPosEdges % 8;
        if (iOld == 7 && i == 0) {
            dut->i_SPI_CS_n = 1;
        }
    }
}

void handleBytesTransferred(VSPIPeripheral *dut, uint8_t maxBytes) {
    static uint8_t i = 0;
    uint8_t iOld = 0;

    if (simTicks % 10 == 0) {
        iOld = i;
        i = spiPosEdges % 8;
        if (iOld == 7 && i == 0) {
            if (bytesTransferred < maxBytes) {
                bytesTransferred++;
            }
        }
    }
}

int main(int argc, char **argv) {
    VSPIPeripheral *dut = new VSPIPeripheral;
    Verilated::traceEverOn(true);
    VerilatedVcdC *m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 99);
    m_trace->open("SPIPeripheral_sim.vcd");
    dut->i_SPI_CS_n = 1;
    dut->eval();
    dut->i_SPI_CS_n = 0;
    dut->eval();
    dut->i_SPI_CS_n = 1;

    dut->i_txData = 0xde;
    dut->i_txDataValid = 1;

    uint8_t txBytes[] = {0xde, 0xad, 0xbe, 0xef, 0x00, 0xca, 0xfe, 0xba, 0xbe};

    while (simTicks < MAX_TICKS) {
        mainTick(dut);
        if (simTicks > 25) {
            sendBytes(dut, txBytes, 9);
            handleCs(dut);
            handleBytesTransferred(dut, 9);
            spiTick(dut);
            dut->i_txDataValid = 0;
        }
        dut->eval();
        m_trace->dump(simTicks);
        simTicks++;
    }

    m_trace->close();
    delete m_trace;
    m_trace = NULL;
    delete dut;
    dut = NULL;

    exit(EXIT_SUCCESS);
}
