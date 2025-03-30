/* simple emulator for programs */

#define MEM_SIZE 512
#define CONSUME_IMMEDIATE_8() pc += 1;
#define CONSUME_IMMEDIATE_16() pc += 2;

typedef unsigned char byte;
typedef unsigned short ushort;

byte regs[4];
ushort pc = 0;
ushort sp = MEM_SIZE - 1;
bool CY = 0, ZE = 0, HALTED = 0;

/* no need for NOP instruction as its just e.g. MOV B,B*/
enum
{
        INST_LDI,    /*R=imm*/
        INST_MOVA,   /*R=A*/
        INST_MOVB,   /*R=B*/
        INST_MOVC,   /*R=C*/
        INST_MOVD,   /*R=D*/
        INST_MOVSPH, /*R=SP(H)*/
        INST_MOVSPL, /*R=SP(L)*/
        INST_LDR,    /*R=ram[imm]*/
        INST_STR,    /*ram[imm]=R*/
        INST_ADD,    /*A+=R*/
        INST_SUB,    /*A-=R*/
        INST_ADC,    /*A+=R*/
        INST_SBB,    /*A-=R+CY*/
        INST_AND,    /*A&=R*/
        INST_OR,     /*A|=R*/
        INST_XOR,    /*A^=R*/
        INST_ROL,    /*R=ROL(R)*/
        INST_ROR,    /*R=ROR(R)*/
        INST_LDR_C,  /*R=ram[C_D]*/
        INST_STR_C,  /*ram[C_D]=R*/
        INST_JMP,    /*PC=imm*/
        INST_JC,     /*(CY) ? PC=imm : .*/
        INST_JZ,     /*(ZE) ? PC=imm : .*/
        INST_JNC,    /*(!CY) ? PC=imm : .*/
        INST_JNZ,    /*(!ZE) ? PC=imm : .*/
        INST_PUSH_R,
        INST_POP_R,
        INST_SPCD, /*SP=CD*/
        INST_PCCD, /*PC=CD*/
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
        byte *deref_cd = &memory[cd];
        byte imm8 = memory[pc & (MEM_SIZE - 1)];
        ushort imm16 = (imm8 << 8) | memory[(pc + 1) & (MEM_SIZE - 1)];
        byte *dst = &regs[short_arg & 3];
        switch (opcode)
        {
        case INST_HLT:
                HALTED = true;
                break;
        case INST_LDI:
                *dst = imm8;
                CONSUME_IMMEDIATE_8();
                break;
        case INST_MOVA:
                *dst = regs[0];
                break;
        case INST_MOVB:
                *dst = regs[1];
                break;
        case INST_MOVC:
                *dst = regs[2];
                break;
        case INST_MOVD:
                *dst = regs[3];
                break;
        case INST_MOVSPH:
                *dst = sp >> 8;
                break;
        case INST_MOVSPL:
                *dst = sp & 0xFF;
                break;
        case INST_LDR:
                *dst = memory[imm16 & (MEM_SIZE - 1)];
                CONSUME_IMMEDIATE_16();
                break;
        case INST_STR:
                memory[imm16 & (MEM_SIZE - 1)] = *dst;
                CONSUME_IMMEDIATE_16();
                break;
        case INST_ADD:
        {
                ushort res = regs[0] + *dst;
                CY = res > 0xFF;
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_SUB:
        {
                ushort res = regs[0] - *dst;
                CY = res > 0xFF; // Borrow
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_ADC:
        {
                ushort res = regs[0] + *dst + CY;
                CY = res > 0xFF;
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_SBB:
        {
                ushort res = regs[0] - *dst - CY;
                CY = res > 0xFF;
                regs[0] = res;
                ZE = regs[0] == 0;
                break;
        }
        case INST_AND:
                regs[0] &= *dst;
                ZE = regs[0] == 0;
                break;
        case INST_OR:
                regs[0] |= *dst;
                ZE = regs[0] == 0;
                break;
        case INST_XOR:
                regs[0] ^= *dst;
                ZE = regs[0] == 0;
                break;
        case INST_ROL:
        {
                bool new_carry = (*dst >> 7);
                *dst = (*dst << 1) | CY;
                CY = new_carry;
                ZE = *dst == 0;
                break;
        }
        case INST_ROR:
        {
                bool new_carry = *dst & 1;
                *dst = (*dst >> 1) | (CY << 7);
                CY = new_carry;
                ZE = *dst == 0;
                break;
        }
        case INST_LDR_C:
                *dst = memory[cd];
                break;
        case INST_STR_C:
                memory[cd] = *dst;
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
                memory[sp--] = *dst;
                break;

        case INST_POP_R:
                *dst = memory[++sp];
                break;

        case INST_SPCD:
                sp = cd;
                break;

        case INST_PCCD:
                pc = cd;
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
        Serial.print("PC=");
        Serial.println(pc, HEX);
        Serial.print("SP=");
        Serial.println(sp, HEX);
}

void loop()
{
}
