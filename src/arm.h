#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>
#include <stdbool.h>
#include "mem.h"
#include "gba.h"

#define ROR(val, s) (((val) >> (s)) | ((val) << (32 - (s))))
#define SBIT(op)    ((op & (1 << 20)) ? true : false)

#define ARM_BYTE_SZ  1
#define ARM_HWORD_SZ 2
#define ARM_WORD_SZ  4
#define ARM_DWORD_SZ 8

// PSR flags
#define ARM_N (1 << 31)    // Negative
#define ARM_Z (1 << 30)    // Zero
#define ARM_C (1 << 29)    // Carry
#define ARM_V (1 << 28)    // Overflow
#define ARM_Q (1 << 27)    // Saturation
#define ARM_A (1 << 8)     // Abort off
#define ARM_I (1 << 7)     // IRQ off
#define ARM_F (1 << 6)     // FIQ off
#define ARM_T (1 << 5)     // Thumb

// Modes
#define ARM_USR 0b10000    // User
#define ARM_FIQ 0b10001    // Fast IRQ
#define ARM_IRQ 0b10010    // IRQ
#define ARM_SVC 0b10011    // Supervisor Call
#define ARM_MON 0b10110    // Monitor
#define ARM_ABT 0b10111    // Abort
#define ARM_UND 0b11011    // Undefined
#define ARM_SYS 0b11111    // System

// Interrupt addresses
#define ARM_VEC_RESET  0x00    // Reset
#define ARM_VEC_UND    0x04    // Undefined
#define ARM_VEC_SVC    0x08    // Supervisor Call
#define ARM_VEC_PABT   0x0c    // Prefetch Abort
#define ARM_VEC_DABT   0x10    // Data Abort
#define ARM_VEC_ADDR26 0x14    // Address exceeds 26 bits (legacy)
#define ARM_VEC_IRQ    0x18    // IRQ
#define ARM_VEC_FIQ    0x1c    // Fast IRQ


typedef enum
{
    AND,
    BIC,
    EOR,
    MVN,
    ORN,
    ORR,
    SHIFT
} arm_logic_e;

typedef enum
{
    BYTE  = 1,
    HWORD = 2,
    WORD  = 4,
    DWORD = 8
} arm_size_e;

typedef union
{
    int32_t w;

    struct
    {
        int16_t lo;
        int16_t hi;
    } h;

    struct
    {
        int8_t b0;
        int8_t b1;
        int8_t b2;
        int8_t b3;
    } b;
} arm_word;

typedef struct
{
    uint64_t lhs;
    uint64_t rhs;
    uint8_t  rd;
    bool     cout;
    bool     s;
} arm_data_t;

typedef struct
{
    uint32_t val;
    bool     cout;
} arm_shifter_t;

typedef struct
{
    uint64_t lhs;
    uint64_t rhs;
    uint8_t  ra;
    uint8_t  rd;
    bool     s;
} arm_mpy_t;

typedef struct
{
    uint8_t  rd;
    uint32_t psr;
    uint32_t mask;
    bool     r;
} arm_psr_t;

typedef struct
{
    uint16_t regs;
    uint8_t  rn;
    uint8_t  rt2;
    uint8_t  rt;
    uint32_t addr;
    int32_t  disp;
} arm_memio_t;

typedef struct
{
    arm_word lhs;
    arm_word rhs;
    uint8_t  rd;
} arm_parith_t;

typedef struct
{
    uint32_t r[16];

    uint32_t r8_usr;
    uint32_t r9_usr;
    uint32_t r10_usr;
    uint32_t r11_usr;
    uint32_t r12_usr;
    uint32_t r13_usr;
    uint32_t r14_usr;

    uint32_t r8_fiq;
    uint32_t r9_fiq;
    uint32_t r10_fiq;
    uint32_t r11_fiq;
    uint32_t r12_fiq;
    uint32_t r13_fiq;
    uint32_t r14_fiq;

    uint32_t r13_irq;
    uint32_t r14_irq;

    uint32_t r13_svc;
    uint32_t r14_svc;

    uint32_t r13_mon;
    uint32_t r14_mon;

    uint32_t r13_abt;
    uint32_t r14_abt;

    uint32_t r13_und;
    uint32_t r14_und;

    uint32_t cpsr;

    uint32_t spsr_fiq;
    uint32_t spsr_irq;
    uint32_t spsr_svc;
    uint32_t spsr_abt;
    uint32_t spsr_und;
    uint32_t spsr_mon;
} arm_regs_t;

