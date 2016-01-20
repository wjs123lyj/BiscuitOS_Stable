all: 
	@${CC} *.c ${FLAGS} -o a 2>&1 | tee compiler.log
	@rm *.c
	@echo =============================================
	@echo          Buddy Memory System
	@echo =============================================
	@./a
