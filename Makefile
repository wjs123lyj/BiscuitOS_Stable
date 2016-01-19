obj-y :=
obj-m :=
SUBDIRS := main \
		   lib \
		   uboot \


FLAGS := -g -pg
DEBUG_FLAGS := -C -E -g -Wfatal-errors
SRCS :=
CC   := gcc
export CC DEBUG_FLAGS SRCS obj-y obj-m

all: $(SUBDIRS)
$(SUBDIRS) : ECHO
	make -C $@

ECHO:
	@echo $(SUBDIRS)
	@echo begin compile