class CPU {
    typedef void (CPU::*arm_proc_t)(void);
    arm_proc_t arm_proc[2][4096];
    arm_proc_t thumb_proc[2048];

  public:
    GBA *gba = nullptr;

    uint32_t arm_op;
    uint32_t arm_pipe[2];
    uint32_t arm_cycles;

    bool int_halt;
    bool pipe_reload;

    arm_regs_t arm_r;

  public:
    CPU(GBA *_gba);

    bool     arm_flag_tst(uint32_t flag);
    bool     arm_cond(int8_t cond);
    bool     arm_in_thumb();
    uint32_t arm_saturate(int64_t val, int32_t min, int32_t max, bool q);
    uint32_t arm_ssatq(int64_t val);
    uint16_t arm_fetchh(access_type_e at);
    uint32_t arm_fetch(access_type_e at);
    uint32_t arm_fetch_n();
    uint32_t arm_fetch_s();
    uint32_t arm_memio_reg_get(uint8_t reg);

    arm_data_t    arm_data_imm_op();
    arm_shifter_t arm_data_regi(uint8_t rm, uint8_t type, uint8_t imm);
    arm_data_t    arm_data_regi_op();
    arm_shifter_t arm_data_regr(uint8_t rm, uint8_t type, uint8_t rs);
    arm_data_t    arm_data_regr_op();

    arm_data_t t16_data_imm3_op();
    arm_data_t t16_data_imm8_op();
    arm_data_t t16_data_rdn3_op();
    arm_data_t t16_data_neg_op();
    arm_data_t t16_data_reg_op();
    arm_data_t t16_data_rdn4_op(bool s);
    arm_data_t t16_data_imm7sp_op();
    arm_data_t t16_data_imm8sp_op();
    arm_data_t t16_data_imm8pc_op();
    arm_data_t t16_data_imm5_op(bool rsh);

    arm_mpy_t   arm_mpy_op();
    arm_mpy_t   t16_mpy_op();
    arm_psr_t   arm_mrs_op();
    arm_psr_t   arm_msr_imm_op();
    arm_psr_t   arm_msr_reg_op();
    arm_memio_t arm_memio_mult_op();

    arm_memio_t  arm_memio_imm_op();
    arm_memio_t  arm_memio_reg_op();
    arm_memio_t  arm_memio_immt_op();
    arm_memio_t  arm_memio_regt_op();
    arm_memio_t  arm_memio_immdh_op();
    arm_memio_t  arm_memio_regdh_op();
    arm_memio_t  t16_memio_mult_op();
    arm_memio_t  t16_memio_imm5_op(int8_t step);
    arm_memio_t  t16_memio_imm8sp_op();
    arm_memio_t  t16_memio_imm8pc_op();
    arm_memio_t  t16_memio_reg_op();
    arm_parith_t arm_parith_op();

    void arm_flag_set(uint32_t flag, bool cond);
    void arm_bank_to_regs(int8_t mode);
    void arm_regs_to_bank(int8_t mode);
    void arm_mode_set(int8_t mode);
    void arm_spsr_get(uint32_t *psr);
    void arm_spsr_set(uint32_t spsr);
    void arm_spsr_to_cpsr();

    void arm_setn(uint32_t res);
    void arm_setn64(uint64_t res);
    void arm_setz(uint32_t res);
    void arm_setz64(uint64_t res);
    void arm_addc(uint64_t res);
    void arm_subc(uint64_t res);
    void arm_addv(uint32_t lhs, uint32_t rhs, uint32_t res);
    void arm_subv(uint32_t lhs, uint32_t rhs, uint32_t res);

    void arm_load_pipe();
    void arm_r15_align();
    void arm_interwork();
    void arm_cycles_s_to_n();

    void arm_arith_set(arm_data_t op, uint64_t res, bool add);
    void arm_arith_add(arm_data_t op, bool adc);
    void arm_arith_subtract(arm_data_t op, bool sbc, bool rev);

    void arm_arith_rsb(arm_data_t op);
    void arm_arith_rsc(arm_data_t op);
    void arm_arith_sbc(arm_data_t op);
    void arm_arith_sub(arm_data_t op);
    void arm_arith_cmn(arm_data_t op);
    void arm_arith_cmp(arm_data_t op);

