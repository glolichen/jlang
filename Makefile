CC=clang
CFLAGS=-I. -Wall -Wextra -g --debug
# CFLAGS=-I. -Wall -Wextra -g -O3 -fsanitize=address,undefined -static-libasan

OBJ = main.o lex.o

build: $(OBJ) 
	$(CC) -o jlang $^ $(CFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f jlang *.o

