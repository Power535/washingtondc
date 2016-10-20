/*******************************************************************************
 *
 *
 *    WashingtonDC Dreamcast Emulator
 *    Copyright (C) 2016 snickerbockers
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

#include <cstring>

#include "Icache.hpp"
#include "Ocache.hpp"
#include "BaseException.hpp"

#include "sh4.hpp"

Sh4::Sh4(Memory *mem) {
    this->mem = mem;

    memset(utlb, 0, sizeof(utlb));

    this->inst_cache = new Icache(this, mem);
    this->op_cache = new Ocache(this, mem);
}

Sh4::~Sh4() {
    delete op_cache;
    delete inst_cache;
}

int Sh4::write_mem(void const *out, addr32_t addr, size_t len) {
    // TODO: finish
    if (mmu.mmucr & MMUCR_AT_MASK) {
    } else {
    }

    return 1;
}

int Sh4::read_mem(void *out, addr32_t addr, size_t len) {
    return 1;
}

// find entry with matching VPN
struct Sh4::utlb_entry *Sh4::utlb_search(addr32_t vaddr) {
    struct Sh4::utlb_entry *ret = NULL;

    for (unsigned i = 0; i < UTLB_SIZE; i++) {
        struct utlb_entry *ent = utlb + i;
        addr32_t vpn_ent;
        addr32_t vpn_vaddr;

        switch ((ent->ent & UTLB_ENT_SZ_MASK) >> UTLB_ENT_SZ_SHIFT) {
        case ONE_KILO:
            // upper 22 bits
            vpn_vaddr = vaddr & 0xfffffc00;
            vpn_ent = ((ent->key & UTLB_KEY_VPN_MASK) << 8) & 0xfffffc00;
            break;
        case FOUR_KILO:
            // upper 20 bits
            vpn_vaddr = vaddr & 0xfffff000;
            vpn_ent = ((ent->key & UTLB_KEY_VPN_MASK) << 8) & 0xfffff000;
            break;
        case SIXTYFOUR_KILO:
            // upper 16 bits
            vpn_vaddr = vaddr & 0xffff0000;
            vpn_ent = ((ent->key & UTLB_KEY_VPN_MASK) << 8) & 0xffff0000;
            break;
        case ONE_MEGA:
            // upper 12 bits
            vpn_vaddr = vaddr & 0xfff00000;
            vpn_ent = ((ent->key & UTLB_KEY_VPN_MASK) << 8) & 0xfff00000;
            break;
        default:
            throw IntegrityError("Unrecognized UTLB size value");
        }

        if (!(UTLB_ENT_SH_MASK & ent->ent) &&
            (!(mmu.mmucr & MMUCR_SV_MASK) || !(reg.sr & SR_MD_MASK))) {
            // (not sharing pages) and (single-VM space or user-mode mode)

            unsigned utlb_asid = (ent->key & UTLB_KEY_ASID_MASK) >>
                UTLB_KEY_ASID_SHIFT;
            unsigned mmu_asid = (mmu.pteh & MMUPTEH_ASID_MASK) >>
                MMUPTEH_ASID_SHIFT;
            if (vpn_vaddr == vpn_ent && (ent->key & UTLB_KEY_VALID_MASK) &&
                utlb_asid == mmu_asid) {
                // UTLB hit
                if (ret)
                    throw UnimplementedError("Data TLB multiple hit exception");
                else
                    ret = ent;
            }
        } else {
            if (vpn_vaddr == vpn_ent && (ent->key & UTLB_KEY_VALID_MASK)) {
                // UTLB hit
                if (ret)
                    throw UnimplementedError("Data TLB multiple hit exception");
                else
                    ret = ent;
            }
        }
    }

    if (!ret)
        throw UnimplementedError("Data TLB miss exception");
    return ret;
}

struct Sh4::itlb_entry *Sh4::itlb_search(addr32_t vaddr) {
    struct Sh4::itlb_entry *ret = NULL;

    for (unsigned i = 0; i < ITLB_SIZE; i++) {
        struct itlb_entry *ent = itlb + i;
        addr32_t vpn_ent;
        addr32_t vpn_vaddr;

        switch ((ent->ent & ITLB_ENT_SZ_MASK) >> ITLB_ENT_SZ_SHIFT) {
        case ONE_KILO:
            // upper 22 bits
            vpn_vaddr = vaddr & 0xfffffc00;
            vpn_ent = ((ent->key & ITLB_KEY_VPN_MASK) << 8) & 0xfffffc00;
            break;
        case FOUR_KILO:
            // upper 20 bits
            vpn_vaddr = vaddr & 0xfffff000;
            vpn_ent = ((ent->key & ITLB_KEY_VPN_MASK) << 8) & 0xfffff000;
            break;
        case SIXTYFOUR_KILO:
            // upper 16 bits
            vpn_vaddr = vaddr & 0xffff0000;
            vpn_ent = ((ent->key & ITLB_KEY_VPN_MASK) << 8) & 0xffff0000;
            break;
        case ONE_MEGA:
            // upper 12 bits
            vpn_vaddr = vaddr & 0xfff00000;
            vpn_ent = ((ent->key & ITLB_KEY_VPN_MASK) << 8) & 0xfff00000;
            break;
        default:
            throw IntegrityError("Unrecognized ITLB size value");
        }

        if (!(ITLB_ENT_SH_MASK & ent->ent) &&
            (!(mmu.mmucr & MMUCR_SV_MASK) || !(reg.sr & SR_MD_MASK))) {
            // (not sharing pages) and (single-VM space or user-mode mode)
            unsigned itlb_asid = (ent->key & ITLB_KEY_ASID_MASK) >>
                ITLB_KEY_ASID_SHIFT;
            unsigned mmu_asid = (mmu.pteh & MMUPTEH_ASID_MASK) >>
                MMUPTEH_ASID_SHIFT;
            if (vpn_vaddr == vpn_ent && (ent->key & ITLB_KEY_VALID_MASK) &&
                itlb_asid == mmu_asid) {
                // ITLB hit
                if (ret)
                    throw UnimplementedError("Data TLB multiple hit exception");
                else
                    ret = ent;
            }
        } else {
            if (vpn_vaddr == vpn_ent && (ent->key & ITLB_KEY_VALID_MASK)) {
                // ITLB hit
                if (ret)
                    throw UnimplementedError("Data TLB multiple hit exception");
                else
                    ret = ent;
            }
        }
    }

    if (ret)
        return ret;

    // ITLB miss - check the UTLB
    struct utlb_entry *utlb_ent;
    try {
        utlb_ent = utlb_search(vaddr);
    } catch (UnimplementedError err) {
        // TODO: When CPU exceptions are implemented, there will need to be an
        //       option for utlb_search to prevent it from creating a CPU
        //       exception on miss so that this function can do it itself.
        throw UnimplementedError("Instruction TLB miss exception");
    }

    // now replace one of the ITLB entries.  Ideally there would be some sort
    // of Least-Recently-Used algorithm here.
    unsigned which = vaddr & (4 - 1);

    // the key formats are exactly the same, so this is safe.
    itlb[which].key = utlb_ent->key;

    // Notice how the PR gets AND'd with 2.  That is because the ITLB version of
    // PR is only 1 bit, while the UTLB version of PR is two bits.  ITLB's PR
    // corresponds to the upper bit of UTLB's PR.
    itlb[which].ent = 0;
    itlb[which].ent |= ((utlb_ent->ent & UTLB_ENT_PPN_MASK) >>
                        UTLB_ENT_PPN_SHIFT) << ITLB_ENT_PPN_SHIFT;
    itlb[which].ent |= ((utlb_ent->ent & UTLB_ENT_SZ_MASK) >>
                        UTLB_ENT_SZ_SHIFT) << ITLB_ENT_SZ_SHIFT;
    itlb[which].ent |= ((utlb_ent->ent & UTLB_ENT_SH_MASK) >>
                        UTLB_ENT_SH_SHIFT) << ITLB_ENT_SH_SHIFT;
    itlb[which].ent |= ((utlb_ent->ent & UTLB_ENT_C_MASK) >>
                        UTLB_ENT_C_SHIFT) << ITLB_ENT_C_SHIFT;
    itlb[which].ent |= ((((utlb_ent->ent & UTLB_ENT_PR_MASK) >>
                         UTLB_ENT_PR_SHIFT) & 2) << ITLB_ENT_PR_SHIFT);
    itlb[which].ent |= ((utlb_ent->ent & UTLB_ENT_SA_MASK) >>
                        UTLB_ENT_SA_SHIFT) << ITLB_ENT_SA_SHIFT;
    itlb[which].ent |= ((utlb_ent->ent & UTLB_ENT_TC_MASK) >>
                        UTLB_ENT_TC_SHIFT) << ITLB_ENT_TC_SHIFT;

    /*
     * The SH7750 Hardware Manual says to loop back to the beginning (see the
     * flowchart on page 44), so I implement that by recursing back into this
     * function.  Some sort of infinite-recursion detection may be warranted
     * here just in case.
     */
    return itlb_search(vaddr);
}

