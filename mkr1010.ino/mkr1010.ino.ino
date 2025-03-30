/* simple emulator for programs */
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library

#define MEM_ADDR(addr) ((addr) & (MEM_SIZE - 1))
#define MEM_SIZE 512
#define CONSUME_IMMEDIATE_8() pc += 1;
#define CONSUME_IMMEDIATE_16() pc += 2;

typedef unsigned char byte;
typedef unsigned short ushort;

byte regs[8]; /*A,B,C,D,E,F,W,Z*/
ushort pc = 0;
ushort sp = MEM_SIZE - 1;
bool CY = 0, ZE = 0, HALTED = 0;
const byte INT_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9};
byte interrupts_mask;
byte interrupt_vector = 0;

byte *MEM = NULL;
byte memory[MEM_SIZE];

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

inline byte construct(byte opcode, byte dst)
{
        return (opcode & 31) | (dst << 5);
}

int x = 0;
int y = 0;

#define LCD_CS A3    // Chip Select goes to Analog 3
#define LCD_CD A2    // Command/Data goes to Analog 2
#define LCD_WR A1    // LCD Write goes to Analog 1
#define LCD_RD A0    // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup_lcd(void)
{
        Serial.print("TFT size is ");
        Serial.print(tft.width());
        Serial.print("x");
        Serial.println(tft.height());
        tft.reset();
        uint16_t identifier = tft.readID();
        if (identifier == 0x9325)
        {
                Serial.println(F("Found ILI9325 LCD driver"));
        }
        else if (identifier == 0x9328)
        {
                Serial.println(F("Found ILI9328 LCD driver"));
        }
        else if (identifier == 0x4535)
        {
                Serial.println(F("Found LGDP4535 LCD driver"));
        }
        else if (identifier == 0x7575)
        {
                Serial.println(F("Found HX8347G LCD driver"));
        }
        else if (identifier == 0x9341)
        {
                Serial.println(F("Found ILI9341 LCD driver"));
        }
        else if (identifier == 0x8357)
        {
                Serial.println(F("Found HX8357D LCD driver"));
        }
        else if (identifier == 0x0101)
        {
                identifier = 0x9341;
                Serial.println(F("Found 0x9341 LCD driver"));
        }
        else if (identifier == 0x1111)
        {
                identifier = 0x9328;
                Serial.println(F("Found 0x9328 LCD driver"));
        }
        else
        {
                Serial.print(F("Unknown LCD driver chip: "));
                Serial.println(identifier, HEX);
                Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
                Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
                Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
                Serial.println(F("If using the breakout board, it should NOT be #defined!"));
                Serial.println(F("Also if using the breakout, double-check that all wiring"));
                Serial.println(F("matches the tutorial."));
                identifier = 0x9328;
        }
        tft.begin(identifier);
}

