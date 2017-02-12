/*******************************************************************************
 *
 *
 *    WashingtonDC Dreamcast Emulator
 *    Copyright (C) 2017 snickerbockers
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ******************************************************************************/

#ifndef SH4_REG_HPP_
#define SH4_REG_HPP_

#include "BaseException.hpp"
#include "sh4_reg_flags.hpp"

typedef boost::error_info<struct tag_feature_name_error_info, std::string>
errinfo_regname;

typedef enum sh4_reg_idx {
    /* general-purpose registers 0-7, bank 0 */
    SH4_REG_R0_BANK0,
    SH4_REG_R1_BANK0,
    SH4_REG_R2_BANK0,
    SH4_REG_R3_BANK0,
    SH4_REG_R4_BANK0,
    SH4_REG_R5_BANK0,
    SH4_REG_R6_BANK0,
    SH4_REG_R7_BANK0,

    /* general-purpose registers 0-7, bank 1 */
    SH4_REG_R0_BANK1,
    SH4_REG_R1_BANK1,
    SH4_REG_R2_BANK1,
    SH4_REG_R3_BANK1,
    SH4_REG_R4_BANK1,
    SH4_REG_R5_BANK1,
    SH4_REG_R6_BANK1,
    SH4_REG_R7_BANK1,

    /* general-purpose registers 8-15 */
    SH4_REG_R8,
    SH4_REG_R9,
    SH4_REG_R10,
    SH4_REG_R11,
    SH4_REG_R12,
    SH4_REG_R13,
    SH4_REG_R14,
    SH4_REG_R15,

    /* status register */
    SH4_REG_SR,

    /* saved-status register */
    SH4_REG_SSR,

    /* saved program counter */
    SH4_REG_SPC,

    /* global base register */
    SH4_REG_GBR,

    /* vector base register */
    SH4_REG_VBR,

    /* saved general register 15 */
    SH4_REG_SGR,

    /* debug base register */
    SH4_REG_DBR,

    /* Multiply-and-accumulate register high */
    SH4_REG_MACH,

    /* multiply-and-accumulate register low */
    SH4_REG_MACL,

    /* procedure register */
    SH4_REG_PR,

    /* program counter */
    SH4_REG_PC,

    /* Page table entry high */
    SH4_REG_PTEH,

    /* Page table entry low */
    SH4_REG_PTEL,

    /* Page table entry assisstance */
    SH4_REG_PTEA,

    /* Translation table base */
    SH4_REG_TTB,

    /* TLB exception address */
    SH4_REG_TEA,

    /* MMU control */
    SH4_REG_MMUCR,

    /* Cache control register */
    SH4_REG_CCR,

    /* Queue address control register 0 */
    SH4_REG_QACR0,

    /* Queue address control register 1 */
    SH4_REG_QACR1,

    /* TRAPA immediate data     - 0xff000020 */
    SH4_REG_TRA,

    /* exception event register - 0xff000024 */
    SH4_REG_EXPEVT,

    /* interrupt event register - 0xff000028 */
    SH4_REG_INTEVT,

    /* Timer output control register */
    SH4_REG_TOCR,

    /* Timer start register */
    SH4_REG_TSTR,

    /* Timer channel 0 constant register */
    SH4_REG_TCOR0,

    /* Timer channel 0 counter */
    SH4_REG_TCNT0,

    /* Timer channel 0 control register */
    SH4_REG_TCR0,

    /* Timer channel 1 constant register */
    SH4_REG_TCOR1,

    /* Timer channel 1 counter */
    SH4_REG_TCNT1,

    /* Timer channel 1 control register */
    SH4_REG_TCR1,

    /* Timer channel 2 constant register */
    SH4_REG_TCOR2,

    /* Timer channel 2 counter */
    SH4_REG_TCNT2,

    /* Timer channel 2 control register */
    SH4_REG_TCR2,

    /* Timer channel 2 input capture register */
    SH4_REG_TCPR2,

    /* DMAC Source Address Register 1 */
    SH4_REG_SAR1,

    /* DMAC Destination Address Register 1 */
    SH4_REG_DAR1,

    /* DMAC transfer count register 1 */
    SH4_REG_DMATCR1,

    /* DMAC channel control register 1 */
    SH4_REG_CHCR1,

    /* DMAC Source Address Register 2 */
    SH4_REG_SAR2,

    /* DMAC Destination Address Register 2 */
    SH4_REG_DAR2,

    /* DMAC transfer count register 2 */
    SH4_REG_DMATCR2,

    /* DMAC channel control register 2 */
    SH4_REG_CHCR2,

    /* DMAC Source Address Register 3 */
    SH4_REG_SAR3,

    /* DMAC Destination Address Register 3 */
    SH4_REG_DAR3,

    /* DMAC transfer count register 3 */
    SH4_REG_DMATCR3,

    /* DMAC channel control register 3 */
    SH4_REG_CHCR3,

    /* DMAC Operation Register */
    SH4_REG_DMAOR,

    /* Interrupt Control Register */
    SH4_REG_ICR,

    /* Interrupt Priority Registers A-D */
    SH4_REG_IPRA,
    SH4_REG_IPRB,
    SH4_REG_IPRC,
    SH4_REG_IPRD,

    SH4_REGISTER_COUNT
} sh4_reg_idx_t;

