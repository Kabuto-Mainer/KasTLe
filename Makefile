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
token_f = Frontend/Tokenize/Token.cpp

str_map_f = Structs/StrMap/StrMap.cpp

common_f = Common/Common.cpp

# ===================================================================
# DIRS
# ===================================================================
token_d = Frontend/Tokenize

str_map_d = Structs/StrMap

common_d = Common/

dir_flags = -I$(token_d) -I$(str_map_d) -I$(common_d)


# ===================================================================
# BUILDS
# ===================================================================
build_token:
	clang Tests/test_token.cpp $(token_f) $(str_map_f) $(common_f) $(flags) $(dir_flags) -o token.elf


# ===================================================================
# RUN
# ===================================================================
run_token:
	./token.elf
