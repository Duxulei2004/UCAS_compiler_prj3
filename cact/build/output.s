  .text
  .globl float_abs
  .type float_abs, @function
float_abs:
.bb_0:
  mv t1, sp
  addi sp, sp, -16
  sw ra, 0(sp)
  fsw fa0, 4(sp)
  li t0, 4
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 8(sp)
  la t0, .f_0
  flw ft0, 0(t0)
  flw ft1, 8(sp)
  flt.S t0, ft1, ft0
  fmv.w.x fs0, zero
  feq.S t0, ft0, fs0
  xori t0, t0, 0
  bne t0, zero, .bb_2
  j .bb_3
.bb_1:
  lw ra, 0(sp)
  addi sp, sp, 16
  ret
.bb_2:
  li t0, 4
  add t0, t0, sp
  flw ft0, 0(t0)
  fsgnjn.s ft0, ft0, ft0
  fmv.s fa0, ft0
  j .bb_1
  j .bb_3
.bb_3:
  li t0, 4
  add t0, t0, sp
  flw ft0, 0(t0)
  fmv.s fa0, ft0
  j .bb_1
  j .bb_1
  .globl float_eq
  .type float_eq, @function
float_eq:
.bb_4:
  mv t1, sp
  addi sp, sp, -16
  sw ra, 0(sp)
  fsw fa0, 4(sp)
  fsw fa1, 8(sp)
  li t0, 4
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 12(sp)
  li t0, 8
  add t0, t0, sp
  flw ft0, 0(t0)
  flw ft1, 12(sp)
  fsub.S ft0, ft1, ft0
  fsw ft0, 12(sp)
  mv t1, sp
  addi sp, sp, 0
  flw fa0, 12(t1)
  jal float_abs
  addi sp, sp, 0
  fmv.s ft0, fa0
  fsw ft0, 12(sp)
  la t0, EPS
  flw ft0, 0(t0)
  flw ft1, 12(sp)
  flt.S t0, ft1, ft0
  fmv.w.x fs0, zero
  feq.S t0, ft0, fs0
  xori t0, t0, 0
  bne t0, zero, .bb_6
  j .bb_8
.bb_5:
  lw ra, 0(sp)
  addi sp, sp, 16
  ret
.bb_6:
  li t0, 1
  mv a0, t0
  j .bb_5
  j .bb_7
.bb_7:
  j .bb_5
.bb_8:
  li t0, 0
  mv a0, t0
  j .bb_5
  j .bb_7
  .globl error
  .type error, @function
error:
.bb_9:
  mv t1, sp
  addi sp, sp, -8
  sw ra, 0(sp)
  li t0, 101
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 114
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 114
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 111
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 114
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 10
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  j .bb_10
.bb_10:
  lw ra, 0(sp)
  addi sp, sp, 8
  ret
  .globl ok
  .type ok, @function
