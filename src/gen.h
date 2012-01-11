#ifndef GEN_H
#define GEN_H

/*** functions implemented for compilation targets: ***/

void gen_nativeinit();
void* gen_nativefinish();
void gen_nativerun(void*a);

/* reg moves */

void gen_mov_reg_reg(int t, int s);
void gen_mov_reg_imm(int t, int32_t i);

/* load & store */

void gen_load_reg_reg(int t, int a);
void gen_load_reg_imm(int t, int32_t a);
void gen_store_reg_reg(int a, int s);
void gen_store_reg_imm(int a, int32_t s);
void gen_store_imm_reg(int32_t a, int s);
void gen_store_imm_imm(int32_t a, int32_t s);
void gen_mov_reg_ivar(int r, int v);
void gen_mov_ivar_reg(int v, int r);

/* arithmetic */

void gen_add_reg_reg_reg(int t, int s1, int s);
void gen_add_reg_reg_imm(int t, int s1, int32_t i);
void gen_sub_reg_reg_reg(int t, int s1, int s);
void gen_sub_reg_reg_imm(int t, int s1, int32_t i);
void gen_mul_reg_reg_reg(int t, int s1, int s);
void gen_mul_reg_reg_imm(int t, int s1, int32_t i);
void gen_div_reg_reg_reg(int t, int s1, int s);
void gen_div_reg_reg_imm(int t, int s1, int32_t i);
void gen_mod_reg_reg_reg(int t, int s1, int s);
void gen_mod_reg_reg_imm(int t, int s1, int32_t i);

void gen_and_reg_reg_reg(int t, int s1, int s);
void gen_and_reg_reg_imm(int t, int s1, int32_t i);
void gen_or_reg_reg_reg(int t, int s1, int s);
void gen_or_reg_reg_imm(int t, int s1, int32_t i);
void gen_xor_reg_reg_reg(int t, int s1, int s);
void gen_xor_reg_reg_imm(int t, int s1, int32_t i);
void gen_ror_reg_reg_reg(int t, int s1, int s);
void gen_ror_reg_reg_imm(int t, int s1, int32_t i);
void gen_shl_reg_reg_reg(int t, int s1, int s);
void gen_shl_reg_reg_imm(int t, int s1, int32_t i);
void gen_neg_reg_reg(int t, int s);

void gen_atan2_reg_reg_reg(int t, int s1, int s);
void gen_atan2_reg_reg_imm(int t, int s1, int32_t i);
void gen_sin_reg_reg(int t, int s);
void gen_sqrt_reg_reg(int t, int s);

void gen_isneg_reg_reg(int t, int s);
void gen_ispos_reg_reg(int t, int s);
void gen_iszero_reg_reg(int t, int s);

/* stack */

void gen_push_reg(int s);
void gen_push_imm(int32_t i);
void gen_pop_reg(int t);
void gen_pop_noreg();
void gen_dup_reg(int t);
void gen_pick_reg_reg(int t,int i);
void gen_pick_reg_imm(int t,int32_t i);
void gen_bury_regreg(int t,int s);
void gen_bury_reg_imm(int t,int32_t s);
void gen_bury_imm_reg(int32_t i,int s);
void gen_bury_imm_imm(int32_t i,int32_t s);

/* rstack */

void gen_rpush_reg_reg(int s);
void gen_rpush_reg_imm(int32_t i);
void gen_rpush_reg_lab(int l);
void gen_rpop_reg_reg(int t);

/* conditionals */

//void gen_cmp_reg_reg(int r0,int r1);
//void gen_cmp_reg_imm(int r,uint32_t i);
//void gen_test_reg(int r);
// blt_reg_reg_lab
// bgt_reg_reg_lab
// btestnz_reg_reg_lab
// btestnz_reg_
// ...

void gen_beq_reg_lab(int s,int l);
void gen_bne_reg_lab(int s,int l);
void gen_beq_reg_rstack(int s);
void gen_bne_reg_rstack(int s);
/* not used */
void gen_bne_lab(int l);
void gen_beq_lab(int l);
void gen_bmi_lab(int l);
void gen_bpl_lab(int l);
/**/

/* jumps */

void gen_jmp_lab(int l);
void gen_jsr_lab(int i);
void gen_jsr_reg(int r);
void gen_label(int l);
void gen_nativeret();

/* some more complex things */

void gen_nativeterminate();
void gen_nativeuserin();

#endif
