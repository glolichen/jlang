# JLang

Compiler for very minimal C-like language. Compiles to LLVM IR.

## Grammar

```
<statement_list> | "{" { <statement> } "}"

<statement> ::= <assignment> ";"
             |  <func_call> ";"
             |  <return> ";"
             |  <conditional>
             |  <for_loop>
             |  ";"

<return> ::= <expression>

<conditional> ::= "if" "(" <expression> ")" <statement_list> ["else" <statement_list>]

<for_loop> ::= "for" "(" [<assignment>] ";" [<expression>] ";" [<assignment>] ")" <statement_list>

<assignment> ::= identifier "=" <expression>

<func_call> ::= identifier <expression_list>

<expression_list> ::= "(" { <expression> "," } <expression> ")"
                   |  "()"

<expression> ::= <expr_no_comp> [("=="|"!="|"<"|"<="|">"|">=") <expr_no_comp>]

<expr_no_comp> ::= ["+"|"-"] <term> { ("+"|"-") <term> }

<term> ::= <factor> { ("*"|"/") <factor> }

<factor> ::= identifier
          |  number
          |  <func_call>
          |  "(" <expression> ")"
```

## AST Structure

Didn't bother with inheritance (AST node is just a list of children).

 - `STATEMENT`: ONE CHILD ONLY: `ASSIGN`, `FUNC_CALL`, `RETURN` or `CONDITIONAL`
 - `EXPRESSION`: IF COMPARISON, (`EXPR_NO_COMP`, comparator, `EXPR_NO_COMP`), IF NOT, (`EXPR_NO_COMP`).
 - `EXPR_NO_COMP`: `TERM`, plus/minus, `TERM`, plus/minus, `TERM`, etc etc.
 - `TERM`: `FACTOR`, star/slash, `FACTOR`, star/slash, `FACTOR`, etc etc.
 - `FACTOR`: identifier, number or `EXPRESSION`.
 - `ASSIGN`: identifier, `EXPRESSION`.
 - `FUNC_CALL`: identifier, `EXPR_LIST` IN ALL CASES, IF PARAMS ARE VOID THEN `EXPR_LIST` LENGTH 0.
 - `EXPR_LIST`: list of `EXPRESSION`. LENGTH 0 IF PARAMS ARE VOID.
 - `CONDITIONAL`: `EXPRESSION`, `STMT_LIST`, (IF NO ELSE, NO MORE, IF THERE IS ELSE: `EXPRESSION`).
 - `FOR`: `ASSIGNMENT` (initial), `CONDITIONAL` (termination condition), `ASSIGNMENT` (every cycle), `STMT_LIST` ALWAYS HAS 4 CHILDREN, CHILDREN HAVE NO CHILDREN IF LEFT EMPTY.
 - `CONTINUE`/`BREAK`: no children

## Implementation Notes

If a variable is assigned for the first time in a block (such as a conditional), it will be "forgotten" as soon as it exits scope. Future uses of that variable will result in an error.

## Known Bugs
 - Nested if broken
 - Variable declared in for loop assignment (for (i = 0; ...)) remains in scope after loop termination
 - Cannot return from before end of body

## LLVM

 - https://www.owenstephens.co.uk/blog/2018/09/25/getting-started-with-the-newer-llvm-c-api.html
 - https://pauladamsmith.com/blog/2015/01/how-to-get-started-with-llvm-c-api.html   
 - https://llvm.org/docs/tutorial/
 - https://github.com/benbjohnson/llvm-c-kaleidoscope (very useful)
 - https://www.javaadvent.com/2025/12/java-hello-world-llvm-edition.html
