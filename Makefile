CC=clang
CFLAGS=-I. -Wall -Wextra -g --debug \
	   -fsanitize=address,undefined -static-libasan \
	   `llvm-config --cflags`

LDFLAGS=`llvm-config --cxxflags --ldflags --libs mcjit core executionengine interpreter analysis native bitwriter --system-libs`
#
# CFLAGS=-I. -Wall -Wextra -g -O3 

OBJ = src/main.o src/lex.o src/ast.o src/parse.o src/codegen.o src/strmap.o

build: $(OBJ) 
	$(CC) -o jlang $^ $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) 

clean:
	rm -f jlang src/*.o test/*.o