enum Sh4::PhysMemArea Sh4::get_mem_area(addr32_t addr) {
    if (addr >= AREA_P0_FIRST && addr <= AREA_P0_LAST)
        return AREA_P0;
    if (addr >= AREA_P1_FIRST && addr <= AREA_P1_LAST)
        return AREA_P1;
    if (addr >= AREA_P2_FIRST && addr <= AREA_P2_LAST)
        return AREA_P2;
    if (addr >= AREA_P3_FIRST && addr <= AREA_P3_LAST)
        return AREA_P3;
    return AREA_P4;
}

void Sh4::set_exception(unsigned excp_code) {
    excp_reg.expevt = (excp_code << EXPEVT_CODE_SHIFT) | EXPEVT_CODE_MASK;

    enter_exception((ExceptionCode)excp_code);
}

void Sh4::set_interrupt(unsigned intp_code) {
    excp_reg.intevt = (intp_code << INTEVT_CODE_SHIFT) | INTEVT_CODE_MASK;

    enter_exception((ExceptionCode)intp_code);
}

const Sh4::ExcpMeta Sh4::excp_meta[Sh4::EXCP_COUNT] = {
    { .code = EXCP_POWER_ON_RESET, .prio_level = 1, .prio_order = 1 },
    { .code = EXCP_MANUAL_RESET, .prio_level = 1, .prio_order = 2 },
    { .code = EXCP_HUDI_RESET, .prio_level = 1, .prio_order = 1 },
    { .code = EXCP_INST_TLB_MULT_HIT, .prio_level = 1, .prio_order = 3 },
    { .code = EXCP_DATA_TLB_MULT_HIT, .prio_level = 1, .prio_order = 4 },
    { .code = EXCP_USER_BREAK_BEFORE, .prio_level = 2, .prio_order = 0, .offset = 0x100 },
    { .code = EXCP_INST_ADDR_ERR, .prio_level = 2, .prio_order = 1, .offset = 0x100 },
    { .code = EXCP_INST_TLB_MISS, .prio_level = 2, .prio_order = 2, .offset = 0x400 },
    { .code = EXCP_INST_TLB_PROT_VIOL, .prio_level = 2, .prio_order = 3, .offset = 0x100 },
    { .code = EXCP_GEN_ILLEGAL_INST, .prio_level = 2, .prio_order = 4, .offset = 0x100 },
    { .code = EXCP_SLOT_ILLEGAL_INST, .prio_level = 2, .prio_order = 4, .offset = 0x100 },
    { .code = EXCP_GEN_FPU_DISABLE, .prio_level = 2, .prio_order = 4, .offset = 0x100 },
    { .code = EXCP_SLOT_FPU_DISABLE, .prio_level = 2, .prio_order = 4, .offset = 0x100 },
    { .code = EXCP_DATA_ADDR_READ, .prio_level = 2, .prio_order = 5, .offset = 0x100 },
    { .code = EXCP_DATA_ADDR_WRITE, .prio_level = 2, .prio_order = 5, .offset = 0x100 },
    { .code = EXCP_DATA_TLB_READ_MISS, .prio_level = 2, .prio_order = 6, .offset = 0x400 },
    { .code = EXCP_DATA_TLB_WRITE_MISS, .prio_level = 2, .prio_order = 6, .offset = 0x400 },
    { .code = EXCP_DATA_TLB_READ_PROT_VIOL, .prio_level = 2, .prio_order = 7, .offset = 0x100 },
    { .code = EXCP_DATA_TLB_WRITE_PROT_VIOL, .prio_level = 2, .prio_order = 7, .offset = 0x100 },
    { .code = EXCP_FPU, .prio_level = 2, .prio_order = 8, .offset = 0x100 },
    { .code = EXCP_INITIAL_PAGE_WRITE, .prio_level = 2, .prio_order = 9, .offset = 0x100 },
    { .code = EXCP_UNCONDITIONAL_TRAP, .prio_level = 2, .prio_order = 4, .offset = 0x100 },
    { .code = EXCP_USER_BREAK_AFTER, .prio_level = 2, .prio_order = 10, .offset = 0x100 },
    { .code = EXCP_NMI, .prio_level = 3, .prio_order = 0, .offset = 0x600 }, // it is nto a mistake that there is no prio_order here
    { .code = EXCP_EXT_0, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_1, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_2, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_3, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_4, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_5, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_6, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_7, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_8, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_9, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_A, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_B, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_C, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_D, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_EXT_E, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_TMU0_TUNI0, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_TMU1_TUNI1, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_TMU2_TUNI2, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_TMU2_TICPI2, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_RTC_ATI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_RTC_PRI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_RTC_CUI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCI_ERI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCI_RXI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCI_TXI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCI_TEI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_WDT_ITI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_REF_RCMI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_REF_ROVI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_GPIO_GPIOI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_DMAC_DMTE0, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_DMAC_DMTE1, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_DMAC_DMTE2, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_DMAC_DMTE3, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_DMAC_DMAE, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCIF_ERI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCIF_RXI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCIF_BRI, .prio_level = 4, .prio_order = 2, .offset = 0x600 },
    { .code = EXCP_SCIF_TXI, .prio_level = 4, .prio_order = 2, .offset = 0x600 }
};