void execute(byte *memory)
{
        byte inst = memory[MEM_ADDR(pc++)];
        byte opcode = inst & 31;
        byte short_arg = inst >> 5; /* 3 bit argument */
        ushort ab = (regs[0] << 8) | regs[1];
        ushort cd = (regs[2] << 8) | regs[3];
        ushort ef = (regs[4] << 8) | regs[5];
        ushort wz = (regs[6] << 8) | regs[7];
        byte *deref_cd = &memory[cd];
        byte imm8 = memory[MEM_ADDR(pc)];
        ushort imm16 = (imm8 << 8) | memory[MEM_ADDR(pc + 1)];
        ushort extimm16 = (memory[MEM_ADDR(pc + 1)] << 8) | memory[MEM_ADDR(pc + 2) & (MEM_SIZE - 1)];
        byte *arg_reg = &regs[short_arg];
        switch (opcode)
        {
        case INST_HLT:
                HALTED = true;
                Serial.println("Emulator Halted");
                dump_registers();
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
                *arg_reg = memory[imm16];
                CONSUME_IMMEDIATE_16();
                break;
        case INST_STR:
                memory[imm16] = *arg_reg;
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
                CY = res > 0xFF;
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
                pc = imm16;
                break;
        case INST_JC:
                if (CY)
                        pc = imm16;
                else
                        CONSUME_IMMEDIATE_16();
                break;
        case INST_JZ:
                if (ZE)
                        pc = imm16;
                else
                        CONSUME_IMMEDIATE_16();
                break;

        case INST_JNC:
                if (!CY)
                        pc = imm16;
                else
                        CONSUME_IMMEDIATE_16();
                break;

        case INST_JNZ:
                if (!ZE)
                        pc = imm16;
                else
                        CONSUME_IMMEDIATE_16();
                break;

        case INST_PUSH_R:
                memory[sp--] = *arg_reg;
                break;

        case INST_POP_R:
                *arg_reg = memory[++sp];
                break;
        case PREFIX_1C:
        {
                CONSUME_IMMEDIATE_8();
                switch (imm8)
                {
                case INST_CALL:
                        memory[sp--] = pc;
                        pc = imm16;
                        CONSUME_IMMEDIATE_16();
                        break;
                case INST_RET:
                        pc = memory[++sp];
                        break;
                case INST_IRET:
                        interrupts_mask = memory[++sp];
                        CY = memory[++sp];
                        ZE = memory[++sp];
                        pc = memory[++sp];
                        break;
                case INST_GMASK:
                {
                        regs[0] = interrupts_mask;
                        break;
                }
                case INST_SMASK:
                {
                        interrupts_mask = regs[0];
                        break;
                }
                case INST_LXAB:
                {
                        short_arg = short_arg & 3;
                        regs[short_arg * 2] = regs[0];
                        regs[short_arg * 2 + 1] = regs[1];
                        break;
                }
                case INST_LXCD:
                {
                        short_arg = short_arg & 3;
                        regs[short_arg * 2] = regs[2];
                        regs[short_arg * 2 + 1] = regs[3];
                        break;
                }
                case INST_LXEF:
                {
                        short_arg = short_arg & 3;
                        regs[short_arg * 2] = regs[4];
                        regs[short_arg * 2 + 1] = regs[5];
                        break;
                }
                case INST_LXWZ:
                {
                        short_arg = short_arg & 3;
                        regs[short_arg * 2] = regs[6];
                        regs[short_arg * 2 + 1] = regs[7];
                        break;
                }
                case INST_LXI:
                {
                        CONSUME_IMMEDIATE_16();
                        short_arg = short_arg & 3;
                        regs[short_arg * 2] = extimm16 >> 8;
                        regs[short_arg * 2 + 1] = extimm16;
                        break;
                }
                case INST_OUT_R:
                {
                        if (regs[7] == 0)
                        {
                                Serial.print((char)*arg_reg);
                        }
                        else if (regs[7] == 1)
                        {
                                x = regs[7];
                        }
                        else if (regs[7] == 2)
                        {
                                y = regs;
                        }
                        else if (regs[7] == 3 && (char)*arg_reg)
                        {
                                if (*arg_reg == 13 || *arg_reg == 10)
                                {
                                        x = 0;
                                        y += 24;
                                }
                                else if (*arg_reg == 8) // Backspace
                                {
                                        x -= 16;
                                        if (x < 0)
                                        {
                                                if (y > 0)
                                                {
                                                        y -= 24;
                                                        x = tft.width() - 16; // Move to end of previous line
                                                }
                                                else
                                                {
                                                        x = 0; // Can't go above top-left
                                                }
                                        }
                                        // Clear the character position
                                        tft.fillRect(x, y, 16, 24, BLACK);
                                }
                                else if (*arg_reg >= 32) // Printable characters only
                                {
                                        tft.setCursor(x, y);
                                        tft.print((char)*arg_reg);
                                        x += 16;
                                }
                                if (x >= tft.width())
                                {
                                        x = 0;
                                        y += 24;
                                        if (y >= tft.height())
                                        {
                                                y = 0;
                                        }
                                }
                        }
                        break;
                }
                case INST_INP_R:
                {
                        if (Serial.available() > 0)
                        {
                                *arg_reg = Serial.read();
                        }
                        else if (Serial.available() <= 0)
                        {
                                *arg_reg = 0;
                        }
                        break;
                }
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
                case INST_SPEF:
                        sp = cd;
                        break;
                case INST_PCEF:
                        pc = cd;
                        break;
                case INST_SPWZ:
                        sp = ab;
                        break;
                case INST_PCWZ:
                        pc = ab;
                        break;
                }
                break;
        }

        case PREFIX_1D:
        {
                CONSUME_IMMEDIATE_8();
                switch (imm8)
                {
                case INST_INC:
                {
                        ushort res = *arg_reg + 1;
                        CY = res > 0xFF;
                        regs[0] = res;
                        ZE = regs[0] == 0;
                        *arg_reg = res;
                        break;
                }
                case INST_DEC:
                {
                        ushort res = *arg_reg - 1;
                        CY = res > 0xFF;
                        regs[0] = res;
                        ZE = regs[0] == 0;
                        *arg_reg = res;
                        break;
                }
                case INST_TEST:
                {
                        ZE = (regs[0] & *arg_reg) == 0;
                        break;
                }
                }
                break;
        }

        default:
                Serial.print("Invalid opcode: 0x");
                Serial.println(opcode, HEX);
        }

        pc = pc & (MEM_SIZE - 1);
}

