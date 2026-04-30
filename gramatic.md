KasTLe Grammar:

<file>       ::= { <top_decl> } <main> { <top_decl> }

<top_decl>   ::= ( <var_decl> ";" ) | <func_decl> | <type_decl>

<var_decl>   ::= "var" { <type_mod> } <type> <name> [ "=" <expr> ]

<func_decl>  ::= "func" { <type_mod> } <type> <name>
                 "(" [ <param_list> ] ")" <body>
<param_list> ::= <param> { "," <param> }
<param>      ::= { <type_mod> } <type> <name>

<type_decl>  ::= <typedef> | <block_decl>
<typedef>    ::= "typedef" <type> "->" <name>
<block_decl> ::= "block" <name> "{" <field> ";" { <field> ";" } "}" ";"
<field>      ::= { <type_mod> } <type> <name>

<main>       ::= "main" <body>

<type_mod>   ::= "const" | "mutable" | "register" | "stack"
<type>       ::= <name> { "*" } { "[" <number> "]" }

<line>       ::= <var_decl> | <assign> | <call>
               | <condition> | <for> | <while>
               | <return> | "break" | "continue" | "exit"

<return>     ::= "return" [ <expr> ]
<assign>     ::= <lvalue> "=" <expr>
<lvalue>     ::= { "*" } <var>
<call>       ::= <name> "(" [ <expr> { "," <expr> } ] ")"

<condition>  ::= "if" "(" <expr> ")" <body>
                 { "elif" "(" <expr> ")" <body> }
                 [ "else" <body> ]

<for>        ::= "for" "(" [ <var_decl> | <assign> ] ";"
                           [ <expr> ] ";"
                           [ <assign> ] ")"
                 <body>

<while>      ::= "while" "(" <expr> ")" <body>

<body>       ::= "{" { <line> ";" } "}"

<expr>       ::= <cmp_step> { <log_op> <cmp_step> }
<log_op>     ::= "and" | "or"
<cmp_step>   ::= <add_step> [ <cmp_op> <add_step> ]
<cmp_op>     ::= "==" | "!=" | "<" | "<=" | ">" | ">="
               | "equal" | "not_equal"
               | "less" | "less_or_equal"
               | "greater" | "greater_or_equal"
<add_step>   ::= <mul_step> { ( "+" | "-" ) <mul_step> }
<mul_step>   ::= <unary> { ( "*" | "/" | "%" ) <unary> }
<unary>      ::= { "&" | "*" | "-" } <atom>
<atom>       ::= "(" <expr> ")" | <call> | <var> | <number> | <string>

<var>        ::= <name> { ( "." <name> ) | ( "->" <name> ) | ( "[" <expr> "]" ) }
<name>       ::= (A-Z | a-z | "_") { A-Z | a-z | 0-9 | "_" }
<number>     ::= 0-9 { 0-9 }
<string>     ::= "\"" { ASCII } "\""
