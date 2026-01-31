; ModuleID = 'test/iotest.bc'
source_filename = "test/iotest"

define i32 @main() {
entry:
  %getchartmp = call i8 @getchar()
  %getcharcasttmp = sext i8 %getchartmp to i32
  %getchartmp1 = call i8 @getchar()
  %getcharcasttmp2 = sext i8 %getchartmp1 to i32
  call void @putchar(i32 %getcharcasttmp)
  %multmp = mul i32 %getcharcasttmp, %getcharcasttmp2
  %subtmp = sub i32 %multmp, 200
  %subtmp3 = sub i32 %subtmp, %getcharcasttmp
  %addtmp = add i32 %subtmp3, %getcharcasttmp2
  %cmptmp = icmp slt i32 %addtmp, 50
  %cmptmp2 = zext i1 %cmptmp to i32
  %multmp4 = mul i32 %cmptmp2, 10
  %addtmp5 = add i32 %addtmp, %multmp4
  ret i32 %addtmp5
}

declare i8 @getchar()

declare void @putchar(i32)
