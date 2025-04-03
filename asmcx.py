import sys
from collections import defaultdict

# Register mapping
REGISTERS = {
    'A': 0, 'B': 1, 'C': 2, 'D': 3,
    'E': 4, 'F': 5, 'W': 6, 'Z': 7
}

MAGIC = 0x7F

# Instruction opcodes
OPCODES = {
    # Base instructions
    'HLT': 0x1F,
    'LDI': 0x00,
    'LDA': 0x01,
    'LDB': 0x02,
    'LDC': 0x03,
    'LDD': 0x04,
    'LDSPH': 0x05,
    'LDSPL': 0x06,
    'LDR': 0x07,
    'STR': 0x08,
    'ADD': 0x09,
    'SUB': 0x0A,
    'ADC': 0x0B,
    'SBB': 0x0C,
    'AND': 0x0D,
    'OR': 0x0E,
    'XOR': 0x0F,
    'ROL': 0x10,
    'ROR': 0x11,
    'LDR_C': 0x12,
    'STR_C': 0x13,
    'JMP': 0x14,
    'JC': 0x15,
    'JZ': 0x16,
    'JNC': 0x17,
    'JNZ': 0x18,
    'PUSH': 0x19,
    'POP': 0x1A,
    
    # 0x1C prefix instructions
    'LXI': (0x1C, 0x00),
    'LXAB': (0x1C, 0x01),
    'LXCD': (0x1C, 0x02),
    'LXEF': (0x1C, 0x03),
    'LXWZ': (0x1C, 0x04),
    'SPCD': (0x1C, 0x05),
    'SPAB': (0x1C, 0x06),
    'SPEF': (0x1C, 0x07),
    'SPWZ': (0x1C, 0x08),
    'PCCD': (0x1C, 0x09),
    'PCAB': (0x1C, 0x0A),
    'PCEF': (0x1C, 0x0B),
    'PCWZ': (0x1C, 0x0C),
    'OUT': (0x1C, 0x0D),
    'INP': (0x1C, 0x0E),
    'GMASK': (0x1C, 0x0F),
    'SMASK': (0x1C, 0x10),
    'CALL': (0x1C, 0x11),
    'IRET': (0x1C, 0x12),
    'RET': (0x1C, 0x13),
    
    # 0x1D prefix instructions
    'INC': (0x1D, 0x00),
    'DEC': (0x1D, 0x01),
    'TEST': (0x1D, 0x02),
}

def construct(opcode, dst):
    return (opcode & 31) | (dst << 5)

def parse_operand(op):
    # Try to parse as register
    if op.upper() in REGISTERS:
        return ('reg', REGISTERS[op.upper()])
    
    # Try to parse as hex immediate
    if op.startswith('0x'):
        try:
            return ('imm', int(op[2:], 16))
        except ValueError:
            pass
    
    # Try to parse as decimal immediate
    try:
        return ('imm', int(op))
    except ValueError:
        pass
    
    # Must be a label reference
    return ('label', op)

def assemble_line(line, labels, pc):
    line = line.split(';')[0].strip()  # Remove comments
    if not line:
        return []
    
    parts = line.split()
    mnemonic = parts[0].upper()
    operands = [x.strip() for x in ','.join(parts[1:]).split(',')] if len(parts) > 1 else []
    
    # Handle label definition
    if mnemonic.endswith(':'):
        return []
    
    # Handle DB directive
    if mnemonic == 'DB':
        values = []
        for op in operands:
            if op.startswith('0x'):
                values.append(int(op[2:], 16))
            elif op.startswith("'") and op.endswith("'"):
                values.append(ord(op[1:-1]))
            else:
                values.append(int(op))
        return values
    
    # Handle DW directive
    if mnemonic == 'DW':
        values = []
        for op in operands:
            if op.startswith('0x'):
                val = int(op[2:], 16)
            else:
                val = int(op)
            values.append((val >> 8) & 0xFF)
            values.append(val & 0xFF)
        return values
    
    # Handle ORG directive
    if mnemonic == 'ORG':
        return []
    
    # Process regular instructions
    opcode = OPCODES.get(mnemonic)
    if opcode is None:
        raise ValueError(f"Unknown instruction: {mnemonic}")
    
    output = []
    
    # Handle prefix instructions
    if isinstance(opcode, tuple):
        prefix, sub_opcode = opcode
        
        # Output the prefix instruction
        if len(operands) > 0:
            dst_reg = parse_operand(operands[0])
            if dst_reg[0] != 'reg':
                raise ValueError(f"Expected register for {mnemonic}, got {operands[0]}")
            output.append(construct(prefix, dst_reg[1]))
        else:
            output.append(construct(prefix, 0))
        
        # Output the sub-opcode
        output.append(sub_opcode)
        
        # Handle remaining operands
        for op in operands[1:]:
            operand = parse_operand(op)
            if operand[0] == 'imm':
                output.append(operand[1] & 0xFF)
            elif operand[0] == 'label':
                # We'll resolve labels in a second pass
                output.append(('label', operand[1]))
            else:
                raise ValueError(f"Unexpected operand type {operand[0]} for {mnemonic}")
        
        # Special handling 16-bit immediate
        if (mnemonic == 'LXI') and len(operands) > 1:
            operand = parse_operand(operands[1])
            if operand[0] == 'imm':
                output.append((operand[1] >> 8) & 0xFF)
                output.append(operand[1] & 0xFF)
            elif operand[0] == 'label':
                output.append(('label_hi', operand[1]))
                output.append(('label_lo', operand[1]))
    else:
        # Handle regular instructions
        if len(operands) > 0:
            dst_reg = parse_operand(operands[0])
            if dst_reg[0] == 'reg':
                output.append(construct(opcode, dst_reg[1]))
            elif dst_reg[0] == 'imm':
                output.append(construct(opcode, 0))
                if mnemonic == 'JMP' or mnemonic == 'JZ' or mnemonic == 'JC' or mnemonic == 'JNZ' or mnemonic == 'JNC' or mnemonic == 'CALL':
                        output.append(dst_reg[1]>>8)
                        output.append(dst_reg[1]&255)
                else:
                        output.append(dst_reg[1])
        else:
            output.append(construct(opcode, 0))
        
        # Handle remaining operands
        for op in operands[1:]:
            operand = parse_operand(op)
            if operand[0] == 'imm':
                output.append(operand[1] & 0xFF)
            elif operand[0] == 'label':
                # We'll resolve labels in a second pass
                output.append(('label', operand[1]))
            else:
                raise ValueError(f"Unexpected operand type {operand[0]} for {mnemonic}")
    return output

