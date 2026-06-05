; ModuleID = 'test/iotest.bc'
source_filename = "test/iotest"

define i32 @main(i32 %a, i32 %b, i32 %c, i32 %d) {
entry:
  %0 = call i8 @getchar()
  %intcasttmp = sext i8 %0 to i32
  %addtmp = add i32 %intcasttmp, 5
  %skibidivalue = call i32 @skibidi()
  %retvalue = add i32 %addtmp, %skibidivalue
  ret i32 %retvalue
}

define i32 @skibidi() {
  ret i32 3;
}

declare i8 @getchar()