/*
 * for the purpose of these handlers, you may assume that the caller has
 * already checked the permissions.
 */
struct Sh4MemMappedReg;
typedef int(*Sh4RegReadHandler)(Sh4 *sh4, void *buf,
                                struct Sh4MemMappedReg const *reg_info);
typedef int(*Sh4RegWriteHandler)(Sh4 *sh4, void const *buf,
                                 struct Sh4MemMappedReg const *reg_info);

/*
 * TODO: turn this into a radix tree of some sort.
 *
 * Alternatively, I could turn this into a simple lookup array; this
 * would incur a huge memory overhead (hundreds of MB), but it looks like
 * it would be feasible in the $CURRENT_YEAR and it would net a
 * beautiful O(1) mapping from addr32_t to MemMappedReg.
 */
struct Sh4MemMappedReg {
    char const *reg_name;

    /*
     * Some registers can be referenced over a range of addresses.
     * To check for equality between this register and a given physical
     * address, AND the address with addr_mask and then check for equality
     * with addr
     */
    addr32_t addr;  // addr shoud be the p4 addr, not the area7 addr
    addr32_t addr_mask;

    unsigned len;

    /* index of the register in the register file */
    sh4_reg_idx_t reg_idx;

    /*
     * if true, the value will be preserved during a manual ("soft") reset
     * and manual_reset_val will be ignored; else value will be set to
     * manual_reset_val during a manual reset.
     */
    bool hold_on_reset;

    Sh4RegReadHandler on_p4_read;
    Sh4RegWriteHandler on_p4_write;

    /*
     * if len < 4, then only the lower "len" bytes of
     * these values will be used.
     */
    reg32_t poweron_reset_val;
    reg32_t manual_reset_val;
};

/*
 * this is called from the sh4 constructor to
 * initialize all memory-mapped registers
 */
void sh4_init_regs(Sh4 *sh4);

// set up the memory-mapped registers for a reset;
void sh4_poweron_reset_regs(Sh4 *sh4);
void sh4_manual_reset_regs(Sh4 *sh4);

/* read/write handler callbacks for when you don't give a fuck */
int Sh4IgnoreRegReadHandler(Sh4 *sh4, void *buf,
                            struct Sh4MemMappedReg const *reg_info);
int Sh4IgnoreRegWriteHandler(Sh4 *sh4, void const *buf,
                             struct Sh4MemMappedReg const *reg_info);

/* default reg reg/write handler callbacks */
int Sh4DefaultRegReadHandler(Sh4 *sh4, void *buf,
                             struct Sh4MemMappedReg const *reg_info);
int Sh4DefaultRegWriteHandler(Sh4 *sh4, void const *buf,
                              struct Sh4MemMappedReg const *reg_info);

/*
 * read handle callback that always fails (although currently it throws an
 * UnimplementedError because I don't know what the proper response is when
 * the software tries to read from an unreadable register).
 *
 * This is used for certain registers which are write-only.
 */
int Sh4WriteOnlyRegReadHandler(Sh4 *sh4, void *buf,
                               struct Sh4MemMappedReg const *reg_info);

/*
 * likewise, this is a write handler for read-only registers.
 * It will also raise an exception whenever it is invokled.
 */
int Sh4ReadOnlyRegWriteHandler(Sh4 *sh4, void const *buf,
                               struct Sh4MemMappedReg const *reg_info);

/*
 * These handlers are functionally equivalent to the default read/write
 * handlers, except they log a warning to std::cerr every time they are called.
 */
int Sh4WarnRegReadHandler(Sh4 *sh4, void *buf,
                          struct Sh4MemMappedReg const *reg_info);
int Sh4WarnRegWriteHandler(Sh4 *sh4, void const *buf,
                           struct Sh4MemMappedReg const *reg_info);

/*
 * called for P4 area read/write ops that
 * fall in the memory-mapped register range
 */
int sh4_read_mem_mapped_reg(Sh4 *sh4, void *buf,
                            addr32_t addr, unsigned len);
int sh4_write_mem_mapped_reg(Sh4 *sh4, void const *buf,
                             addr32_t addr, unsigned len);

#endif
