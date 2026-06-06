; ModuleID = 'test/vartest.bc'
source_filename = "test/vartest"

define i64 @inner(i64 %x) {
entry:
  br label %ifcond

ifcond:                                           ; preds = %entry
  %cmptmp = icmp slt i64 %x, 3
  %cmptmp2 = zext i1 %cmptmp to i64
  %ifcmptmp = icmp ne i64 %cmptmp2, 0
  br i1 %ifcmptmp, label %ifthen, label %ifcont

ifthen:                                           ; preds = %ifcond
  ret i64 %x

ifcont:                                           ; preds = %ifcond
  %addtmp = add i64 %x, 1
  ret i64 %addtmp
}

define i64 @outer(i64 %n) {
entry:
  %cmptmp = icmp slt i64 0, %n
  %cmptmp2 = zext i1 %cmptmp to i64
  %forcmptmp = icmp ne i64 %cmptmp2, 0
  br i1 %forcmptmp, label %forbody, label %forafterphi

forbody:                                          ; preds = %forcond, %entry
  %forbodyphitmp = phi i64 [ 0, %entry ], [ %forcondphitmp, %forcond ]
  %forbodyphitmp1 = phi i64 [ 0, %entry ], [ %addtmp5, %forcond ]
  %forbodyphitmp2 = phi i64 [ %n, %entry ], [ %forcondphitmp4, %forcond ]
  %0 = call i64 @inner(i64 %forbodyphitmp1)
  %addtmp = add i64 %forbodyphitmp, %0
  br label %forcond

forcond:                                          ; preds = %forbody
  %forcondphitmp = phi i64 [ %addtmp, %forbody ]
  %forcondphitmp3 = phi i64 [ %forbodyphitmp1, %forbody ]
  %forcondphitmp4 = phi i64 [ %forbodyphitmp2, %forbody ]
  %addtmp5 = add i64 %forcondphitmp3, 1
  %cmptmp6 = icmp slt i64 %addtmp5, %forcondphitmp4
  %cmptmp27 = zext i1 %cmptmp6 to i64
  %forcmptmp8 = icmp ne i64 %cmptmp27, 0
  br i1 %forcmptmp8, label %forbody, label %forafterphi

forafterphi:                                      ; preds = %forcond, %entry
  %forafterphitmp = phi i64 [ 0, %entry ], [ %forcondphitmp, %forcond ]
  %forafterphitmp9 = phi i64 [ 0, %entry ], [ %addtmp5, %forcond ]
  %forafterphitmp10 = phi i64 [ %n, %entry ], [ %forcondphitmp4, %forcond ]
  ret i64 %forafterphitmp
}

define i64 @main() {
entry:
  %0 = call i64 @outer(i64 6)
  ret i64 %0
}
