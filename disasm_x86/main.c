#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#define RETURN_INIT() int error_code = 0
#define RETURN_SUCCESS() goto end
#define RETURN_ERROR(err_code, message) printf("Error|" message "\n"); error_code = err_code; goto end
#define RETURN(clean_block) end: clean_block return error_code

typedef enum 
{
    OpCodeType_INV        = 0b00000000,
    OpCodeType_MOV_RM_REG = 0b00100010,
    OpCodeType_MOV_IM__RM = 0b01100011,
    OpCodeType_MOV_IM_REG = 0b00001011,
    OpCodeType_MOV_MEM_AC = 0b01010000,
    OpCodeType_MOV_AC_MEM = 0b01010001,
    OpCodeType_MOV_RM_SEG = 0b10001110,
    OpCodeType_MOV_SEG_RM = 0b10001100
} OpCodeType;

typedef enum
{
    ModType_MemoryMode                  = 0b00,
    ModType_MemoryModeDisplacement8Bit  = 0b01,
    ModType_MemoryModeDisplacement16Bit = 0b10,
    ModType_RegisterMode                = 0b11
} ModType;

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

uint16_t ReadData(uint8_t* buffer, size_t* i, uint8_t isWordData)
{
    uint8_t lo = buffer[(*i)++];
    uint8_t hi = isWordData ? buffer[(*i)++] : 0;
    uint16_t result = 0;
    return ((result | (uint16_t)hi) << 8) | (uint16_t)lo;
}

#define RMSTR_MAX 32

typedef struct {
    char rmstr[RMSTR_MAX];
    uint8_t* buffer;
    size_t* i;
} RmToStringContext;

void PrintNoDisplacement(const char* fmt, RmToStringContext* ctx)
{
    uint16_t val = ReadData(ctx->buffer, ctx->i, 1);
    snprintf(ctx->rmstr, 32, fmt, val);
}

void Print8bitDisplacement(const char* fmt, RmToStringContext* ctx)
{
    int8_t data = (int8_t)ctx->buffer[(*ctx->i)++];
    snprintf(ctx->rmstr, 32, fmt, data > 0 ? '+' : '-', abs(data));
}

void Print16bitDisplacement(const char* fmt, RmToStringContext* ctx)
{
    uint16_t val = ReadData(ctx->buffer, ctx->i, 1);
    snprintf(ctx->rmstr, 32, fmt, val);
}

const char* RmToString(uint8_t rm, ModType mod, RmToStringContext* ctx)
{
    const char* result[8] = {0};

    result[0b00000000] = "[BX + SI";
    result[0b00000001] = "[BX + DI";
    result[0b00000010] = "[BP + SI";
    result[0b00000011] = "[BP + DI";
    result[0b00000100] = "[SI";
    result[0b00000101] = "[DI";
    result[0b00000110] = "[BP";
    result[0b00000111] = "[BX";

    char fmt[RMSTR_MAX] = {0};

    switch(mod)
    {
        case ModType_MemoryMode:
        {
            if (rm == 0b00000110)
            {
                snprintf(fmt, RMSTR_MAX, "[%%u]");
                PrintNoDisplacement(fmt, ctx);
            }
            else
            {
                snprintf(ctx->rmstr, RMSTR_MAX, "%s]", result[rm]);
            }
            break;
        }
        case ModType_MemoryModeDisplacement8Bit:
        {
            snprintf(fmt, RMSTR_MAX, "%s %%c %%d]", result[rm]);
            Print8bitDisplacement(fmt, ctx); break;
            break;
        }
        case ModType_MemoryModeDisplacement16Bit:
        {
            snprintf(fmt, RMSTR_MAX, "%s + %%u]", result[rm]);
            Print16bitDisplacement(fmt, ctx);
            break;
        }
        default:
        {
            assert(0); 
            break;
        }
    }

    return ctx->rmstr;
}

OpCodeType GetOpCodeType(uint8_t opCodeByte)
{
    if ((opCodeByte >> 2) == OpCodeType_MOV_RM_REG) return OpCodeType_MOV_RM_REG;
    if ((opCodeByte >> 1) == OpCodeType_MOV_IM__RM) return OpCodeType_MOV_IM__RM;
    if ((opCodeByte >> 4) == OpCodeType_MOV_IM_REG) return OpCodeType_MOV_IM_REG;
    if ((opCodeByte >> 1) == OpCodeType_MOV_MEM_AC) return OpCodeType_MOV_MEM_AC;
    if ((opCodeByte >> 1) == OpCodeType_MOV_AC_MEM) return OpCodeType_MOV_AC_MEM;
    if ((opCodeByte >> 0) == OpCodeType_MOV_RM_SEG) return OpCodeType_MOV_RM_SEG;
    if ((opCodeByte >> 0) == OpCodeType_MOV_SEG_RM) return OpCodeType_MOV_SEG_RM;

    return OpCodeType_INV;
}

