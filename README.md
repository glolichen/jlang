# JLang

Compiler for very minimal C-like language. Compiles to LLVM IR.

## Grammar

```
<statement_list> | "{" { <statement> } "}"

<statement> ::= <assignment> ";"
             |  <func_call> ";"
             |  <return> ";"
             |  <conditional> ";"
             |  ";"

<return> ::= <expression>

<conditional> ::= "if" "(" <expression> ")" <statement_list> ["else" <statement_list>]

<assignment> ::= identifier "=" <expression>

<func_call> ::= identifier <expression_list>

<expression_list> ::= "(" { <expression> "," } <expression> ")"
                   |  "()"

<expression> ::= <expr_no_comp> [("=="|"!="|"<"|"<="|">"|">=") <expr_no_comp>]

<expr_no_comp> ::= ["+"|"-"] <term> { ("+"|"-") <term> }

<term> ::= <factor> { ("*"|"/") <factor> }

<factor> ::= identifier
          |  number
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

## LLVM

 - https://www.owenstephens.co.uk/blog/2018/09/25/getting-started-with-the-newer-llvm-c-api.html
 - https://pauladamsmith.com/blog/2015/01/how-to-get-started-with-llvm-c-api.html   
 - https://llvm.org/docs/tutorial/
 - https://github.com/benbjohnson/llvm-c-kaleidoscope (very useful)
