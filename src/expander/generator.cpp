#include "generator.h"

#include <sstream>
#include <string>

static std::string dec2hex(uint32_t i, size_t width = 8) {
    std::stringstream ioss; // 定义字符串流
    std::string s_temp;     // 存放转化后字符
    ioss << std::hex << i;  // 以十六制形式输出
    ioss >> s_temp;
    std::string s(width > s_temp.size() ? width - s_temp.size() : 0, '0'); // 补0
    s += s_temp;                                                           // 合并
    return s;
}

#ifndef MINIRV32_CUSTOM_MEMORY_BUS
#    define MINIRV32_STORE4(ofs, val) *(uint32_t *) (image + ofs) = val
#    define MINIRV32_STORE2(ofs, val) *(uint16_t *) (image + ofs) = val
#    define MINIRV32_STORE1(ofs, val) *(uint8_t *) (image + ofs) = val
#    define MINIRV32_LOAD4(ofs)       *(uint32_t *) (image + ofs)
#    define MINIRV32_LOAD2(ofs)       *(uint16_t *) (image + ofs)
#    define MINIRV32_LOAD1(ofs)       *(uint8_t *) (image + ofs)
#endif

#define MINIRV32_RAM_IMAGE_OFFSET 0x80000000

Generator::Generator(FILE *fp, const std::string &content) : fp(fp), content(content) {
}

Generator::~Generator() {
}

