; ModuleID = 'test/functest.bc'
source_filename = "test/functest"

define i32 @func(i32 %a) {
entry:
  br label %ifcond

ifcond:                                           ; preds = %entry
  %cmptmp = icmp sgt i32 %a, 0
  %cmptmp2 = zext i1 %cmptmp to i32
  %ifcmptmp = icmp ne i32 %cmptmp2, 0
  br i1 %ifcmptmp, label %ifthen, label %ifelse

ifthen:                                           ; preds = %ifcond
  br label %ifcond1

ifelse:                                           ; preds = %ifcond
  call void @putchar(i32 50)
  br label %ifcont

ifcont:                                           ; preds = %ifcont8, %ifelse
  %ifelsephitmp9 = phi i32 [ %ifelsephitmp, %ifcont8 ], [ 2, %ifelse ]
  br label %return

ifcond1:                                          ; preds = %ifthen
  %cmptmp3 = icmp eq i32 %a, 4
  %cmptmp24 = zext i1 %cmptmp3 to i32
  %ifcmptmp5 = icmp ne i32 %cmptmp24, 0
  br i1 %ifcmptmp5, label %ifthen6, label %ifelse7

ifthen6:                                          ; preds = %ifcond1
  call void @putchar(i32 48)
  br label %ifcont8

ifelse7:                                          ; preds = %ifcond1
  call void @putchar(i32 49)
  br label %ifcont8

ifcont8:                                          ; preds = %ifelse7, %ifthen6
  %ifelsephitmp = phi i32 [ 0, %ifthen6 ], [ 1, %ifelse7 ]
  br label %ifcont

return:                                           ; preds = %ifcont
  ret i32 %ifelsephitmp9
}

declare void @putchar(i32)

define i32 @main() {
entry:
  br label %return

return:                                           ; preds = %entry
  %0 = call i32 @func(i32 4)
  ret i32 %0
}
