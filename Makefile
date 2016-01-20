SUBDIRS := init \
		   lib  \
		   uboot \
		   obj \


FLAGS := -g -pg -I../include
DEBUG_FLAGS := -C -E -g -Wfatal-errors
CC   := gcc
OBJS_DIR = obj
BIN = mainapp
BIN_DIR = bin


export CC DEBUG_FLAGS FLAGS OBJS_DIR BIN BIN_DIR 

all: $(SUBDIRS)
$(SUBDIRS) : ECHO
	@make -C $@

ECHO:
	@echo ==============================================
	@echo            Staring Compile.....
	@echo ==============================================
