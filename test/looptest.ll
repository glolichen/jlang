; ModuleID = 'test/looptest.bc'
source_filename = "test/looptest"

define i32 @main() {
entry:
  br i1 true, label %forbody, label %forafterphi

forbody:                                          ; preds = %forcond, %entry
  %forbodyphitmp = phi i32 [ 0, %entry ], [ %forbodyphitmp, %forcond ]
  %forbodyphitmp1 = phi i32 [ 1, %entry ], [ %addtmp, %forcond ]
  br label %ifcond

forcond:                                          ; preds = %ifcont
  %addtmp = add i32 %forbodyphitmp1, 1
  %cmptmp3 = icmp sle i32 %addtmp, 10
  %cmptmp24 = zext i1 %cmptmp3 to i32
  %forcmptmp = icmp ne i32 %cmptmp24, 0
  br i1 %forcmptmp, label %forbody, label %forafterphi

forafterphi:                                      ; preds = %ifthen, %forcond, %entry
  %forafterphitmp = phi i32 [ 0, %entry ], [ %forbodyphitmp, %forcond ]
  %forafterphitmp5 = phi i32 [ 1, %entry ], [ %addtmp, %forcond ]
  ret i32 %forafterphitmp

ifcond:                                           ; preds = %forbody
  %cmptmp = icmp eq i32 %forbodyphitmp1, 1
  %cmptmp2 = zext i1 %cmptmp to i32
  %ifcmptmp = icmp ne i32 %cmptmp2, 0
  br i1 %ifcmptmp, label %ifthen, label %ifcont

ifthen:                                           ; preds = %ifcond
  br label %forafterphi

ifcont:                                           ; preds = %ifcond
  br label %forcond
}
