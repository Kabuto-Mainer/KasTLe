# =============================================================================
# CONFIG
# =============================================================================
C        := clang
TARGET   := KasTle
BIN_DIR  := Bin
OBJ_DIR  := build

# =============================================================================
# FLAGS
# =============================================================================
WARNINGS := -Wall -Wextra -Wshadow -Wcast-align -Wcast-qual -Wconversion \
            -Wformat=2 -Wformat-security -Wpointer-arith -Wredundant-decls \
            -Wundef -Wunreachable-code -Wunused -Wcomma \
            -Wtautological-compare -DEMIT_DEBUG

CD_FLAGS   := -std=c++20 -g -O0 -D_DEBUG $(WARNINGS) \
              -fno-omit-frame-pointer -fstack-protector-strong \
              -fsanitize=address,undefined,float-divide-by-zero \
              -fPIE -MMD -MP

LDD_FLAGS  := -fsanitize=address,undefined -pie


CR_FLAGS   := -std=c++20 -O3 -DNDEBUG $(WARNINGS) -fPIE -MMD -MP
LDR_FLAGS  := -pie


# =============================================================================
# SOURCES
# =============================================================================
SRC_DIRS := Frontend Frontend/Tokenize Frontend/Parsing Frontend/Analysis \
            Frontend/Dump Backend/GenIR Backend/Gen Backend/IR Backend/Data \
			Common Structs/TypeMap Structs/SymMap Structs/StrMap \
            Structs/BackMap Structs/LabelMap Common/Std System

INCLUDES := $(addprefix -I,$(SRC_DIRS))

SRC := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
OBJ := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEP := $(OBJ:.o=.d)

BIN := $(BIN_DIR)/$(TARGET).elf

# =============================================================================
# RULES
# =============================================================================
.PHONY: all clean run help
.DEFAULT_GOAL := all

all: $(BIN)

$(BIN): $(OBJ) | $(BIN_DIR)
	@echo "  LD   $@"
	@$(C) $(LDD_FLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "  C  $<"
	@$(C) $(CD_FLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

run: $(BIN)
	./$(BIN) $(ARGS)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "All Cleared"

help:
	@echo "Goals:"
	@echo "  make             build compiler"
	@echo "  make run         build & run (ARGS='...' for arguments)"
	@echo "  make clean       clean all object's files"
	@echo ""
	@echo "For fast compilation use ./KasTle:"
	@echo "  ./ktl hello.ktl                generate own ELF (default)"
	@echo "  ./ktl -e nasm hello.ktl        generate ELF by NASM + gcc"
	@echo "  ./ktl -e asm hello.ktl         only .asm"

-include $(DEP)
