#include <cstring>
#include <iostream>

#include "rv32core.h"
#include "rv32macros.h"

#include "binary_data.h"

uint8_t *image = nullptr;

uint32_t ram_amt = 64 * 1024 * 1024;

extern int run(RV32Core &core);

static void DumpState(RV32Core *core, uint8_t *ram_image);

static uint64_t GetTimeMicroseconds();

int main(int argc, char *argv[]) {
    // Allocate image space
    image = new uint8_t[ram_amt];
    memcpy(image, binary_data, sizeof(binary_data));

    RV32Core core;

    auto time1 = GetTimeMicroseconds();
    int ret = run(core);
    auto time2 = GetTimeMicroseconds();

    std::cout << ret << std::endl;

    // Remove image
    delete[] image;

    DumpState(&core, image);

    std::cout << "timeout: " << time2 - time1 << std::endl;
    return 0;
}

static void DumpState(RV32Core *core, uint8_t *ram_image) {
    uint32_t pc = core->pc;
    uint32_t pc_offset = pc - MINIRV32_RAM_IMAGE_OFFSET;
    uint32_t ir = 0;

    printf("PC: %08x ", pc);
    if (pc_offset >= 0 && pc_offset < ram_amt - 3) {
        ir = *((uint32_t *) (&((uint8_t *) ram_image)[pc_offset]));
        printf("[0x%08x] ", ir);
    } else
        printf("[xxxxxxxxxx] ");
    uint32_t *regs = core->regs;
    printf("Z:%08x ra:%08x sp:%08x gp:%08x tp:%08x t0:%08x t1:%08x t2:%08x s0:%08x s1:%08x a0:%08x a1:%08x a2:%08x "
           "a3:%08x a4:%08x a5:%08x ",
           regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9], regs[10], regs[11],
           regs[12], regs[13], regs[14], regs[15]);
    printf("a6:%08x a7:%08x s2:%08x s3:%08x s4:%08x s5:%08x s6:%08x s7:%08x s8:%08x s9:%08x s10:%08x s11:%08x t3:%08x "
           "t4:%08x t5:%08x t6:%08x\n",
           regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23], regs[24], regs[25], regs[26],
           regs[27], regs[28], regs[29], regs[30], regs[31]);
}

#if defined(WINDOWS) || defined(WIN32) || defined(_WIN32)

#    include <conio.h>
#    include <windows.h>

uint64_t GetTimeMicroseconds() {
    static LARGE_INTEGER lpf;
    LARGE_INTEGER li;

    if (!lpf.QuadPart)
        QueryPerformanceFrequency(&lpf);

    QueryPerformanceCounter(&li);
    return ((uint64_t) li.QuadPart * 1000000LL) / (uint64_t) lpf.QuadPart;
}

#else

#    include <sys/time.h>

static uint64_t GetTimeMicroseconds() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_usec + ((uint64_t) (tv.tv_sec)) * 1000000LL;
}

#endif