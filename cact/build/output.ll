; ModuleID = 'main'
source_filename = "main"

@RADIUS = constant float 5.500000e+00
@PI = constant float 0x400921FB60000000
@EPS = constant float 0x3EB0C6F7A0000000
@PI_HEX = constant float 0x400921FB60000000
@HEX2 = constant float 7.812500e-02
@FACT = constant float -3.300000e+04
@CONV1 = constant float 2.330000e+02
@CONV2 = constant float 4.095000e+03
@MAX = constant i32 1000000000
@TWO = constant i32 2
@THREE = constant i32 3

declare void @print_int(i32)

declare void @print_float(float)

declare void @print_char(i8)

declare i32 @get_int()

declare float @get_float()

declare i8 @get_char()

define float @float_abs(float %0) {
entry:
  %x = alloca float, align 4
  store float %0, float* %x, align 4
  %x_var = load float, float* %x, align 4
  %flt_tmp = fcmp olt float %x_var, 0.000000e+00
  %cmp_tmp = icmp ne i1 %flt_tmp, false
  br i1 %cmp_tmp, label %then, label %else

then:                                             ; preds = %entry
  %x_var1 = load float, float* %x, align 4
  %fnegtmp = fneg float %x_var1
  ret float %fnegtmp

else:                                             ; preds = %entry
  br label %merge

merge:                                            ; preds = %else
  %x_var2 = load float, float* %x, align 4
  ret float %x_var2
}

define i32 @float_eq(float %0, float %1) {
entry:
  %a = alloca float, align 4
  store float %0, float* %a, align 4
  %b = alloca float, align 4
  store float %1, float* %b, align 4
  %a_var = load float, float* %a, align 4
  %b_var = load float, float* %b, align 4
  %fsubtmp = fsub float %a_var, %b_var
  %calltmp = call float @float_abs(float %fsubtmp)
  %EPS_var = load float, float* @EPS, align 4
  %flt_tmp = fcmp olt float %calltmp, %EPS_var
  %cmp_tmp = icmp ne i1 %flt_tmp, false
  br i1 %cmp_tmp, label %then, label %else

then:                                             ; preds = %entry
  ret i32 1

else:                                             ; preds = %entry
  ret i32 0
}

define void @error() {
entry:
  call void @print_char(i8 101)
  call void @print_char(i8 114)
  call void @print_char(i8 114)
  call void @print_char(i8 111)
  call void @print_char(i8 114)
  call void @print_char(i8 10)
  ret void
}

define void @ok() {
entry:
  call void @print_char(i8 111)
  call void @print_char(i8 107)
  call void @print_char(i8 10)
  ret void
}

define void @assert(i32 %0) {
entry:
  %cond = alloca i32, align 4
  store i32 %0, i32* %cond, align 4
  %cond_var = load i32, i32* %cond, align 4
  %eq_tmp = icmp eq i32 %cond_var, 0
  %cmp_tmp = icmp ne i1 %eq_tmp, false
  br i1 %cmp_tmp, label %then, label %else

then:                                             ; preds = %entry
  call void @error()
  br label %merge

else:                                             ; preds = %entry
  call void @ok()
  br label %merge

merge:                                            ; preds = %else, %then
  ret void
}

define void @assert_not(i32 %0) {
entry:
  %cond = alloca i32, align 4
  store i32 %0, i32* %cond, align 4
  %cond_var = load i32, i32* %cond, align 4
  %cmp_tmp = icmp ne i32 %cond_var, 0
  br i1 %cmp_tmp, label %then, label %else

then:                                             ; preds = %entry
  call void @error()
  br label %merge

else:                                             ; preds = %entry
  call void @ok()
  br label %merge

merge:                                            ; preds = %else, %then
  ret void
}

define i32 @main() {
entry:
  %EVAL1 = alloca float, align 4
  store float 0.000000e+00, float* %EVAL1, align 4
  %EVAL1_elem_ptr = getelementptr inbounds float, float* %EVAL1, i32 0
  %PI_var = load float, float* @PI, align 4
  %RADIUS_var = load float, float* @RADIUS, align 4
  %fmultmp = fmul float %PI_var, %RADIUS_var
  %RADIUS_var1 = load float, float* @RADIUS, align 4
  %fmultmp2 = fmul float %fmultmp, %RADIUS_var1
  store float %fmultmp2, float* %EVAL1_elem_ptr, align 4
  %EVAL2 = alloca float, align 4
  store float 2.000000e+00, float* %EVAL2, align 4
  %EVAL2_elem_ptr = getelementptr inbounds float, float* %EVAL2, i32 0
  %EVAL2_var = load float, float* %EVAL2, align 4
  %PI_HEX_var = load float, float* @PI_HEX, align 4
  %fmultmp3 = fmul float %EVAL2_var, %PI_HEX_var
  %RADIUS_var4 = load float, float* @RADIUS, align 4
  %fmultmp5 = fmul float %fmultmp3, %RADIUS_var4
  store float %fmultmp5, float* %EVAL2_elem_ptr, align 4
  %EVAL3 = alloca float, align 4
  store float 2.000000e+00, float* %EVAL3, align 4
  %EVAL3_elem_ptr = getelementptr inbounds float, float* %EVAL3, i32 0
  %PI_var6 = load float, float* @PI, align 4
  %EVAL3_var = load float, float* %EVAL3, align 4
  %fmultmp7 = fmul float %PI_var6, %EVAL3_var
  %RADIUS_var8 = load float, float* @RADIUS, align 4
  %fmultmp9 = fmul float %fmultmp7, %RADIUS_var8
  store float %fmultmp9, float* %EVAL3_elem_ptr, align 4
  %FIVE = alloca i32, align 4
  store i32 0, i32* %FIVE, align 4
  %FIVE_elem_ptr = getelementptr inbounds i32, i32* %FIVE, i32 0
  %TWO_var = load i32, i32* @TWO, align 4
  %THREE_var = load i32, i32* @THREE, align 4
  %addtmp = add i32 %TWO_var, %THREE_var
  store i32 %addtmp, i32* %FIVE_elem_ptr, align 4
  %HEX2_var = load float, float* @HEX2, align 4
  %FACT_var = load float, float* @FACT, align 4
  %calltmp = call i32 @float_eq(float %HEX2_var, float %FACT_var)
  call void @assert_not(i32 %calltmp)
  ret i32 0
}
