#include <Elegoo_GFX.h>
#include <Elegoo_TFTLCD.h>
#include <SPI.h>
#include <SD.h>

typedef unsigned char byte;
typedef unsigned short ushort;

#include "video.h"
#include "emu.h"
#include "program.h"

void setup()
{
        File myFile;
        Serial.begin(9600);
        pinMode(10, OUTPUT);
        if (!SD.begin(10))
        {
                Serial.println("initialization failed!");
                while (1)
                {}
        }

        pc = 0;
        if (!SD.exists("KRNL.CX"))
        {
                get_program_via_serial();
        }
        else
        {
                myFile = SD.open("KRNL.CX", FILE_READ);
                myFile.read(memory,MEM_BUFF_SIZE);
                myFile.close();
        }

        setup_lcd();
        tft.fillScreen(BLACK);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
}

void loop()
{
        if (!HALTED)
        {
                execute(memory);
        }
        delay(1);
}