void Sh4::enter_exception(enum ExceptionCode vector) {
    struct ExcpMeta const *meta = NULL;

    for (unsigned idx = 0; idx < EXCP_COUNT; idx++) {
        if (excp_meta[idx].code == vector) {
            meta = excp_meta + idx;
            break;
        }
    }

    if (!meta)
        throw IntegrityError("Unknown CPU exception/interrupt type");

    reg.spc = reg.pc;
    reg.ssr = reg.sr;
    reg.sgr = reg.rgen[7];

    reg.sr |= SR_BL_MASK;
    reg.sr |= SR_MD_MASK;
    reg.sr |= SR_RB_MASK;
    reg.sr &= ~SR_FD_MASK;

    if (vector == EXCP_POWER_ON_RESET ||
        vector == EXCP_MANUAL_RESET ||
        vector == EXCP_HUDI_RESET ||
        vector == EXCP_INST_TLB_MULT_HIT ||
        vector == EXCP_INST_TLB_MULT_HIT) {
        reg.pc = 0xa0000000;
    } else if (vector == EXCP_USER_BREAK_BEFORE ||
               vector == EXCP_USER_BREAK_AFTER) {
        // TODO: check brcr.ubde and use DBR instead of VBR if it is set
        reg.pc = reg.vbr + meta->offset;
    } else {
        reg.pc = reg.vbr + meta->offset;
    }
}
