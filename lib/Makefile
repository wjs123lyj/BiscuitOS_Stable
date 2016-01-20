LSUBDIR := kernel 
LSUBDIR += mm
LSUBDIR += virtual
		
export CC DEBUG_FLAGS FLAGS

all:    ${LSUBDIR}
${LSUBDIR} : ECHO
	@make -C $@

ECHO:
