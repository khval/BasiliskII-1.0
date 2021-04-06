
void dump_68k(char *code, char *code_end)
{
	char *ptr;
	char *ncode;
	char opcodeName[LEN_DISASSEMBLE_OPCODE_STRING], operands[LEN_DISASSEMBLE_OPERANDS_STRING];

	for (;code<code_end;code=ncode)
	{
		ncode = (char *) Disassemble68k( (APTR) code, opcodeName, operands);

		printf("%08x: %s %s\n", code, opcodeName,operands);

		for (ptr = code; ptr<ncode; ptr++) printf("%02x ",*ptr);
		printf("\n");

		code = ncode;
	}
}