def assemble(source):
    # First pass: collect labels
    labels = {}
    pc = 0
    lines = source.split('\n')
    
    for line in lines:
        line = line.split(';')[0].strip()  # Remove comments
        if not line:
            continue
        
        # Handle label definition
        if ':' in line:
            label = line.split(':')[0].strip()
            labels[label] = pc
            line = line.split(':')[1].strip()
            if not line:
                continue
        
        # Handle ORG directive
        parts = line.split()
        if parts and parts[0].upper() == 'ORG':
            pc = int(parts[1], 0)
            continue
        
        # Handle DB/DW directives
        if parts and parts[0].upper() in ('DB', 'DW'):
            if parts[0].upper() == 'DB':
                pc += len(parts) - 1
            else:  # DW
                pc += (len(parts) - 1) * 2
            continue
        
        # Count bytes for regular instructions
        opcode = OPCODES.get(parts[0].upper()) if parts else None
        if opcode is None:
            continue
        
        if isinstance(opcode, tuple):
            pc += 2  # Prefix + sub-opcode
            if parts[0].upper() == 'LXI' and len(parts) > 2:
                pc += 2  # 16-bit immediate
            elif len(parts) > 1:
                pc += len(parts) - 2  # Additional operands
        else:
            pc += 1  # Base instruction
            if len(parts) > 1:
                pc += len(parts) - 1  # Additional operands
    
    # Second pass: generate machine code
    pc = 0
    output = []
    unresolved = []
    
    for line in lines:
        line = line.split(';')[0].strip()  # Remove comments
        if not line:
            continue
        
        # Handle label definition
        if ':' in line:
            label = line.split(':')[0].strip()
            line = line.split(':')[1].strip()
            if not line:
                continue
        
        # Handle ORG directive
        parts = line.split()
        if parts and parts[0].upper() == 'ORG':
            new_pc = int(parts[1], 0)
            if new_pc > len(output):
                output += [0] * (new_pc - len(output))
            pc = new_pc
            continue
        
        # Generate machine code
        bytes_to_add = assemble_line(line, labels, pc)
        
        # Replace label references
        for i, byte in enumerate(bytes_to_add):
            if isinstance(byte, tuple):
                ref_type, label = byte
                if label not in labels:
                    raise ValueError(f"Undefined label: '{label}'")
                
                addr = labels[label]
                if ref_type == 'label':
                    bytes_to_add[i] = addr & 0xFF
                elif ref_type == 'label_hi':
                    bytes_to_add[i] = (addr >> 8) & 0xFF
                elif ref_type == 'label_lo':
                    bytes_to_add[i] = addr & 0xFF
        
        # Add to output
        if pc < len(output):
            # Overwrite existing bytes if ORG moved us back
            for i, byte in enumerate(bytes_to_add):
                output[pc + i] = byte
        else:
            output += bytes_to_add
        pc += len(bytes_to_add)
    
    return output

def main():
    if len(sys.argv) != 2:
        print("Usage: assembler.py <input.asm>")
        return
    
    with open(sys.argv[1], 'r') as f:
        source = f.read()
    
    try:
        machine_code = assemble(source)
        
        # Output in hex format
        print("Machine code:")
        print(f"{MAGIC:02X}",end="")
        print(f"{len(machine_code) >> 8:02X}",end="")
        print(f"{len(machine_code) & 255:02X}",end="")
        for i, byte in enumerate(machine_code):
            print(f"{byte:02X}",end="")
        print("")
        x = open("output.cx","wb")
        x.write(bytearray([MAGIC,len(machine_code) >> 8,(len(machine_code) & 255),0,0]+machine_code+[0x00,0x00]))
        x.close()
    except ValueError as e:
        print(f"Assembly error: {e}")

if __name__ == "__main__":
    main()