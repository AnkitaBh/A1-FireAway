# Author: Arden Diakhate-Palme
# Date: 14.12.22
# Note: Must run executable with SUDO privileges
#

VPATH = /usr/include

IDIR=inc
CC=gcc
CFLAGS=-I$(IDIR)
RM=rm 

# := evaluated only once
OBJ_DIR := obj
SRC_DIR := src
BIN_DIR := .

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))
EXE := $(BIN_DIR)/run_gw_router

CPPFLAGS := -Iinclude -MMD -MP # -I is a preprocessor flag, not a compiler flag
CFLAGS   := -Wall -pthread -g 
LDFLAGS  := -Llib              # -L is a linker flag
LDLIBS   := -lm -lpigpio -lrt  # Left empty if no libs are needed
LDLIBS   += -I/usr/include/json-c -L/usr/lib -ljson-c #json-c linking
CFLAGS   += -I$(IDIR) #needed for json-c

.PHONY: all
all : $(EXE) 

# $(BIN_DIR) might not exist yet
$(EXE) : $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $(EXE)

# $(OBJ_DIR) might not exist yet
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Binary or Object-file directories may not exist yet
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

.PHONY: clean

# The @ disables the echoing of the command
clean:
	@$(RM) -rv $(OBJ_DIR) 

-include $(OBJ:.o=.d) # The dash silences errors when files don't exist (yet)