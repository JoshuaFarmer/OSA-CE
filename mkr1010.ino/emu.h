#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>

void dump_registers();

#define MEM_ADDR(addr) ((addr) % (MEM_BUFF_SIZE))
#define MEM_BUFF_SIZE 512
#define CONSUMEIM() pc += 1;

bool CY = 0, ZE = 0, HALTED = 0;
byte regs[4] = {0,MEM_BUFF_SIZE - 1,0}; /*A,SP,H,L*/
ushort pc = 0;
byte memory[MEM_BUFF_SIZE];

enum
{
        LD_A, /*load from R into A*/
        LD_S, /*load from R into S*/
        LD_H, /*load from R into H*/
        LD_L, /*load from R into L*/
        LDI_A,/*load from IM into A*/
        LDI_S,/*load from IM into S*/
        LDI_L,/*load from IM into L*/
        ADI,  /*add IM*/
        SUI,  /*subtract from IM*/
        ANI,  /*and with IM*/
        ORI,  /*or with IM*/
        XRI,  /*xor with IM*/
        ADD,  /*add value at IM*/
        SUB,  /*sub value from IM*/
        AND,  /*and with value from IM*/
        ORA,  /*or with value from IM*/
        XRA,  /*xor with value from IM*/
        LDA,  /*load a from addr*/
        STA,  /*store a to addr*/
        LDH,  /*load a from [HL]*/
        STH,  /*store a to [HL]*/
        JMP,  /*jump*/
        JIC,  /*jump if condition*/
        INL,  /*increment L*/
        DEL,  /*decrement L*/
        AHI,  /*adjust H after increment (H=H+CY)*/
        AHD,  /*adjust H after decrement (H=H-CY)*/
        NOP,
        CLC, /* clear carry*/
        SEC, /* set carry */
        OUT, /* output */
        INP, /* input */
        PHA, /* psh a */
        PHH, /* psh h*/
        PHL, /* psh l */
        PLA, /* pop a */
        PLH, /* pop h */
        PLL, /* pop l */
        HLT,
};

void add(ushort b,byte *reg)
{
        ushort res = *reg + b;
        CY = (res > 255);
        ZE = (res == 0);
        *reg = res;
}

void push(byte val)
{
        memory[MEM_ADDR(regs[1]++)] = val;
}

void pop(byte *dst)
{
        *dst = memory[MEM_ADDR(--regs[1])];
}

void dump_registers()
{
        Serial.print(" A: ");  Serial.println(regs[0], HEX);
        Serial.print(" S: ");  Serial.println(regs[1], HEX);
        Serial.print(" H: ");  Serial.println(regs[2], HEX);
        Serial.print(" L: ");  Serial.println(regs[3], HEX);
        Serial.print(" PC: "); Serial.println(pc, HEX);
        Serial.print(" CY: "); Serial.println(CY);
        Serial.print(" ZE: "); Serial.println(ZE);
}

void execute(byte *memory)
{
        if (HALTED) return;
        byte inst = memory[MEM_ADDR(pc++)];
        byte op = inst & 63;
        byte arg = (inst >> 6) & 3;
        byte *srcreg = &regs[arg];
        byte imm = memory[MEM_ADDR(pc)];
        byte wrd = imm | (memory[MEM_ADDR(pc+1)] << 8);
        byte *atimm = &memory[MEM_ADDR(wrd)];
        byte *athl = &memory[MEM_ADDR((regs[2] << 8) | regs[3])];
        switch (op)
        {
                case LD_A:  regs[0]=*srcreg; break;
                case LD_S:  regs[1]=*srcreg; break;
                case LD_H:  regs[2]=*srcreg; break;
                case LD_L:  regs[3]=*srcreg; break;
                case LDI_A: regs[0]=imm; CONSUMEIM();  break;
                case LDI_S: regs[1]=imm; CONSUMEIM();  break;
                case LDI_L: regs[3]=imm; CONSUMEIM();  break;
                case ADI:   CONSUMEIM(); add(imm,&regs[0]); break;
                case SUI:   CONSUMEIM(); add(-imm,&regs[0]); break;
                case ADD:   CONSUMEIM(); CONSUMEIM(); add(*atimm,&regs[0]); break;
                case SUB:   CONSUMEIM(); CONSUMEIM(); add(-*atimm,&regs[0]); break;
                case ANI:   CONSUMEIM(); regs[0] &= imm; ZE = (regs[0] == 0); break;
                case ORI:   CONSUMEIM(); regs[0] |= imm; ZE = (regs[0] == 0); break;
                case XRI:   CONSUMEIM(); regs[0] ^= imm; ZE = (regs[0] == 0); break;
                case AND:   CONSUMEIM(); CONSUMEIM(); regs[0] &= *atimm; ZE = (regs[0] == 0); break;
                case ORA:   CONSUMEIM(); CONSUMEIM(); regs[0] |= *atimm; ZE = (regs[0] == 0); break;
                case XRA:   CONSUMEIM(); CONSUMEIM(); regs[0] ^= *atimm; ZE = (regs[0] == 0); break;
                case LDA:   CONSUMEIM(); CONSUMEIM(); regs[0] = *atimm; break;
                case STA:   CONSUMEIM(); CONSUMEIM(); *atimm = regs[0]; break;
                case LDH:   regs[0] = *athl; break;
                case STH:   *athl = regs[0]; break;
                case JMP:   CONSUMEIM(); CONSUMEIM(); pc = wrd; break;
                case JIC:   CONSUMEIM(); CONSUMEIM(); pc = (arg == 0 && ZE) ? wrd : (arg == 1 && CY) ? wrd : (arg == 2 && !ZE) ? wrd : (arg == 3 && !CY) ? wrd : pc; break;
                case INL:   add(1,&regs[3]); break;
                case DEL:   add(-1,&regs[3]); break;
                case AHI:   add(CY,&regs[2]); break;
                case AHD:   add(-CY,&regs[2]); break;
                case HLT:   HALTED = true; dump_registers(); while(1); break;
                case NOP:   break;
                case SEC:   CY=true; break;
                case CLC:   CY=false; break;
                case PHA:   push(regs[0]); break;
                case PLA:   pop(&regs[0]); break;
                case PHH:   push(regs[2]); break;
                case PLH:   pop(&regs[2]); break;
                case PHL:   push(regs[3]); break;
                case PLL:   pop(&regs[3]); break;

                case INP:   regs[0] = (Serial.available() > 0) ? Serial.read() : 0; break;
                case OUT:
                {
                        if (regs[0] == 13 || regs[0] == 10)
                        {
                                x = 0;
                                y += 24;
                        }
                        else if (regs[0] == 8) // Backspace
                        {
                                x -= 16;
                                if (x < 0 && y > 0)
                                {
                                        y -= 24;
                                        x = tft.width() - 16;
                                }
                                else if (x < 0)
                                {
                                        x = 0;
                                }
                                tft.fillRect(x, y, 16, 24, BLACK);
                        }
                        else if (regs[0] >= 32)
                        {
                                tft.setCursor(x, y);
                                tft.print((char)regs[0]);
                                Serial.print((char)regs[0]);
                                x += 16;
                        }
                        if (x >= tft.width())
                        {
                                x = 0;
                                y += 24;
                        }
                        if (y >= tft.height())
                        {
                                y = 0;
                                tft.fillScreen(BLACK);
                        }
                        break;
                }
        }
}