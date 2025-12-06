CC=clang
CFLAGS=-I. -Wall -Wextra -g --debug \
	   -fsanitize=address,undefined -static-libasan
#
# CFLAGS=-I. -Wall -Wextra -g -O3 

OBJ = main.o lex.o ast.o parse.o

build: $(OBJ) 
	$(CC) -o jlang $^ $(CFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f jlang *.o