int main(int argsCount, const char** args)
{
    RETURN_INIT();

    if (argsCount != 3)
    {
        RETURN_ERROR(1, "There should be exactly two arguments: name of the binary assembled with nasm, for disassembly and output binary name.");
    }

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

    fprintf(outputFile, "bits 16\n");

    uint8_t buffer[2048] = {0};
    size_t readCount = 0;
    while (readCount = fread(buffer, sizeof(uint8_t), 2048, inputFile))
    {
        size_t i = 0;
        while (i < readCount)
        {
            uint8_t opCodeByte = buffer[i++];

            OpCodeType opCodeType = GetOpCodeType(opCodeByte);

            if (opCodeType == OpCodeType_INV)
            {
                RETURN_ERROR(1, "Encountered unknown instruction.");
            }

            switch(opCodeType)
            {
                case OpCodeType_MOV_RM_REG:
                {
                    uint8_t isDestinationInReg = (0b00000010 & opCodeByte) >> 1;
                    uint8_t isWordData         = (0b00000001 & opCodeByte) >> 0;

                    uint8_t dataByte = buffer[i++];

                    uint8_t mod = (dataByte & 0b11000000) >> 6;
                    uint8_t reg = (dataByte & 0b00111000) >> 3;
                    uint8_t rm  = (dataByte & 0b00000111) >> 0;

                    if (mod == ModType_RegisterMode)
                    {
                        if (isDestinationInReg)
                        {
                            fprintf(outputFile, "MOV %s, %s\n", RegToString(reg, isWordData), RegToString(rm, isWordData));
                        }
                        else
                        {
                            fprintf(outputFile, "MOV %s, %s\n", RegToString(rm, isWordData), RegToString(reg, isWordData));
                        }
                    }
                    else if (isDestinationInReg)
                    {
                        RmToStringContext ctx = { .rmstr = {0}, .buffer = buffer, .i = &i };
                        fprintf(outputFile, "MOV %s, %s\n", RegToString(reg, isWordData), RmToString(rm, mod, &ctx));
                    }
                    else
                    {
                        RmToStringContext ctx = { .rmstr = {0}, .buffer = buffer, .i = &i };
                        fprintf(outputFile, "MOV %s, %s\n", RmToString(rm, mod, &ctx), RegToString(reg, isWordData));
                    }
                    break;
                }
                case OpCodeType_MOV_IM__RM:
                {
                    uint8_t isWordData = (0b00000001 & opCodeByte) >> 0;

                    uint8_t dataByte = buffer[i++];
                    uint8_t mod = (dataByte & 0b11000000) >> 6;
                    uint8_t rm  = (dataByte & 0b00000111) >> 0;

                    RmToStringContext ctx = { .rmstr = {0}, .buffer = buffer, .i = &i };
                    RmToString(rm, mod, &ctx);
                    uint16_t data = ReadData(buffer, &i, isWordData);
                    fprintf(outputFile, "MOV %s, %s %u\n", ctx.rmstr, isWordData ? "word" : "byte", data);
                    break;
                }
                case OpCodeType_MOV_IM_REG:
                {
                    uint8_t isWordData = (0b00001000 & opCodeByte) >> 3;
                    uint8_t reg        = (opCodeByte & 0b00000111) >> 0;
                    fprintf(outputFile, "MOV %s, %u\n", RegToString(reg, isWordData), ReadData(buffer, &i, isWordData));
                    break;
                }
                case OpCodeType_MOV_MEM_AC:
                {
                    uint8_t isWordData = (0b00000001 & opCodeByte);
                    fprintf(outputFile, "MOV %s, [%u]\n", RegToString(RegType_AX, isWordData), ReadData(buffer, &i, isWordData));
                    break;
                }
                case OpCodeType_MOV_AC_MEM:
                {
                    uint8_t isWordData = (0b00000001 & opCodeByte);
                    fprintf(outputFile, "MOV [%u], %s\n", ReadData(buffer, &i, isWordData), RegToString(RegType_AX, isWordData));
                    break;
                }
                default:
                    assert(0);
                    break;
            }
        }
    }

    RETURN({
        if (inputFile)  fclose(inputFile);
        if (outputFile) fclose(outputFile);
    });
}