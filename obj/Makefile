all: 
	@${CC} *.c ${FLAGS} -o a 2>&1 | tee compiler.log
	@rm *.c
	@echo ==============================================
	@echo          Buddy Memory System Running
	@echo ==============================================
	@./a
#	@./a > buddy.txt
#	@vi buddy.txt