void dump_registers()
{
        ushort ab = (regs[0] << 8) | regs[1];
        ushort cd = (regs[2] << 8) | regs[3];
        ushort ef = (regs[4] << 8) | regs[5];
        ushort wz = (regs[6] << 8) | regs[7];
        Serial.print("AB=");
        Serial.println(ab, HEX);
        Serial.print("CD=");
        Serial.println(cd, HEX);
        Serial.print("EF=");
        Serial.println(ef, HEX);
        Serial.print("WZ=");
        Serial.println(wz, HEX);
        Serial.print("PC=");
        Serial.println(pc, HEX);
        Serial.print("SP=");
        Serial.println(sp, HEX);
}

void save()
{
        if (!MEM)
                return;
        MEM[sp--] = pc;
        MEM[sp--] = ZE;
        MEM[sp--] = CY;
        MEM[sp--] = interrupts_mask;
}

void interruptHandler0()
{
        if (interrupt_vector & 0b00000001)
        {
                save();
                pc = 0 * 8;
        }
}
void interruptHandler1()
{
        if (interrupt_vector & 0b00000010)
        {
                save();
                pc = 1 * 8;
        }
}
void interruptHandler2()
{
        if (interrupt_vector & 0b00000100)
        {
                save();
                pc = 2 * 8;
        }
}
void interruptHandler3()
{
        if (interrupt_vector & 0b00001000)
        {
                save();
                pc = 3 * 8;
        }
}
void interruptHandler4()
{
        if (interrupt_vector & 0b00010000)
        {
                save();
                pc = 4 * 8;
        }
}
void interruptHandler5()
{
        if (interrupt_vector & 0b00100000)
        {
                save();
                pc = 5 * 8;
        }
}
void interruptHandler6()
{
        if (interrupt_vector & 0b01000000)
        {
                save();
                pc = 6 * 8;
        }
}
void interruptHandler7()
{
        if (interrupt_vector & 0b10000000)
        {
                save();
                pc = 7 * 8;
        }
}

///*tft.fillScreen(BLACK);*/
// tft.setCursor(0, 0);
// tft.setTextColor(RED);
// tft.setTextSize(1);
// tft.println("Hello World!");

void setup()
{
        Serial.begin(9600);
        setup_lcd();
        tft.fillScreen(BLACK);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        /*LDI A,1*/
        /*LDI B,3*/
        /*ADD B*/

        MEM = memory;
        memory[0] = construct(INST_LDI, 7);
        memory[1] = 3; /* tty */
        memory[2] = construct(PREFIX_1C, 0);
        memory[3] = construct(INST_INP_R, 0);
        memory[4] = construct(PREFIX_1D, 0);
        memory[5] = construct(INST_TEST, 0);
        memory[6] = construct(INST_JZ, 0);
        memory[7] = 0;
        memory[8] = 2;
        memory[9] = construct(PREFIX_1C, 0);
        memory[10] = construct(INST_OUT_R, 0);
        memory[11] = construct(INST_JMP, 0);
        memory[12] = 0;
        memory[13] = 2;
}

void loop()
{
        if (!HALTED)
        {
                execute(memory);
        }

        delay(1);
}
