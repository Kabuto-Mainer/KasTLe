KasTLe Grammar:

<file> ::=  { ( <var_decl> ";" ) | <func_decl> | <type_decl> }
            <main>
            { ( <var_decl> ";" ) | <func_decl> | <type_decl> }

<var_decl> ::= "var" { <type_mod> } <type> <name> [ "=" <expr> ]
<func_decl> ::= "func"  { <type_mod> } <type> <name> "("
                        [ { <type_mod> } <type> <name>
                        {"," { <type_mod> } <type> <name> } ] ")"
                        ( "{" { ( <line> | <return>) ";" } "}" )
<return> ::= "return" <expr>

<type_decl> ::= <typedef> | <block_decl>
<block_decl> ::=    "block" <name> "{" { <type_mod> } <type> <name> ";"
                    { { <type_mod> } <type> <name> ";" } "}" ";"
<typedef> ::=   "typedef" <type> "->" <name>

<main> ::= "main" "{" ( <line> | "exit" ) ";" { ( <line> | "exit" ) ";" } "}"

<type_mod> ::= "const" | "mutable" | "register" | "stack"
<type> ::= <name> { "*" } { "[" { 0-9 }  "]" }

<line> ::= <var_decl> | <assign> | <call> | <condition> | <for> | <while>
<assign> ::= <var> "=" <expr>
<call> ::= <name> "(" [ <expr> { "," <expr> } ] ")"
<condition> ::= "if" "(" <expr> ")" "{" <line> ";" { <line> ";" } "}"
                { "elif" "(" <expr> ")" "{" <line> ";" { <line> ";" } "}" }
                [ "else" "{" <line> ";" { <line> ";" } "}" ]

<for> ::= "for" "(" [ <var_decl> | <assign> ] ";" <expr> ";" [ <assign> ] ")"
          "{" ( <line> | "break" | "continue" ) ";" { ( <line> | "break" | "continue" ) ";" } "}"

<while> ::= "while" "(" <expr> ")"
            "{" ( <line> | "break" | "continue" ) ";" { ( <line> | "break" | "continue" ) ";" } "}"

<expr> ::= <cmp_step> { ( "and" | "or" ) <cmp_step> }
<cmp_step> ::= <ptr_step> [ ( "==" | ">" | ">=" | "<" | "<=" | "!=" ) <ptr_step> ]
<ptr_step> ::= ( { <get_ptr> } | { <unget_ptr> } ) <mul_step>
<get_ptr> ::= "&"
<unget_ptr> ::= "*"

<mul_step> ::= <add_step> ( "*" | "/" | "%" ) <add_step> { ( "*" | "/" | "%" ) <add_step> }
<add_step> ::= <use_value> ( "+" | "-" ) <use_value> { ( "+" | "-" ) <use_value> }
<use_value> ::= ( "(" <expr> ")" ) | <call> | <var> | <number> | <string>
<var> ::= <name> { ( "." <var>  ) | ( "[" <expr> "]" ) }
<name> ::= (A-Z | a-z | "_") { A-Z | a-z | 0-9 | "_" }
<number> ::= [ "-" ] 0-9 { 0-9 }
<string> ::= "\"" [ ASCII ] "\""
