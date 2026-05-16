#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define RETURN_INIT() int error_code = 0
#define RETURN_SUCCESS() goto end
#define RETURN_ERROR(err_code, message) printf("Error|" message "\n"); error_code = 1; goto end
#define RETURN(clean_block) end: clean_block return error_code

typedef enum 
{
    OpCodeType_MOV_REG = 0b00100010
} OpCodeType;

typedef enum
{
    RegType_AX = 0b00000000,
    RegType_CX = 0b00000001,
    RegType_DX = 0b00000010,
    RegType_BX = 0b00000011,
    RegType_SP = 0b00000100,
    RegType_BP = 0b00000101,
    RegType_SI = 0b00000110,
    RegType_DI = 0b00000111
} RegType;

const char* RegToString(RegType reg, uint8_t isWordData)
{
    switch(reg)
    {
        case RegType_AX: return isWordData ? "AX" : "AL";
        case RegType_CX: return isWordData ? "CX" : "CL";
        case RegType_DX: return isWordData ? "DX" : "DL";
        case RegType_BX: return isWordData ? "BX" : "BL";
        case RegType_SP: return isWordData ? "SP" : "AH";
        case RegType_BP: return isWordData ? "BP" : "CH";
        case RegType_SI: return isWordData ? "SI" : "DH";
        case RegType_DI: return isWordData ? "DI" : "BH";
        default: return "";
    }
}

int main(int argsCount, const char** args)
{
    RETURN_INIT();

    if (argsCount != 3)
    {
        RETURN_ERROR(1, "There should be exactly two arguments: name of the binary assembled with nasm, for disassembly and output binary name.");
    }

    uint8_t supportedOpCodes[256] = {0};
    supportedOpCodes[OpCodeType_MOV_REG] = 1;

    const char* inputFileName = args[1];
    const char* outputFileName = args[2];

    FILE* inputFile = fopen(inputFileName, "rb");
    if (!inputFile) 
    {
        RETURN_ERROR(1, "No binary file found.");
    }

    FILE* outputFile = fopen(outputFileName, "w");
    if (!outputFile) 
    {
        RETURN_ERROR(1, "Couldn't create output asm file.");
    }

    uint8_t buffer[2048] = {0};
    size_t readCount = 0;
    while (readCount = fread(buffer, sizeof(uint8_t), 2048, inputFile))
    {
        size_t i = 0;
        while (i < readCount)
        {
            uint8_t opCodeByte = buffer[i++];

            uint8_t opCodeType         = (0b11111100 & opCodeByte) >> 2;
            uint8_t isDestinationInReg = (0b00000010 & opCodeByte) >> 1;
            uint8_t isWordData         = (0b00000001 & opCodeByte) >> 0;

            if (supportedOpCodes[opCodeType])
            {
                switch((OpCodeType)opCodeType)
                {
                    case OpCodeType_MOV_REG:
                    {                            
                        uint8_t dataByte = buffer[i++];

                        uint8_t mod = (dataByte & 0b11000000) >> 6;
                        uint8_t reg = (dataByte & 0b00111000) >> 3;
                        uint8_t rm  = (dataByte & 0b00000111) >> 0;

                        assert(mod == 0b11);
                        
                        if (isDestinationInReg)
                        {
                            fprintf(outputFile, "MOV %s, %s\n", RegToString(reg, isWordData), RegToString(rm, isWordData));
                        }
                        else
                        {
                            fprintf(outputFile, "MOV %s, %s\n", RegToString(rm, isWordData), RegToString(reg, isWordData));
                        }
                        break;
                    }
                    default:
                        assert(0);
                        break;
                }
            }
            else
            {
                RETURN_ERROR(1, "Encountered unknown instruction.");
            }
        }
    }

    RETURN({
        if (inputFile)  fclose(inputFile);
        if (outputFile) fclose(outputFile);
    });
}