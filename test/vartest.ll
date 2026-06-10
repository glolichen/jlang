; ModuleID = 'test/vartest.bc'
source_filename = "test/vartest"

define i64 @main() {
entry:
  br i1 true, label %forbody, label %forafterphi

forbody:                                          ; preds = %forcond, %entry
  %forbodyphi = phi i64 [ 1, %entry ], [ %add, %forcond ]
  %forbodyphi1 = phi i64 [ 0, %entry ], [ %forcondphi3, %forcond ]
  %forbodyphi2 = phi i64 [ 0, %entry ], [ %forcondphi4, %forcond ]
  br label %ifcond

forcond:                                          ; preds = %ifcont
  %forcondphi = phi i64 [ %forbodyphi, %ifcont ]
  %forcondphi3 = phi i64 [ %forbodyphi1, %ifcont ]
  %forcondphi4 = phi i64 [ %forbodyphi2, %ifcont ]
  %add = add i64 %forcondphi, 1
  %cmp5 = icmp sle i64 %add, 5
  %cmp26 = zext i1 %cmp5 to i64
  %forcmp = icmp ne i64 %cmp26, 0
  br i1 %forcmp, label %forbody, label %forafterphi

forafterphi:                                      ; preds = %forcond, %entry
  %forafterphi7 = phi i64 [ 1, %entry ], [ %add, %forcond ]
  %forafterphi8 = phi i64 [ 0, %entry ], [ %forcondphi3, %forcond ]
  %forafterphi9 = phi i64 [ 0, %entry ], [ %forcondphi4, %forcond ]
  br label %return

ifcond:                                           ; preds = %forbody
  %cmp = icmp sgt i64 %forbodyphi, 4
  %cmp2 = zext i1 %cmp to i64
  %ifcmp = icmp ne i64 %cmp2, 0
  br i1 %ifcmp, label %ifthen, label %ifcont

ifthen:                                           ; preds = %ifcond
  br label %ifcont

ifcont:                                           ; preds = %ifthen, %ifcond
  br label %forcond

return:                                           ; preds = %forafterphi
  ret i64 %forafterphi9
}
