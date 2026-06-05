; ModuleID = 'test/vartest.bc'
source_filename = "test/vartest"

define i32 @thing(i32 %a, i32 %b, i32 %c) {
entry:
  %cmptmp = icmp sle i32 0, %a
  %cmptmp2 = zext i1 %cmptmp to i32
  %forcmptmp = icmp ne i32 %cmptmp2, 0
  br i1 %forcmptmp, label %forbody, label %forafterphi

forbody:                                          ; preds = %forcond, %entry
  %forbodyphitmp = phi i32 [ %a, %entry ], [ %forcondphitmp, %forcond ]
  %forbodyphitmp1 = phi i32 [ %b, %entry ], [ %forcondphitmp4, %forcond ]
  %forbodyphitmp2 = phi i32 [ %c, %entry ], [ %forcondphitmp5, %forcond ]
  %forbodyphitmp3 = phi i32 [ 0, %entry ], [ %addtmp, %forcond ]
  br label %forcond

forcond:                                          ; preds = %forbody
  %forcondphitmp = phi i32 [ %forbodyphitmp, %forbody ]
  %forcondphitmp4 = phi i32 [ %forbodyphitmp1, %forbody ]
  %forcondphitmp5 = phi i32 [ %forbodyphitmp2, %forbody ]
  %forcondphitmp6 = phi i32 [ %forbodyphitmp3, %forbody ]
  %addtmp = add i32 %forcondphitmp6, 1
  %cmptmp7 = icmp sle i32 %addtmp, %forcondphitmp
  %cmptmp28 = zext i1 %cmptmp7 to i32
  %forcmptmp9 = icmp ne i32 %cmptmp28, 0
  br i1 %forcmptmp9, label %forbody, label %forafterphi

forafterphi:                                      ; preds = %forcond, %entry
  %forafterphitmp = phi i32 [ %a, %entry ], [ %forcondphitmp, %forcond ]
  %forafterphitmp10 = phi i32 [ %b, %entry ], [ %forcondphitmp4, %forcond ]
  %forafterphitmp11 = phi i32 [ %c, %entry ], [ %forcondphitmp5, %forcond ]
  %forafterphitmp12 = phi i32 [ 0, %entry ], [ %addtmp, %forcond ]
  br label %ifcond

ifcond:                                           ; preds = %forafterphi
  %cmptmp13 = icmp sge i32 %forafterphitmp, %forafterphitmp10
  %cmptmp214 = zext i1 %cmptmp13 to i32
  %ifcmptmp = icmp ne i32 %cmptmp214, 0
  br i1 %ifcmptmp, label %ifthen, label %ifelse

ifthen:                                           ; preds = %ifcond
  %addtmp15 = add i32 %forafterphitmp12, %forafterphitmp
  %subtmp = sub i32 %addtmp15, %forafterphitmp10
  br label %ifcont

ifelse:                                           ; preds = %ifcond
  %addtmp16 = add i32 %forafterphitmp12, %forafterphitmp
  %addtmp17 = add i32 %addtmp16, %forafterphitmp10
  br label %ifcont

ifcont:                                           ; preds = %ifelse, %ifthen
  %ifelsephitmp = phi i32 [ %subtmp, %ifthen ], [ %addtmp17, %ifelse ]
  %multmp = mul i32 %ifelsephitmp, %forafterphitmp11
  ret i32 %multmp
}

define i32 @main() {
entry:
  %0 = call i32 @thing(i32 9, i32 3, i32 4)
  ret i32 %0
}
