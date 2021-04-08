
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

void show_sigs(char *txt)
{
	struct Task *t;
	int n;

	t = FindTask(NULL);

	printf("\n%s\n",txt);

	for (n=0;n<31;n++)
	{
		if ((t -> tc_SigAlloc & 0x1FFF0000) & (1 << n))	printf("Sig %08x\n",1 << n);
	}

/*
	printf("\n%s\n",txt);

	printf("BIT: ");
	for (n=31;n>=0;n--) printf("%d", (t -> tc_SigAlloc & 0x1FFF0000) & (1<<n) ? 1 : 0);
	printf("\n");

	printf("HEX: %08X\n",t -> tc_SigAlloc & 0x1FFF0000 );

*/
}