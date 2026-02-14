# https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

CC = clang

CFLAGS = -I. -Wall -Wextra -g --debug -I$(IDIR) \
         `llvm-config --cflags` \
         -fsanitize=address,undefined -static-libasan \

		 # --analyze -Xclang -analyzer-output=html -o analyze/

LDFLAGS = `llvm-config --cxxflags --ldflags --libs mcjit core executionengine interpreter analysis native bitwriter --system-libs`

IDIR = include
ODIR = obj

_OBJ = main.o lex.o ast.o parse.o \
       utils/strmap.o utils/linkedlist.o \
       codegen/assignment.o codegen/conditional.o \
       codegen/expression.o codegen/forloop.o \
       codegen/function.o codegen/return.o \
       codegen/statement.o codegen/codegen.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

build: $(OBJ)
	$(CC) -o jlang $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(ODIR)/codegen/*.o *~ core # $(INCDIR)/*~ 

