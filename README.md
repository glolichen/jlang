# JLang

Compiler for very minimal C-like language.

## Grammar

```
<statement_list> | "{" { <statement> } "}"

<statement> ::= <assignment> ";"
             |  <func_call> ";"
             |  ";"

<assignment> ::= identifier "=" <expression>

<func_call> ::= identifier <expression_list>

<expression_list> ::= "(" { <expression> "," } <expression> ")"
                   |  "()"

<expression> ::= <expr_no_comp>
              |  <expr_no_comp> ("=="|"!="|"<"|"<="|">"|">=") <expr_no_comp>

<expr_no_comp> ::= ["+"|"-"] <term> { ("+"|"-") <term> }

<term> ::= <factor> { ("*"|"/") <factor> }

<factor> ::= identifier
          |  number
          |  "(" <expression> ")"
```

