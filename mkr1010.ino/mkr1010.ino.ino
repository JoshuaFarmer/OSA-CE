/* simple emulator for programs */

#define MEM_SIZE 512
#define CONSUME_IMMEDIATE_8() pc += 1;
#define CONSUME_IMMEDIATE_16() pc += 2;

typedef unsigned char byte;
typedef unsigned short ushort;

byte regs[8]; /*A,B,C,D,E,F,W,Z*/
ushort pc = 0;
ushort sp = MEM_SIZE - 1;
bool CY = 0, ZE = 0, HALTED = 0;

/* no need for NOP instruction as its just e.g. LD B,B*/
enum
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
        INST_SPCD, /*SP=CD*/
        INST_SPAB, /*SP=AB*/
        INST_PCCD, /*PC=CD*/
        INST_PCAB, /*PC=AB*/
        INST_HLT,
};

inline byte construct(byte opcode, byte dst)
{
        return (opcode & 31) | (dst << 5);
}

void execute(byte *memory)
{
        byte inst = memory[(pc++ & (MEM_SIZE - 1))];
        byte opcode = inst & 31;
        byte short_arg = inst >> 5; /* 3 bit argument */
        ushort cd = (regs[2] << 8) | regs[3];
        ushort ab = (regs[0] << 8) | regs[1];
        byte *deref_cd = &memory[cd];
        byte imm8 = memory[pc & (MEM_SIZE - 1)];
        ushort imm16 = (imm8 << 8) | memory[(pc + 1) & (MEM_SIZE - 1)];
        byte *arg_reg = &regs[short_arg];
        switch (opcode)
        {
        case INST_HLT:
                HALTED = true;
                break;
        case INST_LDI:
                *arg_reg = imm8;
                CONSUME_IMMEDIATE_8();
                break;
        case INST_LDA:
                *arg_reg = regs[0];
                break;
        case INST_LDB:
                *arg_reg = regs[1];
                break;
        case INST_LDC:
                *arg_reg = regs[2];
                break;
        case INST_LDD:
                *arg_reg = regs[3];
                break;
        case INST_LDSPH:
                *arg_reg = sp >> 8;
                break;
        case INST_LDSPL:
                *arg_reg = sp & 0xFF;
                break;
        case INST_LDR:
                *arg_reg = memory[imm16 & (MEM_SIZE - 1)];
                CONSUME_IMMEDIATE_16();
                break;
        case INST_STR:
                memory[imm16 & (MEM_SIZE - 1)] = *arg_reg;
                CONSUME_IMMEDIATE_16();
                break;
        case INST_ADD:
        {
                ushort res = regs[0] + *arg_reg;
                CY = res > 0xFF;
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_SUB:
        {
                ushort res = regs[0] - *arg_reg;
                CY = res > 0xFF; // Borrow
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_ADC:
        {
                ushort res = regs[0] + *arg_reg + CY;
                CY = res > 0xFF;
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_SBB:
        {
                ushort res = regs[0] - *arg_reg - CY;
                CY = res > 0xFF;
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_AND:
                regs[0] &= *arg_reg;
                ZE = regs[0] == 0;
                break;
        case INST_OR:
                regs[0] |= *arg_reg;
                ZE = regs[0] == 0;
                break;
        case INST_XOR:
                regs[0] ^= *arg_reg;
                ZE = regs[0] == 0;
                break;
        case INST_ROL:
        {
                bool new_carry = (*arg_reg >> 7);
                *arg_reg = (*arg_reg << 1) | CY;
                CY = new_carry;
                ZE = *arg_reg == 0;
                break;
        }
        case INST_ROR:
        {
                bool new_carry = *arg_reg & 1;
                *arg_reg = (*arg_reg >> 1) | (CY << 7);
                CY = new_carry;
                ZE = *arg_reg == 0;
                break;
        }
        case INST_LDR_C:
                *arg_reg = memory[cd];
                break;
        case INST_STR_C:
                memory[cd] = *arg_reg;
                break;
        case INST_JMP:
                pc = imm16 & (MEM_SIZE - 1);
                break;
        case INST_JC:
                if (CY)
                        pc = imm16 & (MEM_SIZE - 1);
                else
                        CONSUME_IMMEDIATE_16();
                break;
        case INST_JZ:
                if (ZE)
                        pc = imm16 & (MEM_SIZE - 1);
                else
                        CONSUME_IMMEDIATE_16();
                break;

        case INST_JNC:
                if (!CY)
                        pc = imm16 & (MEM_SIZE - 1);
                else
                        CONSUME_IMMEDIATE_16();
                break;

        case INST_JNZ:
                if (!ZE)
                        pc = imm16 & (MEM_SIZE - 1);
                else
                        CONSUME_IMMEDIATE_16();
                break;

        case INST_PUSH_R:
                memory[sp--] = *arg_reg;
                break;

        case INST_POP_R:
                *arg_reg = memory[++sp];
                break;

        case INST_SPCD:
                sp = cd;
                break;

        case INST_PCCD:
                pc = cd;
                break;

        case INST_SPAB:
                sp = ab;
                break;

        case INST_PCAB:
                pc = ab;
                break;

        default:
                Serial.print("Invalid opcode: 0x");
                Serial.println(opcode, HEX);
        }

        pc = pc & (MEM_SIZE - 1);
}

void setup()
{
        Serial.begin(9600);
        /*LDI A,1*/
        /*LDI B,3*/
        /*ADD B*/
        byte memory[MEM_SIZE];
        memory[0] = construct(INST_LDI, 0);
        memory[1] = 1;
        memory[2] = construct(INST_LDI, 1);
        memory[3] = 3;
        memory[4] = construct(INST_ADD, 1);
        memory[5] = construct(INST_HLT, 0);
        while (!HALTED)
        {
                execute(memory);
        }

        Serial.print("Emulator Halted\nA=");
        Serial.println(regs[0], HEX);
        Serial.print("B=");
        Serial.println(regs[1], HEX);
        Serial.print("C=");
        Serial.println(regs[2], HEX);
        Serial.print("D=");
        Serial.println(regs[3], HEX);
        Serial.print("E=");
        Serial.println(regs[4], HEX);
        Serial.print("F=");
        Serial.println(regs[5], HEX);
        Serial.print("W=");
        Serial.println(regs[6], HEX);
        Serial.print("Z=");
        Serial.println(regs[7], HEX);
        Serial.print("PC=");
        Serial.println(pc, HEX);
        Serial.print("SP=");
        Serial.println(sp, HEX);
}

void loop()
{
}
