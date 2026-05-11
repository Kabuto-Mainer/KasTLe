# ===================================================================
# FLAGS
# ===================================================================
flags = -D_DEBUG -g -O3 -std=c++20 \
-Wall -Wextra -Wshadow -Wcast-align -Wcast-qual -Wconversion \
-Wformat=2 -Wformat-security -Wpointer-arith -Wredundant-decls \
-Wunused -Wundef -Wunreachable-code -Winline \
-Wdocumentation -Wcomma -Wtautological-compare \
-Wmissing-prototypes -Wstrict-prototypes \
-fstack-protector-strong -fno-omit-frame-pointer \
-fsanitize=address,undefined,float-divide-by-zero \
-fPIE -pie


# ===================================================================
# FILES
# ===================================================================
# parse_f = Frontend/Parse.cpp
token_f = 		Frontend/Tokenize/Token.cpp

type_map_f =	Structs/TypeMap/TypeMap.cpp
sym_map_f = 	Structs/SymMap/SymMap.cpp
str_map_f = 	Structs/StrMap/StrMap.cpp
back_map_f = 	Structs/BackMap/BackMap.cpp
label_map_f =   Structs/LabelMap/LabelMap.cpp

parse_f =       Frontend/Parsing/Parse.cpp
analysis_f = 	Frontend/Analysis/Analysis.cpp
backend_f = 	Backend/Backend.cpp Backend/GenNASM.cpp Backend/BackIR.cpp Backend/GenByte.cpp

common_f = 		Common/Common.cpp
ast_common_f =  Frontend/ASTCommon.cpp
ast_dump_f =    Frontend/Dump/DumpAst.cpp
diagnostic_f =  Common/Diagnostic.cpp


struct_f =      $(type_map_f) $(str_map_f) $(sym_map_f) $(back_map_f) $(label_map_f)

# ===================================================================
# DIRS
# ===================================================================
token_d = 		Frontend/Tokenize

type_map_d =	Structs/TypeMap
sym_map_d =		Structs/SymMap
str_map_d = 	Structs/StrMap
back_map_d = 	Structs/BackMap
label_map_d =   Structs/LabelMap

parse_d = 		Frontend/Parsing
analysis_d =    Frontend/Analysis
backend_d = 	Backend

common_d = 		Common
system_d =      Structs/BackMap
ast_common_d =  Frontend
ast_dump_d =    Frontend/Dump
diagnostic_d =  $(common_d)

dir_flags =  -I$(token_d) 		-I$(type_map_d) 	-I$(str_map_d) 		-I$(sym_map_d) \
			 -I$(parse_d)       -I$(common_d)		-I$(ast_common_d) 	-I$(ast_dump_d) \
			 -I$(analysis_d)    -I$(system_d)		-I$(backend_d)		-I$(back_map_d) \
			 -I$(label_map_d)


# ===================================================================
# BUILDS
# ===================================================================
build_test_token:
	clang Tests/test_token.cpp $(token_f) $(str_map_f) $(common_f) $(flags) \
	$(diagnostic_f) $(dir_flags) -o Tests/token_test.elf

build_test_sym:
	clang Tests/test_sym.cpp $(struct_f) $(common_f) $(flags) $(dir_flags) -o Tests/sym_test.elf

build_test_parse:
	clang Tests/test_parse.cpp $(token_f) $(struct_f) $(parse_f) $(common_f) \
	$(ast_dump_f) $(ast_common_f) $(diagnostic_f) $(flags) $(dir_flags) -o Tests/parse_test.elf

build_test_analysis:
	clang Tests/test_analysis.cpp $(token_f) $(struct_f) $(parse_f) $(common_f) \
	$(ast_dump_f) $(ast_common_f) $(diagnostic_f) $(analysis_f) $(flags) $(dir_flags) -o Tests/analysis_test.elf

build:
	clang compiler.cpp $(token_f) $(struct_f) $(parse_f) $(common_f) \
	$(ast_dump_f) $(ast_common_f) $(diagnostic_f) $(analysis_f) $(backend_f) \
	$(flags) $(dir_flags) -o Bin/KasTle.elf


build_prog:
	nasm -f elf64 -o Bin/1.o Bin/1.asm
	gcc -nostartfiles -no-pie Bin/1.o -o Bin/1.elf
	chmod +x Bin/1.elf

# ===================================================================
# RUN
# ===================================================================
run_test_token:
	./Tests/token_test.elf

run_test_sym:
	./Tests/sym_test.elf

run_test_parse:
	./Tests/parse_test.elf

run_test_analysis:
	./Tests/analysis_test.elf

run:
	./Bin/KasTle.elf

run_prog:
	./Bin/1.elf

comp:
	./Bin/KasTle.elf
	nasm -f elf64 -o Bin/1.o Bin/1.asm
	gcc -nostartfiles -no-pie Bin/1.o -o Bin/1.elf
	chmod +x Bin/1.elf
	./Bin/1.elf