    void arm_logic_set(arm_data_t op, uint32_t res);
    void arm_logic(arm_data_t op, arm_logic_e inst);

    void arm_asr(arm_data_t op);
    void arm_lsl(arm_data_t op);
    void arm_lsr(arm_data_t op);
    void arm_ror(arm_data_t op);

    void arm_count_zeros(uint8_t rd);
    void arm_logic_teq(arm_data_t op);
    void arm_logic_tst(arm_data_t op);

    void arm_mpy_inc_cycles(uint64_t rhs, bool u);
    void arm_mpy_add(arm_mpy_t op);
    void arm_mpy(arm_mpy_t op);
    void arm_mpy_smla__(arm_mpy_t op, bool m, bool n);
    void arm_mpy_smlal(arm_mpy_t op);
    void arm_mpy_smlal__(arm_mpy_t op, bool m, bool n);
    void arm_mpy_smlaw_(arm_mpy_t op, bool m);
    void arm_mpy_smul(arm_mpy_t op, bool m, bool n);
    void arm_mpy_smull(arm_mpy_t op);
    void arm_mpy_smulw_(arm_mpy_t op, bool m);
    void arm_mpy_umlal(arm_mpy_t op);
    void arm_mpy_umull(arm_mpy_t op);

    void arm_memio_ldm(arm_memio_t op);
    void arm_memio_ldm_usr(arm_memio_t op);
    void arm_memio_stm(arm_memio_t op);
    void arm_memio_stm_usr(arm_memio_t op);
    void arm_memio_ldr(arm_memio_t op);
    void arm_memio_ldrb(arm_memio_t op);
    void arm_memio_ldrh(arm_memio_t op);
    void arm_memio_ldrsb(arm_memio_t op);
    void arm_memio_ldrsh(arm_memio_t op);
    void arm_memio_ldrd(arm_memio_t op);
    void arm_memio_str(arm_memio_t op);
    void arm_memio_strb(arm_memio_t op);
    void arm_memio_strh(arm_memio_t op);
    void arm_memio_strd(arm_memio_t op);
    void arm_memio_load_usr(arm_memio_t op, arm_size_e size);
    void arm_memio_store_usr(arm_memio_t op, arm_size_e size);
    void arm_memio_ldrbt(arm_memio_t op);
    void arm_memio_strbt(arm_memio_t op);

    void arm_psr_to_reg(arm_psr_t op);
    void arm_reg_to_psr(arm_psr_t op);

    void arm_parith_qadd(arm_parith_t op);
    void arm_parith_qsub(arm_parith_t op);
    void arm_parith_qdadd(arm_parith_t op);
    void arm_parith_qdsub(arm_parith_t op);

