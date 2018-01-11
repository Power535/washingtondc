/*******************************************************************************
 *
 *
 *    WashingtonDC Dreamcast Emulator
 *    Copyright (C) 2018 snickerbockers
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

#include "jit/jit_il.h"
#include "jit/code_block.h"

#include "sh4.h"
#include "sh4_read_inst.h"
#include "sh4_disas.h"

static void sh4_disas_delay_slot(struct jit_code_block *block, unsigned pc) {
    inst_t inst = sh4_do_read_inst(pc);
    struct InstOpcode const *inst_op = sh4_decode_inst(inst);
    if (inst_op->pc_relative) {
        error_set_feature("illegal slot exceptions in the jit");
        error_set_address(pc);
        RAISE_ERROR(ERROR_UNIMPLEMENTED);
    }

    if (!inst_op->disas(block, pc, inst_op, inst)) {
        /*
         * in theory, this will never happen because only branch instructions
         * can return true, and those all should have been filtered out by the
         * pc_relative check above.
         */
        printf("inst is 0x%04x\n", (unsigned)inst);
        RAISE_ERROR(ERROR_INTEGRITY);
    }
    block->cycle_count += sh4_count_inst_cycles(inst_op,
                                                &block->last_inst_type);
}

bool sh4_disas_inst(struct jit_code_block *block, unsigned pc) {
    inst_t inst = sh4_do_read_inst(pc);
    struct InstOpcode const *inst_op = sh4_decode_inst(inst);
    block->cycle_count += sh4_count_inst_cycles(inst_op,
                                                &block->last_inst_type);
    return inst_op->disas(block, pc, inst_op, inst);
}

bool sh4_disas_fallback(struct jit_code_block *block, unsigned pc,
                        struct InstOpcode const *op, inst_t inst) {
    struct jit_inst il_inst;

    il_inst.op = JIT_OP_FALLBACK;
    il_inst.immed.fallback.fallback_fn = op->func;
    il_inst.immed.fallback.inst.inst = inst;
        
    jit_code_block_push_inst(block, &il_inst);

    return true;
}

