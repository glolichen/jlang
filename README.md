### JLang

Compiler for an extremely minimal C-like language.

```
<statement> ::= <assignment>;

<assignment> ::= identifier "=" <expression>

TODO <condition> ::= <expression> ("=="|"!="|"<"|"<="|">"|">=") <expression>

<expression> ::= ["+"|"-"] <term>
              |  ["+"|"-"] <term> ("+"|"-") <term>

<term> ::= <factor>
        |  <factor> ("*"|"/") <factor>

<factor> ::= identifier
          |  number
          |  "(" <expression> ")"
```