    void arm_adc_imm();
    void arm_adc_regi();
    void arm_adc_regr();
    void t16_adc_rdn3();
    void arm_add_imm();
    void arm_add_regi();
    void arm_add_regr();
    void t16_add_imm3();
    void t16_add_imm8();
    void t16_add_reg();
    void t16_add_rdn4();
    void t16_add_sp7();
    void t16_add_sp8();
    void t16_adr();
    void arm_and_imm();
    void arm_and_regi();
    void arm_and_regr();
    void t16_and_rdn3();
    void arm_shift_imm();
    void arm_shift_reg();
    void t16_asr_imm5();
    void t16_asr_rdn3();
    void arm_b();
    void t16_b_imm11();
    void t16_b_imm8();
    void arm_bic_imm();
    void arm_bic_regi();
    void arm_bic_regr();
    void t16_bic_rdn3();
    void arm_bkpt();
    void t16_bkpt();
    void arm_bl();
    void arm_blx_imm();
    void arm_blx_reg();
    void t16_blx();
    void t16_blx_h2();
    void t16_blx_lrimm();
    void t16_blx_h1();
    void t16_blx_h3();
    void arm_bx();
    void t16_bx();
    void arm_cdp();
    void arm_cdp2();
    void arm_clz();
    void arm_cmn_imm();
    void arm_cmn_regi();
    void arm_cmn_regr();
    void t16_cmn_rdn3();
    void arm_cmp_imm();
    void arm_cmp_regi();
    void arm_cmp_regr();
    void t16_cmp_imm8();
    void t16_cmp_rdn3();
    void t16_cmp_rdn4();
    void arm_eor_imm();
    void arm_eor_regi();
    void arm_eor_regr();
    void t16_eor_rdn3();
    void arm_ldc();
    void arm_ldc2();
    void arm_ldm();
    void arm_ldm_usr();
    void t16_ldm();
    void arm_ldr_imm();
    void arm_ldr_reg();
    void t16_ldr_imm5();
    void t16_ldr_sp8();
    void t16_ldr_pc8();
    void t16_ldr_reg();
    void arm_ldrb_imm();
    void arm_ldrb_reg();
    void t16_ldrb_imm5();
    void t16_ldrb_reg();
    void arm_ldrbt_imm();
    void arm_ldrbt_reg();
    void arm_ldrd_imm();
    void arm_ldrd_reg();
    void arm_ldrh_imm();
    void arm_ldrh_reg();
    void t16_ldrh_imm5();
    void t16_ldrh_reg();
    void arm_ldrsb_imm();
    void arm_ldrsb_reg();
    void t16_ldrsb_reg();
    void arm_ldrsh_imm();
    void arm_ldrsh_reg();
    void t16_ldrsh_reg();
    void t16_lsl_imm5();
    void t16_lsl_rdn3();
    void t16_lsr_imm5();
    void t16_lsr_rdn3();
    void arm_mcr();
    void arm_mcr2();
    void arm_mcrr();
    void arm_mcrr2();
    void arm_mla();
    void arm_mov_imm12();
    void t16_mov_imm();
    void t16_mov_rd4();
    void t16_mov_rd3();
    void arm_mrc();
    void arm_mrc2();
    void arm_mrrc();
    void arm_mrrc2();
    void arm_mrs();
    void arm_msr_imm();
    void arm_msr_reg();
    void arm_mul();
    void t16_mul();
    void arm_mvn_imm();
    void arm_mvn_regi();
    void arm_mvn_regr();
    void t16_mvn_rdn3();
    void arm_orr_imm();
    void arm_orr_regi();
    void arm_orr_regr();
    void t16_orr_rdn3();
    void arm_pld_imm();
    void arm_pld_reg();
    void t16_pop();
    void t16_push();
    void arm_qadd();
    void arm_qdadd();
    void arm_qdsub();
    void arm_qsub();
    void t16_ror();
    void arm_rsb_imm();
    void arm_rsb_regi();
    void arm_rsb_regr();
    void t16_rsb_rdn3();
    void arm_rsc_imm();
    void arm_rsc_regi();
    void arm_rsc_regr();
    void arm_sbc_imm();
    void arm_sbc_regi();
    void arm_sbc_regr();
    void t16_sbc_rdn3();
    void arm_smla__();
    void arm_smlal();
    void arm_smlal__();
    void arm_smlaw_();
    void arm_smul();
    void arm_smull();
    void arm_smulw_();
    void arm_stc();
    void arm_stc2();
    void arm_stm();
    void arm_stm_usr();
    void t16_stm();
    void arm_str_imm();
    void arm_str_reg();
    void t16_str_imm5();
    void t16_str_sp8();
    void t16_str_reg();
    void arm_strb_imm();
    void arm_strb_reg();
    void t16_strb_imm5();
    void t16_strb_reg();
    void arm_strbt_imm();
    void arm_strbt_reg();
    void arm_strd_imm();
    void arm_strd_reg();
    void arm_strh_imm();
    void arm_strh_reg();
    void t16_strh_imm5();
    void t16_strh_reg();
    void arm_sub_imm();
    void arm_sub_regi();
    void arm_sub_regr();
    void t16_sub_imm3();
    void t16_sub_imm8();
    void t16_sub_reg();
    void t16_sub_sp7();
    void arm_svc();
    void t16_svc();
    void arm_swp();
    void arm_teq_imm();
    void arm_teq_regi();
    void arm_teq_regr();
    void arm_tst_imm();
    void arm_tst_regi();
    void arm_tst_regr();
    void t16_tst_rdn3();
    void arm_umlal();
    void arm_umull();
    void arm_und();

    void arm_proc_fill(bool arm);
    void arm_proc_set(bool arm, int idx, arm_proc_t proc, uint32_t op, uint32_t mask, int32_t bits);

    void arm_proc_init();
    void thumb_proc_init();
    void arm_init();
    void arm_uninit();
    void t16_inc_r15();
    void t16_step();
    void arm_inc_r15();
    void arm_step();
    void arm_exec(uint32_t target_cycles);
    void arm_int(uint32_t address, int8_t mode);
    void arm_check_irq();
    void arm_reset();
};

#endif
