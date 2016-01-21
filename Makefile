SUBDIRS := init \
		   uboot \
		   kernel \
		   arch \
		   mm \
		   lib \
		   tools \
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
	@make -s -C $@

ECHO:
	@echo ==============================================
	@echo            Staring Compile.....
	@echo ==============================================
