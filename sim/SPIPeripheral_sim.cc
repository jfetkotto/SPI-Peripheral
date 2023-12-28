#include <sys/types.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "../verilated/VSPIPeripheral.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#define MAIN_CLK_DELAY 2
#define SPI_CLK_DELAY 10

#define MAX_TICKS 2000
#define MAX_BYTES 9
#define SPI_WORD_LENGTH 8

vluint64_t simTicks = 0;
uint8_t spiPosEdges = 0;
uint8_t bytesTransmitted = 0;

void mainTick(VSPIPeripheral *dut) {
    if (simTicks % MAIN_CLK_DELAY == 0) {
        dut->i_clk ^= 1;
    }
}

void spiTick(VSPIPeripheral *dut) {
    static bool oldClock = false;
    oldClock = dut->i_SPI_CLK;

    if (simTicks % SPI_CLK_DELAY == 0) {
        dut->i_SPI_CLK ^= 1;
    }

    if (!oldClock && dut->i_SPI_CLK) {
        spiPosEdges++;
    }
}

void sendBytes(VSPIPeripheral *dut, uint8_t *txBytes, uint8_t maxBytes) {
    static uint8_t i = 0;

    if (simTicks % SPI_CLK_DELAY == 0 && bytesTransmitted < maxBytes) {
        i = spiPosEdges % SPI_WORD_LENGTH;

        dut->i_SPI_CS_n = 0;

        if ((txBytes[bytesTransmitted] << i) & 0x80) {
            dut->i_SPI_PICO = 1;
        } else {
            dut->i_SPI_PICO = 0;
        }
    }
}

void handleCs(VSPIPeripheral *dut) {
    static uint8_t i = 0;
    uint8_t iOld = 0;

    if (simTicks % SPI_CLK_DELAY == 0) {
        iOld = i;
        i = spiPosEdges % SPI_WORD_LENGTH;
        if (iOld == 7 && i == 0) {
            dut->i_SPI_CS_n = 1;
        }
    }
}

void handleBytesTransferred(VSPIPeripheral *dut, uint8_t maxBytes) {
    static uint8_t i = 0;
    uint8_t iOld = 0;

    if (simTicks % SPI_CLK_DELAY == 0) {
        iOld = i;
        i = spiPosEdges % SPI_WORD_LENGTH;
        if (iOld == 7 && i == 0) {
            if (bytesTransmitted < maxBytes) {
                bytesTransmitted++;
            }
        }
    }
}

void loopback(VSPIPeripheral *dut) {
    bool rxDataValidOld = 0;
    static bool rxDataValid = 0;

    if (simTicks % SPI_CLK_DELAY == 0) {
        rxDataValidOld = rxDataValid;
        rxDataValid = dut->o_rxDataValid;
        if (!rxDataValidOld && rxDataValid) {
            dut->i_txData = dut->o_rxData;
            dut->i_txDataValid = 1;
        } else {
            dut->i_txDataValid = 0;
        }
    }
}

void receiveBytes(VSPIPeripheral *dut, uint8_t *rxBytes, uint8_t maxBytes) {
    bool spiClkOld;
    static bool spiClk;

    static uint8_t rxBits = 0;
    static uint8_t currentByte;

    spiClkOld = spiClk;
    spiClk = dut->i_SPI_CLK;

    if (!spiClkOld && spiClk) {
        uint8_t bitSet = dut->b_SPI_POCI;
        currentByte <<= 1;
        currentByte |= bitSet;
        rxBits++;
    }

    if (rxBits == SPI_WORD_LENGTH) {
        rxBytes[bytesTransmitted] = currentByte;
        rxBits = 0;
        currentByte = 0;
    }
}

bool checkBytes(uint8_t *txBytes, uint8_t *rxBytes) {
    return txBytes[0] == rxBytes[1] && txBytes[1] == rxBytes[2] &&
           txBytes[2] == rxBytes[3] && txBytes[3] == rxBytes[4];
}

int main(int argc, char **argv) {
    VSPIPeripheral *dut = new VSPIPeripheral;
    Verilated::traceEverOn(true);
    VerilatedVcdC *m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 99);
    m_trace->open("SPIPeripheral_sim.vcd");
    uint8_t txBytes[MAX_BYTES] = {0xde, 0xad, 0xbe, 0xef, 0x00,
                                  0xca, 0xfe, 0xba, 0xbe};
    uint8_t rxBytes[MAX_BYTES] = {};

    while (simTicks < MAX_TICKS) {
        mainTick(dut);
        if (simTicks > 25) {
            loopback(dut);
            sendBytes(dut, txBytes, MAX_BYTES);
            receiveBytes(dut, rxBytes, MAX_BYTES);
            handleCs(dut);
            handleBytesTransferred(dut, MAX_BYTES);
            spiTick(dut);
        }
        dut->eval();
        m_trace->dump(simTicks);
        simTicks++;
    }

    if (checkBytes(txBytes, rxBytes)) {
        printf("SUCCESS: Tx bytes match Rx bytes :)\n");
    } else {
        printf("FAILURE: Tx bytes mismatch Rx bytes :(\n");
    }

    m_trace->close();
    delete m_trace;
    m_trace = NULL;
    delete dut;
    dut = NULL;

    exit(EXIT_SUCCESS);
}
