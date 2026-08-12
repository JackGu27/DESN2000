//Without incldung any C files, the ARM compiler doesn't include clib,
//which is quite useful!

extern void clear_registers(void);
extern void asm_entry(void);

int main(void) {
	//Reset all registers to zero (useful for debugging).
	clear_registers();
	
	//Jump to student's assembly code
	asm_entry();

	return 0;
}
