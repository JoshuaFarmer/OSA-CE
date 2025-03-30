/* create example program (for mkr) */
#include<stdio.h>
#include<stdlib.h>

/* no need for NOP instruction as its just e.g. LD B,B*/
enum NO_EXT /* no prefix */
{
        INST_LDI,   /*R=imm*/
        INST_LDA,   /*R=A*/
        INST_LDB,   /*R=B*/
        INST_LDC,   /*R=C*/
        INST_LDD,   /*R=D*/
        INST_LDSPH, /*R=SP(H)*/
        INST_LDSPL, /*R=SP(L)*/
        INST_LDR,   /*R=ram[imm]*/
        INST_STR,   /*ram[imm]=R*/
        INST_ADD,   /*A+=R*/
        INST_SUB,   /*A-=R*/
        INST_ADC,   /*A+=R*/
        INST_SBB,   /*A-=R+CY*/
        INST_AND,   /*A&=R*/
        INST_OR,    /*A|=R*/
        INST_XOR,   /*A^=R*/
        INST_ROL,   /*R=ROL(R)*/
        INST_ROR,   /*R=ROR(R)*/
        INST_LDR_C, /*R=ram[C_D]*/
        INST_STR_C, /*ram[C_D]=R*/
        INST_JMP,   /*PC=imm*/
        INST_JC,    /*(CY) ? PC=imm : .*/
        INST_JZ,    /*(ZE) ? PC=imm : .*/
        INST_JNC,   /*(!CY) ? PC=imm : .*/
        INST_JNZ,   /*(!ZE) ? PC=imm : .*/
        INST_PUSH_R,
        INST_POP_R,
        PREFIX_1C,
        PREFIX_1D,
        PREFIX_1E,
        PREFIX_1F,
        INST_HLT,
};

enum OPCODES_PREFIX_1C /* with 0x1C prefix */
{
        INST_LXI,
        INST_LXAB,
        INST_LXCD,
        INST_LXEF,
        INST_LXWZ,
        INST_SPCD, /*SP=CD*/
        INST_SPAB, /*SP=AB*/
        INST_SPEF, /*SP=EF*/
        INST_SPWZ, /*SP=WZ*/
        INST_PCCD, /*PC=CD*/
        INST_PCAB, /*PC=AB*/
        INST_PCEF, /*PC=EF*/
        INST_PCWZ, /*PC=WZ*/
        INST_OUT_R,
        INST_INP_R,
        INST_GMASK, /*A=IMASK*/
        INST_SMASK, /*IMASK=A*/
        INST_CALL,
        INST_IRET,
        INST_RET,
};

enum OPCODES_PREFIX_1D /* with 0x1D prefix */
{
        INST_INC,
        INST_DEC,
        INST_TEST,
};

typedef unsigned char byte;

 byte construct(byte opcode, byte dst)
{
        return (opcode & 31) | (dst << 5);
}

int main()
{
        byte example[] =
        {
                0x7F, /* magic */
                0xE, /* size */
                construct(INST_LDI, 7),
                3,
                construct(PREFIX_1C, 0),
                construct(INST_INP_R, 0),
                construct(PREFIX_1D, 0),
                construct(INST_TEST, 0),
                construct(INST_JZ, 0),
                0,
                2,
                construct(PREFIX_1C, 0),
                construct(INST_OUT_R, 0),
                construct(INST_JMP, 0),
                0,
                2,
       };

        FILE *f = fopen("example.bin","wb");
        if (!f) return -1;
        fwrite(example,1,sizeof(example),f);
        fclose(f);
        f = NULL;
}