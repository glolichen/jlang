; ModuleID = 'test/fibonacci.bc'
source_filename = "test/fibonacci"

define i32 @main() {
entry:
  br i1 true, label %forbody, label %forafterphi

forbody:                                          ; preds = %forcond, %entry
  %forbodyphitmp = phi i32 [ -1, %entry ], [ %addtmp4, %forcond ]
  %forbodyphitmp1 = phi i32 [ 48, %entry ], [ %getcharcasttmp, %forcond ]
  %forbodyphitmp2 = phi i32 [ 1000000000, %entry ], [ %divtmp, %forcond ]
  %forbodyphitmp3 = phi i32 [ 0, %entry ], [ %addtmp, %forcond ]
  %subtmp = sub i32 %forbodyphitmp1, 48
  %multmp = mul i32 %forbodyphitmp2, %subtmp
  %addtmp = add i32 %forbodyphitmp3, %multmp
  %addtmp4 = add i32 %forbodyphitmp, 1
  %getchartmp = call i8 @getchar()
  %getcharcasttmp = sext i8 %getchartmp to i32
  br label %forcond

forcond:                                          ; preds = %forbody
  %divtmp = sdiv i32 %forbodyphitmp2, 10
  %cmptmp = icmp ne i32 %getcharcasttmp, 10
  %cmptmp2 = zext i1 %cmptmp to i32
  %forcmptmp = icmp ne i32 %cmptmp2, 0
  br i1 %forcmptmp, label %forbody, label %forafterphi

forafterphi:                                      ; preds = %forcond, %entry
  %forafterphitmp = phi i32 [ -1, %entry ], [ %addtmp4, %forcond ]
  %forafterphitmp5 = phi i32 [ 48, %entry ], [ %getcharcasttmp, %forcond ]
  %forafterphitmp6 = phi i32 [ 1000000000, %entry ], [ %divtmp, %forcond ]
  %forafterphitmp7 = phi i32 [ 0, %entry ], [ %addtmp, %forcond ]
  %subtmp11 = sub i32 10, %forafterphitmp
  %subtmp12 = sub i32 %subtmp11, 1
  %cmptmp13 = icmp slt i32 0, %subtmp12
  %cmptmp214 = zext i1 %cmptmp13 to i32
  %forcmptmp15 = icmp ne i32 %cmptmp214, 0
  br i1 %forcmptmp15, label %forbody8, label %forafterphi10

forbody8:                                         ; preds = %forcond9, %forafterphi
  %forbodyphitmp16 = phi i32 [ %forafterphitmp, %forafterphi ], [ %forbodyphitmp16, %forcond9 ]
  %forbodyphitmp17 = phi i32 [ %forafterphitmp5, %forafterphi ], [ %forbodyphitmp17, %forcond9 ]
  %forbodyphitmp18 = phi i32 [ 0, %forafterphi ], [ %addtmp21, %forcond9 ]
  %forbodyphitmp19 = phi i32 [ %forafterphitmp7, %forafterphi ], [ %divtmp20, %forcond9 ]
  %divtmp20 = sdiv i32 %forbodyphitmp19, 10
  br label %forcond9

forcond9:                                         ; preds = %forbody8
  %addtmp21 = add i32 %forbodyphitmp18, 1
  %subtmp22 = sub i32 10, %forbodyphitmp16
  %subtmp23 = sub i32 %subtmp22, 1
  %cmptmp24 = icmp slt i32 %addtmp21, %subtmp23
  %cmptmp225 = zext i1 %cmptmp24 to i32
  %forcmptmp26 = icmp ne i32 %cmptmp225, 0
  br i1 %forcmptmp26, label %forbody8, label %forafterphi10

forafterphi10:                                    ; preds = %forcond9, %forafterphi
  %forafterphitmp27 = phi i32 [ %forafterphitmp, %forafterphi ], [ %forbodyphitmp16, %forcond9 ]
  %forafterphitmp28 = phi i32 [ %forafterphitmp5, %forafterphi ], [ %forbodyphitmp17, %forcond9 ]
  %forafterphitmp29 = phi i32 [ 0, %forafterphi ], [ %addtmp21, %forcond9 ]
  %forafterphitmp30 = phi i32 [ %forafterphitmp7, %forafterphi ], [ %divtmp20, %forcond9 ]
  br label %ifcond

ifcond:                                           ; preds = %forafterphi10
  %cmptmp31 = icmp sgt i32 %forafterphitmp30, 2
  %cmptmp232 = zext i1 %cmptmp31 to i32
  %ifcmptmp = icmp ne i32 %cmptmp232, 0
  br i1 %ifcmptmp, label %ifthen, label %ifelse

ifthen:                                           ; preds = %ifcond
  %subtmp36 = sub i32 %forafterphitmp30, 3
  %cmptmp37 = icmp slt i32 0, %subtmp36
  %cmptmp238 = zext i1 %cmptmp37 to i32
  %forcmptmp39 = icmp ne i32 %cmptmp238, 0
  br i1 %forcmptmp39, label %forbody33, label %forafterphi35

ifelse:                                           ; preds = %ifcond
  br label %ifcond62

ifcont:                                           ; preds = %ifcont67, %forafterphi35
  %ifelsephitmp = phi i32 [ %forafterphitmp61, %forafterphi35 ], [ %ifphitmp, %ifcont67 ]
  %ifelsephitmp68 = phi i32 [ %forafterphitmp56, %forafterphi35 ], [ %forafterphitmp27, %ifcont67 ]
  %ifelsephitmp69 = phi i32 [ %forafterphitmp58, %forafterphi35 ], [ %forafterphitmp28, %ifcont67 ]
  %ifelsephitmp70 = phi i32 [ %forafterphitmp59, %forafterphi35 ], [ %forafterphitmp29, %ifcont67 ]
  %ifelsephitmp71 = phi i32 [ %forafterphitmp60, %forafterphi35 ], [ %forafterphitmp30, %ifcont67 ]
  br label %ifcond72

forbody33:                                        ; preds = %forcond34, %ifthen
  %forbodyphitmp40 = phi i32 [ 1, %ifthen ], [ %forbodyphitmp40, %forcond34 ]
  %forbodyphitmp41 = phi i32 [ 1, %ifthen ], [ %forbodyphitmp43, %forcond34 ]
  %forbodyphitmp42 = phi i32 [ %forafterphitmp27, %ifthen ], [ %forbodyphitmp42, %forcond34 ]
  %forbodyphitmp43 = phi i32 [ 1, %ifthen ], [ %forbodyphitmp47, %forcond34 ]
  %forbodyphitmp44 = phi i32 [ %forafterphitmp28, %ifthen ], [ %forbodyphitmp44, %forcond34 ]
  %forbodyphitmp45 = phi i32 [ 0, %ifthen ], [ %addtmp49, %forcond34 ]
  %forbodyphitmp46 = phi i32 [ %forafterphitmp30, %ifthen ], [ %forbodyphitmp46, %forcond34 ]
  %forbodyphitmp47 = phi i32 [ 2, %ifthen ], [ %addtmp48, %forcond34 ]
  %addtmp48 = add i32 %forbodyphitmp43, %forbodyphitmp47
  br label %forcond34

forcond34:                                        ; preds = %forbody33
  %addtmp49 = add i32 %forbodyphitmp45, 1
  %subtmp50 = sub i32 %forbodyphitmp46, 3
  %cmptmp51 = icmp slt i32 %addtmp49, %subtmp50
  %cmptmp252 = zext i1 %cmptmp51 to i32
  %forcmptmp53 = icmp ne i32 %cmptmp252, 0
  br i1 %forcmptmp53, label %forbody33, label %forafterphi35

forafterphi35:                                    ; preds = %forcond34, %ifthen
  %forafterphitmp54 = phi i32 [ 1, %ifthen ], [ %forbodyphitmp40, %forcond34 ]
  %forafterphitmp55 = phi i32 [ 1, %ifthen ], [ %forbodyphitmp43, %forcond34 ]
  %forafterphitmp56 = phi i32 [ %forafterphitmp27, %ifthen ], [ %forbodyphitmp42, %forcond34 ]
  %forafterphitmp57 = phi i32 [ 1, %ifthen ], [ %forbodyphitmp47, %forcond34 ]
  %forafterphitmp58 = phi i32 [ %forafterphitmp28, %ifthen ], [ %forbodyphitmp44, %forcond34 ]
  %forafterphitmp59 = phi i32 [ 0, %ifthen ], [ %addtmp49, %forcond34 ]
  %forafterphitmp60 = phi i32 [ %forafterphitmp30, %ifthen ], [ %forbodyphitmp46, %forcond34 ]
  %forafterphitmp61 = phi i32 [ 2, %ifthen ], [ %addtmp48, %forcond34 ]
  br label %ifcont

ifcond62:                                         ; preds = %ifelse
  %cmptmp63 = icmp sle i32 %forafterphitmp30, 0
  %cmptmp264 = zext i1 %cmptmp63 to i32
  %ifcmptmp65 = icmp ne i32 %cmptmp264, 0
  br i1 %ifcmptmp65, label %ifthen66, label %ifcont67

ifthen66:                                         ; preds = %ifcond62
  br label %ifcont67

ifcont67:                                         ; preds = %ifthen66, %ifcond62
  %ifphitmp = phi i32 [ 0, %ifthen66 ], [ 1, %ifcond62 ]
  br label %ifcont

ifcond72:                                         ; preds = %ifcont
  %cmptmp73 = icmp eq i32 %ifelsephitmp, 0
  %cmptmp274 = zext i1 %cmptmp73 to i32
  %ifcmptmp75 = icmp ne i32 %cmptmp274, 0
  br i1 %ifcmptmp75, label %ifthen76, label %ifelse77

ifthen76:                                         ; preds = %ifcond72
  call void @putchar(i32 48)
  br label %ifcont78

ifelse77:                                         ; preds = %ifcond72
  %cmptmp82 = icmp sle i32 1, %ifelsephitmp
  %cmptmp283 = zext i1 %cmptmp82 to i32
  %forcmptmp84 = icmp ne i32 %cmptmp283, 0
  br i1 %forcmptmp84, label %forbody79, label %forafterphi81

ifcont78:                                         ; preds = %forafterphi106, %ifthen76
  %ifelsephitmp132 = phi i32 [ %ifelsephitmp, %ifthen76 ], [ %forafterphitmp124, %forafterphi106 ]
  %ifelsephitmp133 = phi i32 [ %ifelsephitmp68, %ifthen76 ], [ %forafterphitmp127, %forafterphi106 ]
  %ifelsephitmp134 = phi i32 [ %ifelsephitmp69, %ifthen76 ], [ %forafterphitmp128, %forafterphi106 ]
  %ifelsephitmp135 = phi i32 [ %ifelsephitmp70, %ifthen76 ], [ %forafterphitmp130, %forafterphi106 ]
  %ifelsephitmp136 = phi i32 [ %ifelsephitmp71, %ifthen76 ], [ %forafterphitmp131, %forafterphi106 ]
  call void @putchar(i32 10)
  ret i32 0

forbody79:                                        ; preds = %forcond80, %ifelse77
  %forbodyphitmp85 = phi i32 [ %ifelsephitmp, %ifelse77 ], [ %forbodyphitmp85, %forcond80 ]
  %forbodyphitmp86 = phi i32 [ 0, %ifelse77 ], [ %forbodyphitmp86, %forcond80 ]
  %forbodyphitmp87 = phi i32 [ %ifelsephitmp68, %ifelse77 ], [ %forbodyphitmp87, %forcond80 ]
  %forbodyphitmp88 = phi i32 [ %ifelsephitmp69, %ifelse77 ], [ %forbodyphitmp88, %forcond80 ]
  %forbodyphitmp89 = phi i32 [ 1, %ifelse77 ], [ %multmp92, %forcond80 ]
  %forbodyphitmp90 = phi i32 [ %ifelsephitmp70, %ifelse77 ], [ %forbodyphitmp90, %forcond80 ]
  %forbodyphitmp91 = phi i32 [ %ifelsephitmp71, %ifelse77 ], [ %forbodyphitmp91, %forcond80 ]
  br label %forcond80

forcond80:                                        ; preds = %forbody79
  %multmp92 = mul i32 %forbodyphitmp89, 10
  %cmptmp93 = icmp sle i32 %multmp92, %forbodyphitmp85
  %cmptmp294 = zext i1 %cmptmp93 to i32
  %forcmptmp95 = icmp ne i32 %cmptmp294, 0
  br i1 %forcmptmp95, label %forbody79, label %forafterphi81

forafterphi81:                                    ; preds = %forcond80, %ifelse77
  %forafterphitmp96 = phi i32 [ %ifelsephitmp, %ifelse77 ], [ %forbodyphitmp85, %forcond80 ]
  %forafterphitmp97 = phi i32 [ 0, %ifelse77 ], [ %forbodyphitmp86, %forcond80 ]
  %forafterphitmp98 = phi i32 [ %ifelsephitmp68, %ifelse77 ], [ %forbodyphitmp87, %forcond80 ]
  %forafterphitmp99 = phi i32 [ %ifelsephitmp69, %ifelse77 ], [ %forbodyphitmp88, %forcond80 ]
  %forafterphitmp100 = phi i32 [ 1, %ifelse77 ], [ %multmp92, %forcond80 ]
  %forafterphitmp101 = phi i32 [ %ifelsephitmp70, %ifelse77 ], [ %forbodyphitmp90, %forcond80 ]
  %forafterphitmp102 = phi i32 [ %ifelsephitmp71, %ifelse77 ], [ %forbodyphitmp91, %forcond80 ]
  %divtmp103 = sdiv i32 %forafterphitmp100, 10
  %cmptmp107 = icmp sge i32 %divtmp103, 1
  %cmptmp2108 = zext i1 %cmptmp107 to i32
  %forcmptmp109 = icmp ne i32 %cmptmp2108, 0
  br i1 %forcmptmp109, label %forbody104, label %forafterphi106

forbody104:                                       ; preds = %forcond105, %forafterphi81
  %forbodyphitmp110 = phi i32 [ %forafterphitmp96, %forafterphi81 ], [ %modtmp, %forcond105 ]
  %forbodyphitmp111 = phi i32 [ %forafterphitmp97, %forafterphi81 ], [ %forbodyphitmp111, %forcond105 ]
  %forbodyphitmp112 = phi i32 [ %divtmp103, %forafterphi81 ], [ %divtmp120, %forcond105 ]
  %forbodyphitmp113 = phi i32 [ %forafterphitmp98, %forafterphi81 ], [ %forbodyphitmp113, %forcond105 ]
  %forbodyphitmp114 = phi i32 [ %forafterphitmp99, %forafterphi81 ], [ %forbodyphitmp114, %forcond105 ]
  %forbodyphitmp115 = phi i32 [ %forafterphitmp100, %forafterphi81 ], [ %forbodyphitmp115, %forcond105 ]
  %forbodyphitmp116 = phi i32 [ %forafterphitmp101, %forafterphi81 ], [ %forbodyphitmp116, %forcond105 ]
  %forbodyphitmp117 = phi i32 [ %forafterphitmp102, %forafterphi81 ], [ %forbodyphitmp117, %forcond105 ]
  %divtmp118 = sdiv i32 %forbodyphitmp110, %forbodyphitmp112
  %addtmp119 = add i32 %divtmp118, 48
  call void @putchar(i32 %addtmp119)
  %modtmp = srem i32 %forbodyphitmp110, %forbodyphitmp112
  br label %forcond105

forcond105:                                       ; preds = %forbody104
  %divtmp120 = sdiv i32 %forbodyphitmp112, 10
  %cmptmp121 = icmp sge i32 %divtmp120, 1
  %cmptmp2122 = zext i1 %cmptmp121 to i32
  %forcmptmp123 = icmp ne i32 %cmptmp2122, 0
  br i1 %forcmptmp123, label %forbody104, label %forafterphi106

forafterphi106:                                   ; preds = %forcond105, %forafterphi81
  %forafterphitmp124 = phi i32 [ %forafterphitmp96, %forafterphi81 ], [ %modtmp, %forcond105 ]
  %forafterphitmp125 = phi i32 [ %forafterphitmp97, %forafterphi81 ], [ %forbodyphitmp111, %forcond105 ]
  %forafterphitmp126 = phi i32 [ %divtmp103, %forafterphi81 ], [ %divtmp120, %forcond105 ]
  %forafterphitmp127 = phi i32 [ %forafterphitmp98, %forafterphi81 ], [ %forbodyphitmp113, %forcond105 ]
  %forafterphitmp128 = phi i32 [ %forafterphitmp99, %forafterphi81 ], [ %forbodyphitmp114, %forcond105 ]
  %forafterphitmp129 = phi i32 [ %forafterphitmp100, %forafterphi81 ], [ %forbodyphitmp115, %forcond105 ]
  %forafterphitmp130 = phi i32 [ %forafterphitmp101, %forafterphi81 ], [ %forbodyphitmp116, %forcond105 ]
  %forafterphitmp131 = phi i32 [ %forafterphitmp102, %forafterphi81 ], [ %forbodyphitmp117, %forcond105 ]
  br label %ifcont78
}

declare i8 @getchar()

declare void @putchar(i32)
