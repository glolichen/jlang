; ModuleID = 'test/vartest.bc'
source_filename = "test/vartest"

define void @func(i64 %a) {
entry:
  br label %ifcond

ifcond:                                           ; preds = %entry
  %cmptmp = icmp eq i64 %a, 0
  %cmptmp2 = zext i1 %cmptmp to i64
  %ifcmptmp = icmp ne i64 %cmptmp2, 0
  br i1 %ifcmptmp, label %ifthen, label %ifcont

ifthen:                                           ; preds = %ifcond
  call void @putchar(i32 50)
  br label %return

ifcont:                                           ; preds = %ifcond
  call void @putchar(i32 49)
  ret void

return:                                           ; preds = %ifthen
  ret void
}

declare void @putchar(i32)

define i64 @main() {
entry:
  call void @func(i64 0)
  br label %return

return:                                           ; preds = %entry
  ret i64 0
}