ok:
.bb_11:
  mv t1, sp
  addi sp, sp, -8
  sw ra, 0(sp)
  li t0, 111
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 107
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  li t0, 10
  sw t0, 4(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 4(t1)
  jal print_char
  addi sp, sp, 0
  mv t0, zero
  j .bb_12
.bb_12:
  lw ra, 0(sp)
  addi sp, sp, 8
  ret
  .globl assert
  .type assert, @function
assert:
.bb_13:
  mv t1, sp
  addi sp, sp, -16
  sw ra, 0(sp)
  sw a0, 4(sp)
  li t0, 4
  add t0, t0, sp
  lw t0, 0(t0)
  sw t0, 8(sp)
  li t0, 0
  lw t1, 8(sp)
  beq t1, t0, .bb_15
  j .bb_16
.bb_14:
  lw ra, 0(sp)
  addi sp, sp, 16
  ret
.bb_15:
  li t0, 1
  j .bb_17
.bb_16:
  li t0, 0
  j .bb_17
.bb_17:
  bne t0, zero, .bb_18
  j .bb_20
.bb_18:
  mv t1, sp
  addi sp, sp, 0
  jal error
  addi sp, sp, 0
  mv t0, zero
  j .bb_19
.bb_19:
  j .bb_14
.bb_20:
  mv t1, sp
  addi sp, sp, 0
  jal ok
  addi sp, sp, 0
  mv t0, zero
  j .bb_19
  .globl assert_not
  .type assert_not, @function
assert_not:
.bb_21:
  mv t1, sp
  addi sp, sp, -8
  sw ra, 0(sp)
  sw a0, 4(sp)
  li t0, 4
  add t0, t0, sp
  lw t0, 0(t0)
  bne t0, zero, .bb_23
  j .bb_25
.bb_22:
  lw ra, 0(sp)
  addi sp, sp, 8
  ret
.bb_23:
  mv t1, sp
  addi sp, sp, 0
  jal error
  addi sp, sp, 0
  mv t0, zero
  j .bb_24
.bb_24:
  j .bb_22
.bb_25:
  mv t1, sp
  addi sp, sp, 0
  jal ok
  addi sp, sp, 0
  mv t0, zero
  j .bb_24
  .globl main
  .type main, @function
main:
.bb_26:
  mv t1, sp
  addi sp, sp, -32
  sw ra, 0(sp)
  la t0, .f_0
  flw ft0, 0(t0)
  li t1, 4
  add t1, t1, sp
  fsw ft0, 0(t1)
  li t0, 4
  add t0, t0, sp
  sw t0, 8(sp)
  la t0, PI
  flw ft0, 0(t0)
  fsw ft0, 12(sp)
  la t0, RADIUS
  flw ft0, 0(t0)
  flw ft1, 12(sp)
  fmul.S ft0, ft1, ft0
  fsw ft0, 12(sp)
  la t0, RADIUS
  flw ft0, 0(t0)
  flw ft1, 12(sp)
  fmul.S ft0, ft1, ft0
  lw t1, 8(sp)
  fsw ft0, 0(t1)
  la t0, .f_1
  flw ft0, 0(t0)
  li t1, 8
  add t1, t1, sp
  fsw ft0, 0(t1)
  li t0, 8
  add t0, t0, sp
  sw t0, 12(sp)
  li t0, 8
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 16(sp)
  la t0, PI_HEX
  flw ft0, 0(t0)
  flw ft1, 16(sp)
  fmul.S ft0, ft1, ft0
  fsw ft0, 16(sp)
  la t0, RADIUS
  flw ft0, 0(t0)
  flw ft1, 16(sp)
  fmul.S ft0, ft1, ft0
  lw t1, 12(sp)
  fsw ft0, 0(t1)
  la t0, .f_1
  flw ft0, 0(t0)
  li t1, 12
  add t1, t1, sp
  fsw ft0, 0(t1)
  li t0, 12
  add t0, t0, sp
  sw t0, 16(sp)
  la t0, PI
  flw ft0, 0(t0)
  fsw ft0, 20(sp)
  li t0, 12
  add t0, t0, sp
  flw ft0, 0(t0)
  flw ft1, 20(sp)
  fmul.S ft0, ft1, ft0
  fsw ft0, 20(sp)
  la t0, RADIUS
  flw ft0, 0(t0)
  flw ft1, 20(sp)
  fmul.S ft0, ft1, ft0
  lw t1, 16(sp)
  fsw ft0, 0(t1)
  li t0, 0
  li t1, 16
  add t1, t1, sp
  sw t0, 0(t1)
  li t0, 16
  add t0, t0, sp
  sw t0, 20(sp)
  la t0, TWO
  lw t0, 0(t0)
  sw t0, 24(sp)
  la t0, THREE
  lw t0, 0(t0)
  lw t1, 24(sp)
  add t0, t1, t0
  lw t1, 20(sp)
  sw t0, 0(t1)
  la t0, HEX2
  flw ft0, 0(t0)
  fsw ft0, 20(sp)
  la t0, FACT
  flw ft0, 0(t0)
  fsw ft0, 24(sp)
  mv t1, sp
  addi sp, sp, 0
  flw fa0, 20(t1)
  flw fa1, 24(t1)
  jal float_eq
  addi sp, sp, 0
  mv t0, a0
  sw t0, 20(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 20(t1)
  jal assert_not
  addi sp, sp, 0
  mv t0, zero
  li t0, 4
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 20(sp)
  li t0, 8
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 24(sp)
  mv t1, sp
  addi sp, sp, 0
  flw fa0, 20(t1)
  flw fa1, 24(t1)
  jal float_eq
  addi sp, sp, 0
  mv t0, a0
  sw t0, 20(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 20(t1)
  jal assert_not
  addi sp, sp, 0
  mv t0, zero
  li t0, 8
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 20(sp)
  li t0, 12
  add t0, t0, sp
  flw ft0, 0(t0)
  fsw ft0, 24(sp)
  mv t1, sp
  addi sp, sp, 0
  flw fa0, 20(t1)
  flw fa1, 24(t1)
  jal float_eq
  addi sp, sp, 0
  mv t0, a0
  sw t0, 20(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 20(t1)
  jal assert
  addi sp, sp, 0
  mv t0, zero
  la t0, CONV1
  flw ft0, 0(t0)
  fsw ft0, 20(sp)
  la t0, CONV2
  flw ft0, 0(t0)
  fsw ft0, 24(sp)
  mv t1, sp
  addi sp, sp, 0
  flw fa0, 20(t1)
  flw fa1, 24(t1)
  jal float_eq
  addi sp, sp, 0
  mv t0, a0
  sw t0, 20(sp)
  mv t1, sp
  addi sp, sp, 0
  lw a0, 20(t1)
  jal assert_not
  addi sp, sp, 0
  mv t0, zero
  li t0, 0
  mv a0, t0
  j .bb_27
  j .bb_27
.bb_27:
  lw ra, 0(sp)
  addi sp, sp, 32
  ret


  .data
  .type RADIUS, @object
  .size RADIUS, 4
RADIUS:
  .float 5.5
  .type PI, @object
  .size PI, 4
PI:
  .float 3.1415927
  .type EPS, @object
  .size EPS, 4
EPS:
  .float 0.000001
  .type PI_HEX, @object
  .size PI_HEX, 4
PI_HEX:
  .float 3.1415927
  .type HEX2, @object
  .size HEX2, 4
HEX2:
  .float 0.078125
  .type FACT, @object
  .size FACT, 4
FACT:
  .float -33000
  .type CONV1, @object
  .size CONV1, 4
CONV1:
  .float 233
  .type CONV2, @object
  .size CONV2, 4
CONV2:
  .float 4095
  .type MAX, @object
  .size MAX, 4
MAX:
  .word 1000000000
  .type TWO, @object
  .size TWO, 4
TWO:
  .word 2
  .type THREE, @object
  .size THREE, 4
THREE:
  .word 3
  .type .f_0, @object
  .size .f_0, 4
.f_0:
  .float 0
  .type .f_1, @object
  .size .f_1, 4
.f_1:
  .float 2
