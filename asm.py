from sys import argv

# Instruction to opcode mapping
opcodes = {
    'TAA': 0x00,
    'TAS': 0x01,
    'TAH': 0x02,
    'TAL': 0x03,
    'TSA': 0x40,
    'TSS': 0x41,
    'TSH': 0x42,
    'TSL': 0x43,
    'THA': 0x80,
    'THS': 0x81,
    'THH': 0x82,
    'THL': 0x83,
    'TLA': 0xC0,
    'TLS': 0xC1,
    'TLH': 0xC2,
    'TLL': 0xC3,
    'LIA': 0x04,
    'LIS': 0x05,
    'LIL': 0x06,
    'ADI': 0x07,
    'SUI': 0x08,
    'ANI': 0x09,
    'ORI': 0x0A,
    'XRI': 0x0B,
    'ADD': 0x0C,
    'SUB': 0x0D,
    'AND': 0x0E,
    'ORA': 0x0F,
    'XRA': 0x10,
    'LDA': 0x11,
    'STA': 0x12,
    'LDH': 0x13,
    'STH': 0x14,
    'JMP': 0x15,
    'JZ':  0x16,
    'JC' : 0x56,
    'JNZ': 0x96,
    'JNC': 0xD6,
    'INL': 0x17,
    'DEL': 0x18,
    'AHI': 0x19,
    'AHD': 0x1A,
    'NOP': 0x1B,
    'CLC': 0x1C,
    'SEC': 0x1D,
    'OUT': 0x1E,
    'IN':  0x1F,
    'PHA': 0x20,
    'PHH': 0x21,
    'PHL': 0x22,
    'PLA': 0x23,
    'PLH': 0x24,
    'PLL': 0x25,
    'HLT': 0x26
}

if len(argv) != 2:
    exit(1)

with open(argv[1], "r") as x:
    data = x.read()

# Process each line
output = []
labels = {}
current_address = 0

# First pass: collect labels
for line in data.split('\n'):
    line = line.split(';')[0].strip()  # Remove comments and whitespace
    if not line:
        continue
        
    if ':' in line:
        # This is a label definition
        label = line.split(':')[0].strip()
        labels[label] = current_address
    else:
        # This is an instruction
        parts = line.split()
        if parts:
            current_address += 1
            if parts[0] in ['.BYTE']:
                continue
            elif parts[0] in ['.STR']:
                string = ' '.join(parts[1:])
                current_address += len(string[1:-1])-1
                continue
            elif parts[0] in ['LIA', 'LIS', 'LIL', 'ADI', 'SUI', 'ANI', 'ORI', 'XRI']:
                current_address += 1
            elif parts[0] in ['ADD', 'SUB', 'AND', 'ORA', 'XRA', 'LDA', 'STA', 'JMP', 'JZ', 'JC', 'JNZ', 'JNC']:
                current_address += 2

# Second pass: generate code
current_address = 0
for line in data.split('\n'):
    line = line.split(';')[0].strip()  # Remove comments and whitespace
    if not line or ':' in line:
        continue  # Skip empty lines and label definitions (already processed)
        
    parts = line.split()
    if not parts:
        continue
        
    mnemonic = parts[0].upper()
    operand = None
    
    if mnemonic in ['.BYTE']:
        if len(parts) < 2:
            print(f"Error: Missing operand for {mnemonic} at line {current_address}")
            exit(1)
        try:
            operand = int(parts[1], 0)  # Accepts hex (0x), binary (0b), or decimal
            if operand < 0 or operand > 255:
                print(f"Error: Operand out of range (0-255) at line {current_address}")
                exit(1)
            output.append(operand)
            current_address += 1
        except ValueError:
            print(f"Error: Invalid operand for {mnemonic} at line {current_address}")
            exit(1)
    elif mnemonic in ['.STR']:
            string = ' '.join(parts[1:])
            current_address += len(string[1:-1])
            for i in string[1:-1]:
                output.append(ord(i))
    elif mnemonic in opcodes:
        output.append(opcodes[mnemonic])
        current_address += 1
        
        # short immediates
        if mnemonic in ['LIA', 'LIS', 'LIL', 'ADI', 'SUI', 'ANI', 'ORI', 'XRI']:
            if len(parts) < 2:
                print(f"Error: Missing operand for {mnemonic} at line {current_address}")
                exit(1)
            operand = parts[1]
            if operand in labels:
                output.append(labels[operand] & 255)
                current_address += 1
            else:
                try:
                    operand = int(parts[1], 0)  # Accepts hex (0x), binary (0b), or decimal
                    if operand < 0 or operand > 255:
                        print(f"Error: Operand out of range (0-255) at line {current_address}")
                        exit(1)
                    output.append(operand)
                    current_address += 1
                except ValueError:
                    print(f"Error: Invalid operand for {mnemonic} at line {current_address}")
                    exit(1)
        # wide immediates
        elif mnemonic in ['JMP', 'JZ', 'JC', 'JNZ', 'JNC', 'ADD', 'SUB', 'AND', 'ORA', 'XRA', 'LDA', 'STA']:
            if len(parts) < 2:
                print(f"Error: Missing operand for {mnemonic} at line {current_address}")
                exit(1)
            operand = parts[1]
            if operand in labels:
                output.append(labels[operand] & 255)
                output.append(labels[operand] >> 8)
                current_address += 2
            else:
                try:
                    operand = int(parts[1], 0)
                    if operand < 0 or operand > 65535:
                        print(f"Error: Address out of range (0-65535) at line {current_address}")
                        exit(1)
                    output.append(operand & 255)
                    output.append(operand >> 8)
                    current_address += 2
                except ValueError:
                    print(f"Error: Invalid address/label for {mnemonic} at line {current_address}")
                    exit(1)
    else:
        print(f"Error: Unknown instruction {mnemonic} at line {current_address}")
        exit(1)

# Print the output in hex format
print("Generated machine code:")
for i, byte in enumerate(output):
    print(f"{i:03X}: {byte:02X}")
with open("output.cx","wb") as f:
        f.write(bytearray(output))
        f.close()