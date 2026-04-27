# ===================================================================
# FLAGS
# ===================================================================
flags = -D_DEBUG -g -O3 \
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

common_f = 		Common/Common.cpp

# ===================================================================
# DIRS
# ===================================================================
token_d = 		Frontend/Tokenize

type_map_d =	Structs/TypeMap
sym_map_d =		Structs/SymMap
str_map_d = 	Structs/StrMap

common_d = Common/

dir_flags = -I$(token_d) -I$(str_map_d) -I$(common_d) -I$(sym_map_d) -I$(type_map_d)


# ===================================================================
# BUILDS
# ===================================================================
build_test_token:
	clang Tests/test_token.cpp $(token_f) $(str_map_f) $(common_f) $(flags) $(dir_flags) -o Tests/token_test.elf

build_test_sym:
	clang Tests/test_sym.cpp $(sym_map_f) $(type_map_f) $(str_map_f) $(common_f) $(flags) $(dir_flags) -o Tests/sym_test.elf


# ===================================================================
# RUN
# ===================================================================
run_test_token:
	./Tests/token_test.elf

run_test_sym:
	./Tests/sym_test.elf
