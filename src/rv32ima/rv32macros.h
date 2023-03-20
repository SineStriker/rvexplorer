#ifndef RV32MACROS_H
#define RV32MACROS_H

#include <stdint.h>

extern uint8_t *image;

extern uint32_t ram_amt;

#ifndef MINIRV32_CUSTOM_MEMORY_BUS
#    define MINIRV32_STORE4(ofs, val) *(uint32_t *) (image + ofs) = val
#    define MINIRV32_STORE2(ofs, val) *(uint16_t *) (image + ofs) = val
#    define MINIRV32_STORE1(ofs, val) *(uint8_t *) (image + ofs) = val
#    define MINIRV32_LOAD4(ofs)       *(uint32_t *) (image + ofs)
#    define MINIRV32_LOAD2(ofs)       *(uint16_t *) (image + ofs)
#    define MINIRV32_LOAD1(ofs)       *(uint8_t *) (image + ofs)
#endif

#define MINI_RV32_RAM_SIZE ram_amt

#define MINIRV32_RAM_IMAGE_OFFSET 0x80000000

#endif // RV32MACROS_H