void Generator::generate() {
    auto image = content.data();
    auto image_size = content.size();

    bool hasError = false;
    auto error = [&](uint32_t pc) {
        if (!hasError) {
            std::cerr << "Unexpected instruction at pc " << std::hex << pc << std::endl;
            hasError = true;
        }
    };

    uint32_t pc = MINIRV32_RAM_IMAGE_OFFSET;

    // Function name
    fprintf(fp, "#include \"rv32core.h\"\n");
    fprintf(fp, "#include \"rv32macros.h\"\n");
    fprintf(fp, "\n\n");

    fprintf(fp, "static inline bool is_syscon(uint32_t addy) {\n"
                "    return addy == 2433744896;\n"
                "}\n\n\n");

    fprintf(fp, "int run(RV32Core &core) {\n\n");

    // Jump table
    fprintf(fp, "#define CREATE_JUMP_TABLE(PC) \\\n");
    fprintf(fp, "switch (PC) {\\\n");
    for (int i = 0; i < image_size; i += 4) {
        uint32_t tmp = MINIRV32_RAM_IMAGE_OFFSET + i;
        fprintf(fp,
                "    case 0x%x:\\\n"
                "        goto lab_%s;\\\n"
                "        break;\\\n",
                tmp, dec2hex(tmp).data());
    }
    fprintf(fp, "    default:\\\n"
                "        break;\\\n"
                "}\n\n");

    while (true) {
        std::string if_jump;
        uint32_t jal_pc = 0;
        bool jalr = false;

        uint32_t ofs_pc = pc - MINIRV32_RAM_IMAGE_OFFSET;

        if (ofs_pc >= content.size()) {
            break;
        }

        uint32_t ir = MINIRV32_LOAD4(ofs_pc);

        // Add block start
        fprintf(fp, "lab_%s: {\n", dec2hex(pc).data());
        fprintf(fp, "    // IR: %s\n", dec2hex(ir, 8).data());

        uint32_t rdid = (ir >> 7) & 0x1f;

        // uint32_t rval = 0;

        // Add define rval
        fprintf(fp, "uint32_t rval = 0;\n");

        switch (ir & 0x7f) {
            case 0b0110111: // LUI
                fprintf(fp, "// LUI\n");

                // rval = (ir & 0xfffff000);

                // Add rval assign
                fprintf(fp, "rval = 0x%x;\n", (ir & 0xfffff000));
                break;
            case 0b0010111: // AUIPC
                fprintf(fp, "// AUIPC\n");


                // rval = pc + (ir & 0xfffff000);

                // Add rval assign
                fprintf(fp, "rval = 0x%x;\n", pc + (ir & 0xfffff000));
                break;
            case 0b1101111: // JAL
            {
                fprintf(fp, "// JAL\n");

                int32_t reladdy = ((ir & 0x80000000) >> 11) | ((ir & 0x7fe00000) >> 20) | ((ir & 0x00100000) >> 9) |
                                  ((ir & 0x000ff000));
                if (reladdy & 0x00100000)
                    reladdy |= 0xffe00000; // Sign extension.
                // rval = pc + 4;

                // Add rval assign
                fprintf(fp, "rval = 0x%x;\n", pc + 4);

                // pc = pc + reladdy - 4;
                jal_pc = pc + reladdy - 4;
                break;
            }
            case 0b1100111: // JALR
            {
                fprintf(fp, "// JALR\n");

                uint32_t imm = ir >> 20;
                int32_t imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);

                // rval = pc + 4;

                // Add rval assign
                fprintf(fp, "rval = 0x%x;\n", pc + 4);

                // pc = ((REG((ir >> 15) & 0x1f) + imm_se) & ~1) - 4;
                // No need to minus 4
                fprintf(fp, "uint32_t pc = ((core.regs[%d] + (uint32_t) 0x%x) & ~1);\n", (ir >> 15) & 0x1f, imm_se);
                jalr = true;

                break;
            }
            case 0b1100011: // Branch
            {
                fprintf(fp, "// Branch\n");

                uint32_t immm4 =
                    ((ir & 0xf00) >> 7) | ((ir & 0x7e000000) >> 20) | ((ir & 0x80) << 4) | ((ir >> 31) << 12);
                if (immm4 & 0x1000)
                    immm4 |= 0xffffe000;

                // int32_t rs1 = REG((ir >> 15) & 0x1f);
                // int32_t rs2 = REG((ir >> 20) & 0x1f);

                // Add read reg
                fprintf(fp, "int32_t rs1 = core.regs[%d];\n", (ir >> 15) & 0x1f);
                fprintf(fp, "int32_t rs2 = core.regs[%d];\n", (ir >> 20) & 0x1f);

                immm4 = pc + immm4 - 4;
                rdid = 0;
                switch ((ir >> 12) & 0x7) {
                    // BEQ, BNE, BLT, BGE, BLTU, BGEU
                    case 0b000:
                        // if (rs1 == rs2)
                        //     pc = immm4;

                        // Add jump
                        if_jump = "if (rs1 == rs2) ";
                        jal_pc = immm4;
                        break;

                    case 0b001:
                        // if (rs1 != rs2)
                        //     pc = immm4;

                        // Add jump
                        if_jump = "if (rs1 != rs2) ";
                        jal_pc = immm4;
                        break;

                    case 0b100:
                        // if (rs1 < rs2)
                        //     pc = immm4;

                        // Add jump
                        if_jump = "if (rs1 < rs2) ";
                        jal_pc = immm4;
                        break;

                    case 0b101:
                        // if (rs1 >= rs2)
                        //     pc = immm4;

                        // Add jump
                        if_jump = "if (rs1 >= rs2) ";
                        jal_pc = immm4;
                        break; // BGE

                    case 0b110:
                        // if ((uint32_t) rs1 < (uint32_t) rs2)
                        //     pc = immm4;

                        // Add jump
                        if_jump = "if ((uint32_t) rs1 < (uint32_t) rs2)) ";
                        jal_pc = immm4;
                        break; // BLTU

                    case 0b111:
                        // if ((uint32_t) rs1 >= (uint32_t) rs2)
                        //     pc = immm4;

                        // Add jump
                        if_jump = "if ((uint32_t) rs1 >= (uint32_t) rs2)) ";
                        jal_pc = immm4;
                        break; // BGEU

                    default:
                        // trap = (2 + 1);
                        error(pc);
                        break;
                }
                break;
            }
            case 0b0000011: // Load
            {
                fprintf(fp, "// Load\n");

                uint32_t imm = ir >> 20;
                int32_t imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);

                // uint32_t rs1 = REG((ir >> 15) & 0x1f);
                // uint32_t rsval = rs1 + imm_se;

                // Add read reg
                fprintf(fp, "uint32_t rs1 = core.regs[%d];\n", (ir >> 15) & 0x1f);
                fprintf(fp, "uint32_t rsval = rs1 + (int32_t) %d;\n", imm_se);

                // rsval -= MINIRV32_RAM_IMAGE_OFFSET;
                fprintf(fp, "rsval -= MINIRV32_RAM_IMAGE_OFFSET;\n");

                // if (rsval >= MINI_RV32_RAM_SIZE - 3) {
                // Ignore

                // rsval += MINIRV32_RAM_IMAGE_OFFSET;
                // if (rsval >= 0x10000000 && rsval < 0x12000000) // UART, CLNT
                // {
                //     if (rsval == 0x1100bffc) // https://chromitem-soc.readthedocs.io/en/latest/clint.html
                //         rval = CSR(timerh);
                //     else if (rsval == 0x1100bff8)
                //         rval = CSR(timerl);
                //     else
                //         handleMemLoadControl(rsval, rval);
                // } else {
                //     trap = (5 + 1);
                //     rval = rsval;
                // }
                // } else {

                switch ((ir >> 12) & 0x7) {
                    // LB, LH, LW, LBU, LHU
                    case 0b000:
                        // rval = (int8_t) MINIRV32_LOAD1(rsval);

                        // Add rval assign
                        fprintf(fp, "rval = (int8_t) MINIRV32_LOAD1(rsval);\n");
                        break;
                    case 0b001:
                        // rval = (int16_t) MINIRV32_LOAD2(rsval);

                        // Add rval assign
                        fprintf(fp, "rval = (int16_t) MINIRV32_LOAD2(rsval);\n");
                        break;
                    case 0b010:
                        // rval = MINIRV32_LOAD4(rsval);

                        // Add rval assign
                        fprintf(fp, "rval = MINIRV32_LOAD4(rsval);\n");
                        break;
                    case 0b100:
                        // rval = MINIRV32_LOAD1(rsval);

                        // Add rval assign
                        fprintf(fp, "rval = MINIRV32_LOAD1(rsval);\n");
                        break;
                    case 0b101:
                        // rval = MINIRV32_LOAD2(rsval);

                        // Add rval assign
                        fprintf(fp, "rval = MINIRV32_LOAD2(rsval);\n");
                        break;
                    default:
                        // trap = (2 + 1);
                        error(pc);
                        break;
                }

                // }
                break;
            }
            case 0b0100011: // Store
            {
                fprintf(fp, "// Store\n");

                // uint32_t rs1 = REG((ir >> 15) & 0x1f);
                // uint32_t rs2 = REG((ir >> 20) & 0x1f);

                // Add read reg
                fprintf(fp, "uint32_t rs1 = core.regs[%d];\n", (ir >> 15) & 0x1f);
                fprintf(fp, "uint32_t rs2 = core.regs[%d];\n", (ir >> 20) & 0x1f);

                uint32_t addy = ((ir >> 7) & 0x1f) | ((ir & 0xfe000000) >> 20);
                if (addy & 0x800)
                    addy |= 0xfffff000;

                // addy += rs1 - MINIRV32_RAM_IMAGE_OFFSET;
                fprintf(fp, "uint32_t addy = (uint32_t) 0x%x + rs1 - MINIRV32_RAM_IMAGE_OFFSET;\n", addy);

                rdid = 0;

                // if (addy >= MINI_RV32_RAM_SIZE - 3) {
                //     addy += MINIRV32_RAM_IMAGE_OFFSET;
                //     if (addy >= 0x10000000 && addy < 0x12000000) {
                //         // Should be stuff like SYSCON, 8250, CLNT
                //         if (addy == 0x11004004)      // CLNT
                //             CSR(timermatchh) = rs2;
                //         else if (addy == 0x11004000) // CLNT
                //             CSR(timermatchl) = rs2;
                //         else if (addy == 0x11100000) // SYSCON (reboot, poweroff, etc.)
                //         {
                //             SETCSR(pc, pc + 4);
                //             return rs2; // NOTE: PC will be PC of Syscon.
                //         } else
                //             handleMemStoreControl(addy, rs2);
                //     } else {
                //         trap = (7 + 1); // Store access fault.
                //         rval = addy;
                //     }
                // } else {

                fprintf(fp, "if(is_syscon(addy)) //SYSCON (reboot, poweroff, etc.)\n"
                            "{\n"
                            "    return rs2; // NOTE: PC will be PC of Syscon.\n"
                            "}\n");

                switch ((ir >> 12) & 0x7) {
                    // SB, SH, SW
                    case 0b000:
                        // MINIRV32_STORE1(addy, rs2);
                        fprintf(fp, "MINIRV32_STORE1(addy, rs2);\n");
                        break;
                    case 0b001:
                        // MINIRV32_STORE2(addy, rs2);
                        fprintf(fp, "MINIRV32_STORE2(addy, rs2);\n");
                        break;
                    case 0b010:
                        // MINIRV32_STORE4(addy, rs2);
                        fprintf(fp, "MINIRV32_STORE4(addy, rs2);\n");
                        break;
                    default:
                        // trap = (2 + 1);
                        error(pc);
                        break;
                }
                // }
                break;
            }
            case 0b0010011: // Op-immediate
            case 0b0110011: // Op
            {
                fprintf(fp, "// ALU\n");

                uint32_t imm = ir >> 20;
                imm = imm | ((imm & 0x800) ? 0xfffff000 : 0);
                uint32_t is_reg = !!(ir & 0b100000);

                // uint32_t rs1 = REG((ir >> 15) & 0x1f);
                // uint32_t rs2 = is_reg ? REG(imm & 0x1f) : imm;

                // Add read reg
                fprintf(fp, "uint32_t rs1 = core.regs[%d];\n", (ir >> 15) & 0x1f);
                if (is_reg) {
                    fprintf(fp, "uint32_t rs2 = core.regs[%d];\n", imm & 0x1f);
                } else {
                    fprintf(fp, "uint32_t rs2 = 0x%x;\n", imm);
                }

                if (is_reg && (ir & 0x02000000)) {
                    switch ((ir >> 12) & 7) // 0x02000000 = RV32M
                    {
                        case 0b000:
                            // rval = rs1 * rs2;

                            // Add rval assign
                            fprintf(fp, "rval = rs1 * rs2;\n");
                            break; // MUL
                        case 0b001:
                            // rval = ((int64_t) ((int32_t) rs1) * (int64_t) ((int32_t) rs2)) >> 32;

                            // Add rval assign
                            fprintf(fp, "rval = ((int64_t) ((int32_t) rs1) * (int64_t) ((int32_t) rs2)) >> 32;\n");
                            break; // MULH
                        case 0b010:
                            // rval = ((int64_t) ((int32_t) rs1) * (uint64_t) rs2) >> 32;

                            // Add rval assign
                            fprintf(fp, "rval = ((int64_t) ((int32_t) rs1) * (uint64_t) rs2) >> 32;\n");
                            break; // MULHSU
                        case 0b011:
                            // rval = ((uint64_t) rs1 * (uint64_t) rs2) >> 32;

                            // Add rval assign
                            fprintf(fp, "rval = ((uint64_t) rs1 * (uint64_t) rs2) >> 32;\n");
                            break; // MULHU
                        case 0b100:
                            // if (rs2 == 0)
                            //     rval = -1;
                            // else
                            //     rval = ((int32_t) rs1 == INT32_MIN && (int32_t) rs2 == -1)
                            //                ? rs1
                            //                : ((int32_t) rs1 / (int32_t) rs2);

                            // Add rval assign
                            fprintf(fp, "if (rs2 == 0)\n"
                                        "   rval = -1;\n"
                                        "else\n"
                                        "    rval = ((int32_t) rs1 == INT32_MIN && (int32_t) rs2 == -1)\n"
                                        "               ? rs1\n"
                                        "               : ((int32_t) rs1 / (int32_t) rs2);\n");
                            break; // DIV
                        case 0b101:
                            // if (rs2 == 0)
                            //     rval = 0xffffffff;
                            // else
                            //     rval = rs1 / rs2;

                            // Add rval assign
                            fprintf(fp, "if (rs2 == 0)\n"
                                        "    rval = 0xffffffff;\n"
                                        "else\n"
                                        "    rval = rs1 / rs2;\n");
                            break; // DIVU
                        case 0b110:
                            // if (rs2 == 0)
                            //     rval = rs1;
                            // else
                            //     rval = ((int32_t) rs1 == INT32_MIN && (int32_t) rs2 == -1)
                            //                ? 0
                            //                : ((uint32_t) ((int32_t) rs1 % (int32_t) rs2));

                            // Add rval assign
                            fprintf(fp, "if (rs2 == 0)\n"
                                        "    rval = rs1;\n"
                                        "else\n"
                                        "    rval = ((int32_t) rs1 == INT32_MIN && (int32_t) rs2 == -1)\n"
                                        "               ? 0\n"
                                        "               : ((uint32_t) ((int32_t) rs1 %% (int32_t) rs2));\n");


                            break; // REM
                        case 0b111:
                            // if (rs2 == 0)
                            //     rval = rs1;
                            // else
                            //     rval = rs1 % rs2;

                            // Add rval assign
                            fprintf(fp, "if (rs2 == 0)\n"
                                        "    rval = rs1;\n"
                                        "else\n"
                                        "    rval = rs1 %% rs2;\n");
                            break; // REMU
                    }
                } else {
                    switch ((ir >> 12) & 7) // These could be either op-immediate or op commands.  Be careful.
                    {
                        case 0b000:
                            // rval = (is_reg && (ir & 0x40000000)) ? (rs1 - rs2) : (rs1 + rs2);
                            if (is_reg && (ir & 0x40000000))
                                fprintf(fp, "rval = rs1 - rs2;\n");
                            else
                                fprintf(fp, "rval = rs1 + rs2;\n");
                            break;
                        case 0b001:
                            // rval = rs1 << (rs2 & 0x1F);
                            fprintf(fp, "rval = rs1 << (rs2 & 0x1F);\n");
                            break;
                        case 0b010:
                            // rval = (int32_t) rs1 < (int32_t) rs2;
                            fprintf(fp, "rval = (int32_t) rs1 < (int32_t) rs2;\n");
                            break;
                        case 0b011:
                            // rval = rs1 < rs2;
                            fprintf(fp, "rval = rs1 < rs2;\n");
                            break;
                        case 0b100:
                            // rval = rs1 ^ rs2;
                            fprintf(fp, "rval = rs1 ^ rs2;\n");
                            break;
                        case 0b101:
                            // rval = (ir & 0x40000000) ? (((int32_t) rs1) >> (rs2 & 0x1F)) : (rs1 >> (rs2 & 0x1F));
                            if (ir & 0x40000000)
                                fprintf(fp, "rval = (((int32_t) rs1) >> (rs2 & 0x1F));\n");
                            else
                                fprintf(fp, "rval = (rs1 >> (rs2 & 0x1F));\n");
                            break;
                        case 0b110:
                            // rval = rs1 | rs2;
                            fprintf(fp, "rval = rs1 | rs2;\n");
                            break;
                        case 0b111:
                            // rval = rs1 & rs2;
                            fprintf(fp, "rval = rs1 & rs2;\n");
                            break;
                    }
                }
                break;
            }
            case 0b0001111:
                rdid = 0; // fencetype = (ir >> 12) & 0b111; We ignore fences in this impl.
                break;
            // case 0b1110011: // Zifencei+Zicsr
            // {
            //     uint32_t csrno = ir >> 20;
            //     int microop = (ir >> 12) & 0b111;
            //     if ((microop & 3)) // It's a Zicsr function.
            //     {
            //         int rs1imm = (ir >> 15) & 0x1f;
            //         uint32_t rs1 = REG(rs1imm);
            //         uint32_t writeval = rs1;

            //         // https://raw.githubusercontent.com/riscv/virtual-memory/main/specs/663-Svpbmt.pdf
            //         // Generally, support for Zicsr
            //         switch (csrno) {
            //             case 0x340:
            //                 rval = CSR(mscratch);
            //                 break;
            //             case 0x305:
            //                 rval = CSR(mtvec);
            //                 break;
            //             case 0x304:
            //                 rval = CSR(mie);
            //                 break;
            //             case 0xC00:
            //                 rval = CSR(cyclel);
            //                 break;
            //             case 0x344:
            //                 rval = CSR(mip);
            //                 break;
            //             case 0x341:
            //                 rval = CSR(mepc);
            //                 break;
            //             case 0x300:
            //                 rval = CSR(mstatus);
            //                 break; // mstatus
            //             case 0x342:
            //                 rval = CSR(mcause);
            //                 break;
            //             case 0x343:
            //                 rval = CSR(mtval);
            //                 break;
            //             case 0xf11:
            //                 rval = 0xff0ff0ff;
            //                 break; // mvendorid
            //             case 0x301:
            //                 rval = 0x40401101;
            //                 break; // misa (XLEN=32, IMA+X)
            //             // case 0x3B0: rval = 0; break; //pmpaddr0
            //             // case 0x3a0: rval = 0; break; //pmpcfg0
            //             // case 0xf12: rval = 0x00000000; break; //marchid
            //             // case 0xf13: rval = 0x00000000; break; //mimpid
            //             // case 0xf14: rval = 0x00000000; break; //mhartid
            //             default:
            //                 otherCSRRead(csrno, rval);
            //                 break;
            //         }

            //         switch (microop) {
            //             case 0b001:
            //                 writeval = rs1;
            //                 break; // CSRRW
            //             case 0b010:
            //                 writeval = rval | rs1;
            //                 break; // CSRRS
            //             case 0b011:
            //                 writeval = rval & ~rs1;
            //                 break; // CSRRC
            //             case 0b101:
            //                 writeval = rs1imm;
            //                 break; // CSRRWI
            //             case 0b110:
            //                 writeval = rval | rs1imm;
            //                 break; // CSRRSI
            //             case 0b111:
            //                 writeval = rval & ~rs1imm;
            //                 break; // CSRRCI
            //         }

            //         switch (csrno) {
            //             case 0x340:
            //                 SETCSR(mscratch, writeval);
            //                 break;
            //             case 0x305:
            //                 SETCSR(mtvec, writeval);
            //                 break;
            //             case 0x304:
            //                 SETCSR(mie, writeval);
            //                 break;
            //             case 0x344:
            //                 SETCSR(mip, writeval);
            //                 break;
            //             case 0x341:
            //                 SETCSR(mepc, writeval);
            //                 break;
            //             case 0x300:
            //                 SETCSR(mstatus, writeval);
            //                 break; // mstatus
            //             case 0x342:
            //                 SETCSR(mcause, writeval);
            //                 break;
            //             case 0x343:
            //                 SETCSR(mtval, writeval);
            //                 break;
            //             // case 0x3a0: break; //pmpcfg0
            //             // case 0x3B0: break; //pmpaddr0
            //             // case 0xf11: break; //mvendorid
            //             // case 0xf12: break; //marchid
            //             // case 0xf13: break; //mimpid
            //             // case 0xf14: break; //mhartid
            //             // case 0x301: break; //misa
            //             default:
            //                 otherCSRWrite(csrno, writeval);
            //                 break;
            //         }
            //     } else if (microop == 0b000) // "SYSTEM"
            //     {
            //         rdid = 0;
            //         if (csrno == 0x105)       // WFI (Wait for interrupts)
            //         {
            //             CSR(mstatus) |= 8;    // Enable interrupts
            //             CSR(extraflags) |= 4; // Infor environment we want to go to sleep.
            //             SETCSR(pc, pc + 4);
            //             return 1;
            //         } else if (((csrno & 0xff) == 0x02)) // MRET
            //         {
            //             // https://raw.githubusercontent.com/riscv/virtual-memory/main/specs/663-Svpbmt.pdf
            //             // Table 7.6. MRET then in mstatus/mstatush sets MPV=0, MPP=0, MIE=MPIE, and MPIE=1. La
            //             //  Should also update mstatus to reflect correct mode.
            //             uint32_t startmstatus = CSR(mstatus);
            //             uint32_t startextraflags = CSR(extraflags);
            //             SETCSR(mstatus, ((startmstatus & 0x80) >> 4) | ((startextraflags & 3) << 11) | 0x80);
            //             SETCSR(extraflags, (startextraflags & ~3) | ((startmstatus >> 11) & 3));
            //             pc = CSR(mepc) - 4;
            //         } else {
            //             switch (csrno) {
            //                 case 0:
            //                     trap = (CSR(extraflags) & 3) ? (11 + 1) : (8 + 1);
            //                     break; // ECALL; 8 = "Environment call from U-mode"; 11 = "Environment call from
            //                            // M-mode"
            //                 case 1:
            //                     trap = (3 + 1);
            //                     break; // EBREAK 3 = "Breakpoint"
            //                 default:
            //                     trap = (2 + 1);
            //                     break; // Illegal opcode.
            //             }
            //         }
            //     } else
            //         trap = (2 + 1); // Note micrrop 0b100 == undefined.
            //     break;
            // }
            // case 0b0101111: // RV32A
            // {
            //     uint32_t rs1 = REG((ir >> 15) & 0x1f);
            //     uint32_t rs2 = REG((ir >> 20) & 0x1f);
            //     uint32_t irmid = (ir >> 27) & 0x1f;

            //     rs1 -= MINIRV32_RAM_IMAGE_OFFSET;

            //     // We don't implement load/store from UART or CLNT with RV32A here.

            //     if (rs1 >= MINI_RV32_RAM_SIZE - 3) {
            //         trap = (7 + 1); // Store/AMO access fault
            //         rval = rs1 + MINIRV32_RAM_IMAGE_OFFSET;
            //     } else {
            //         rval = MINIRV32_LOAD4(rs1);

            //         // Referenced a little bit of
            //         // https://github.com/franzflasch/riscv_em/blob/master/src/core/core.c
            //         uint32_t dowrite = 1;
            //         switch (irmid) {
            //             case 0b00010:
            //                 dowrite = 0;
            //                 CSR(extraflags) |= 8;
            //                 break; // LR.W
            //             case 0b00011:
            //                 rval = !(CSR(extraflags) & 8);
            //                 break; // SC.W (Lie and always say it's good)
            //             case 0b00001:
            //                 break; // AMOSWAP.W
            //             case 0b00000:
            //                 rs2 += rval;
            //                 break; // AMOADD.W
            //             case 0b00100:
            //                 rs2 ^= rval;
            //                 break; // AMOXOR.W
            //             case 0b01100:
            //                 rs2 &= rval;
            //                 break; // AMOAND.W
            //             case 0b01000:
            //                 rs2 |= rval;
            //                 break; // AMOOR.W
            //             case 0b10000:
            //                 rs2 = ((int32_t) rs2 < (int32_t) rval) ? rs2 : rval;
            //                 break; // AMOMIN.W
            //             case 0b10100:
            //                 rs2 = ((int32_t) rs2 > (int32_t) rval) ? rs2 : rval;
            //                 break; // AMOMAX.W
            //             case 0b11000:
            //                 rs2 = (rs2 < rval) ? rs2 : rval;
            //                 break; // AMOMINU.W
            //             case 0b11100:
            //                 rs2 = (rs2 > rval) ? rs2 : rval;
            //                 break; // AMOMAXU.W
            //             default:
            //                 trap = (2 + 1);
            //                 dowrite = 0;
            //                 break; // Not supported.
            //         }
            //         if (dowrite)
            //             MINIRV32_STORE4(rs1, rs2);
            //     }
            //     break;
            // }
            default:
                error(pc);
                break;
        }

        if (rdid) {
            // REGSET(rdid, rval); // Write back register.

            // Add write reg
            fprintf(fp, "core.regs[%d] = rval;\n", rdid);
        }

        if (jal_pc) {
            fprintf(fp, "%sgoto lab_%s;\n", if_jump.data(), dec2hex(jal_pc + 4).data());
        } else if (jalr) {
            fprintf(fp, "CREATE_JUMP_TABLE(pc)\n");
        }

        // Write block end
        fprintf(fp, "}\n\n");

        pc += 4;
    }

    // Write function end
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");
}