bool sh4_disas_rts(struct jit_code_block *block, unsigned pc,
                   struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;

    jit_prepare_jump(&jit_inst, SH4_REG_PR, 0);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_rte(struct jit_code_block *block, unsigned pc,
                   struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;

    jit_prepare_jump(&jit_inst, SH4_REG_SPC, 0);
    jit_code_block_push_inst(block, &jit_inst);

    jit_mov_reg(&jit_inst, SH4_REG_SSR, SH4_REG_SR);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_braf_rn(struct jit_code_block *block, unsigned pc,
                       struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    unsigned reg_no = (inst >> 8) & 0xf;
    unsigned jump_offs = pc + 4;

    jit_prepare_jump(&jit_inst, SH4_REG_R0 + reg_no, jump_offs);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bsrf_rn(struct jit_code_block *block, unsigned pc,
                       struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    unsigned reg_no = (inst >> 8) & 0xf;
    unsigned jump_offs = pc + 4;

    /*
     * TODO: this opcode has some unnecessary duplication of effort because it
     * adds 4 to the PC twice: once when it emits the prepare_jump instruction,
     * and again when it adds 4 to the PR after copying over the PC.
     */

    jit_prepare_jump(&jit_inst, SH4_REG_R0 + reg_no, jump_offs);
    jit_code_block_push_inst(block, &jit_inst);

    jit_mov_reg(&jit_inst, SH4_REG_PC, SH4_REG_PR);
    jit_code_block_push_inst(block, &jit_inst);

    jit_add_const_reg(&jit_inst, 4, SH4_REG_PR);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bf(struct jit_code_block *block, unsigned pc,
                  struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    int jump_offs = (int)((int8_t)(inst & 0x00ff)) * 2 + 4;

    jit_prepare_jump(&jit_inst, SH4_REG_PC, jump_offs);
    jit_code_block_push_inst(block, &jit_inst);

    jit_prepare_alt_jump(&jit_inst, pc + 2);
    jit_code_block_push_inst(block, &jit_inst);

    jit_set_cond_jump_based_on_t(&jit_inst, 0);
    jit_code_block_push_inst(block, &jit_inst);

    jit_jump_cond(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bt(struct jit_code_block *block, unsigned pc,
                  struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    int jump_offs = (int)((int8_t)(inst & 0x00ff)) * 2 + 4;

    jit_prepare_jump(&jit_inst, SH4_REG_PC, jump_offs);
    jit_code_block_push_inst(block, &jit_inst);

    jit_prepare_alt_jump(&jit_inst, pc + 2);
    jit_code_block_push_inst(block, &jit_inst);

    jit_set_cond_jump_based_on_t(&jit_inst, 1);
    jit_code_block_push_inst(block, &jit_inst);

    jit_jump_cond(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bfs(struct jit_code_block *block, unsigned pc,
                   struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    int jump_offs = (int)((int8_t)(inst & 0x00ff)) * 2 + 4;

    jit_prepare_jump(&jit_inst, SH4_REG_PC, jump_offs);
    jit_code_block_push_inst(block, &jit_inst);

    jit_prepare_alt_jump(&jit_inst, pc + 4);
    jit_code_block_push_inst(block, &jit_inst);

    jit_set_cond_jump_based_on_t(&jit_inst, 0);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump_cond(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bts(struct jit_code_block *block, unsigned pc,
                   struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    int jump_offs = (int)((int8_t)(inst & 0x00ff)) * 2 + 4;

    jit_prepare_jump(&jit_inst, SH4_REG_PC, jump_offs);
    jit_code_block_push_inst(block, &jit_inst);

    jit_prepare_alt_jump(&jit_inst, pc + 4);
    jit_code_block_push_inst(block, &jit_inst);

    jit_set_cond_jump_based_on_t(&jit_inst, 1);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump_cond(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bra(struct jit_code_block *block, unsigned pc,
                   struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    int32_t disp = inst & 0x0fff;
    if (disp & 0x0800)
        disp |= 0xfffff000;
    disp = disp * 2 + 4;

    jit_prepare_jump(&jit_inst, SH4_REG_PC, disp);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_bsr(struct jit_code_block *block, unsigned pc,
                   struct InstOpcode const *op, inst_t inst) {
    struct jit_inst jit_inst;
    int32_t disp = inst & 0x0fff;
    if (disp & 0x0800)
        disp |= 0xfffff000;
    disp = disp * 2 + 4;

    /*
     * TODO: this opcode has some unnecessary duplication of effort because it
     * adds 4 to the PC twice: once when it emits the prepare_jump instruction,
     * and again when it adds 4 to the PR after copying over the PC.
     */

    jit_prepare_jump(&jit_inst, SH4_REG_PC, disp);
    jit_code_block_push_inst(block, &jit_inst);

    jit_mov_reg(&jit_inst, SH4_REG_PC, SH4_REG_PR);
    jit_code_block_push_inst(block, &jit_inst);

    jit_add_const_reg(&jit_inst, 4, SH4_REG_PR);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_jmp_arn(struct jit_code_block *block, unsigned pc,
                       struct InstOpcode const *op, inst_t inst) {
    unsigned reg_no = (inst >> 8) & 0xf;
    struct jit_inst jit_inst;

    jit_prepare_jump(&jit_inst, SH4_REG_R0 + reg_no, 0);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}

bool sh4_disas_jsr_arn(struct jit_code_block *block, unsigned pc,
                       struct InstOpcode const *op, inst_t inst) {
    unsigned reg_no = (inst >> 8) & 0xf;
    struct jit_inst jit_inst;

    jit_prepare_jump(&jit_inst, SH4_REG_R0 + reg_no, 0);
    jit_code_block_push_inst(block, &jit_inst);

    jit_mov_reg(&jit_inst, SH4_REG_PC, SH4_REG_PR);
    jit_code_block_push_inst(block, &jit_inst);

    jit_add_const_reg(&jit_inst, 4, SH4_REG_PR);
    jit_code_block_push_inst(block, &jit_inst);

    sh4_disas_delay_slot(block, pc + 2);

    jit_jump(&jit_inst);
    jit_code_block_push_inst(block, &jit_inst);

    return false;
}