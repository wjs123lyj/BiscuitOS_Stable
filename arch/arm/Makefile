LSUBDIR := kernel 
LSUBDIR += mach-s3c64xx
LSUBDIR += mm

export CC DEBUG_FLAGS FLAGS

all:    ${LSUBDIR}
${LSUBDIR} : ECHO
	@make -C $@

ECHO:
