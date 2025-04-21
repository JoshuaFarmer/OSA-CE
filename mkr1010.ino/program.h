#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#define MAGIC 0x7F

void get_program_via_serial()
{
        // Wait for magic byte
        while (true)
        {
                if (Serial.available() > 0)
                {
                        byte start = Serial.read();
                        if (start == MAGIC)
                        {
                                break;
                        }
                }
        }

        // Read size (16-bit, big-endian)
        ushort size = 0;
        while (Serial.available() < 2)
                ; // Wait for both bytes of size
        size = Serial.read() << 8;
        size |= Serial.read();

        // Limit size to available memory
        if (size > MEM_BUFF_SIZE)
        {
                size = MEM_BUFF_SIZE;
        }

        // Read program data
        for (int i = 0; i < size; ++i)
        {
                while (Serial.available() <= 0)
                        ; // Wait for each byte
                memory[i] = Serial.read();
        }

        Serial.println("Program loaded successfully");
        Serial.print("Size: ");
        Serial.println(size);
}
