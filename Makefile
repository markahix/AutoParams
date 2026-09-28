CC=g++

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
AGIMUS_BIN_DIR := ../bin

EXE := $(BIN_DIR)/autoparams
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS := -Iinclude -MMD -MP
CFLAGS   := -Wall
LDFLAGS  := -Llib -static
LDLIBS   := -lm -lstdc++fs

.PHONY: all clean install

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) -std=c++17 $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

# install: link (not copy) the binary into $(AGIMUS_BIN_DIR). autoparams finds
# include/known_parameters.dat next to its own real location, so a copy on
# its own cannot find the library; a symlink resolves back to this tree.
install: $(EXE)
	mkdir -p $(AGIMUS_BIN_DIR)
	ln -sf $(abspath $(EXE)) $(AGIMUS_BIN_DIR)/autoparams

-include $(OBJ:.o=.d)
