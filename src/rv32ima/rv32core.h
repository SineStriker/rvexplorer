#ifndef RV32CORE_H
#define RV32CORE_H

#include <cstdint>
#include <cstring>


struct RV32Core {
    uint32_t regs[32];

    uint32_t pc;
    uint32_t mstatus;
    uint32_t cyclel;
    uint32_t cycleh;

    uint32_t timerl;
    uint32_t timerh;
    uint32_t timermatchl;
    uint32_t timermatchh;

    uint32_t mscratch;
    uint32_t mtvec;
    uint32_t mie;
    uint32_t mip;

    uint32_t mepc;
    uint32_t mtval;
    uint32_t mcause;

    // Note: only a few bits are used.  (Machine = 3, User = 0)
    // Bits 0..1 = privilege.
    // Bit 2 = WFI (Wait for interrupt)
    // Bit 3 = Load/Store has a reservation.
    uint32_t extraflags;

    RV32Core() {
        memset(this, 0, sizeof(RV32Core));
    }
};

#endif // RV32CORE_H
