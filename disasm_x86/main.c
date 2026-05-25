#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#define RETURN_INIT() int error_code = 0
#define RETURN_SUCCESS() goto end
#define RETURN_ERROR(err_code, message) printf("Error|" message "\n"); error_code = err_code; goto end
#define RETURN(clean_block) end: clean_block return error_code

#define REGISTER_OPERATION(shift, opcode)\
    if ((opCodeByte >> shift) == opcode)\
    {\
        *outOpCode = opcode;\
        return 1;\
    }\

typedef enum 
{
    OpCodeType_MOV_RM_REG = 0b00100010,
    OpCodeType_ADD_RM_REG = 0b00000000,
    OpCodeType_SUB_RM_REG = 0b00001010,
    OpCodeType_CMP_RM_REG = 0b00001110,

    OpCodeType_MOV_IM__RM = 0b01100011,
    OpCodeType_ADDSUB_CMP = 0b00100000,

    OpCodeType_MOV_MEM_AC = 0b01010000,
    OpCodeType_ADD_IM__AC = 0b00000010,
    OpCodeType_SUB_IM__AC = 0b00010110,
    OpCodeType_CMP_IM__AC = 0b00011110,
    
    OpCodeType_MOV_IM_REG = 0b00001011,
    OpCodeType_MOV_AC_MEM = 0b01010001,
    OpCodeType_MOV_RM_SEG = 0b10001110,
    OpCodeType_MOV_SEG_RM = 0b10001100,

    OpCodeType_JE         = 0b01110100,
    OpCodeType_JL         = 0b01111100,
    OpCodeType_JLE        = 0b01111110,
    OpCodeType_JB         = 0b01110010,
    OpCodeType_JBE        = 0b01110110,
    OpCodeType_JP         = 0b01111010,
    OpCodeType_JO         = 0b01110000,
    OpCodeType_JS         = 0b01111000,
    OpCodeType_JNE        = 0b01110101,
    OpCodeType_JNL        = 0b01111101,
    OpCodeType_JG         = 0b01111111,
    OpCodeType_JNB        = 0b01110011,
    OpCodeType_JA         = 0b01110111,
    OpCodeType_JNP        = 0b01111011,
    OpCodeType_JNO        = 0b01110001,
    OpCodeType_JNS        = 0b01111001,
    OpCodeType_LOOP       = 0b11100010,
    OpCodeType_LOOPZ      = 0b11100001,
    OpCodeType_LOOPNZ     = 0b11100000,
    OpCodeType_JCXZ       = 0b11100011
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

uint8_t GetOpCodeType(OpCodeType* outOpCode, uint8_t opCodeByte)
{
    REGISTER_OPERATION(2, OpCodeType_MOV_RM_REG)
    REGISTER_OPERATION(2, OpCodeType_ADD_RM_REG)
    REGISTER_OPERATION(2, OpCodeType_SUB_RM_REG)
    REGISTER_OPERATION(2, OpCodeType_CMP_RM_REG)

    REGISTER_OPERATION(1, OpCodeType_MOV_IM__RM)
    REGISTER_OPERATION(2, OpCodeType_ADDSUB_CMP)

    REGISTER_OPERATION(1, OpCodeType_MOV_MEM_AC)
    REGISTER_OPERATION(1, OpCodeType_ADD_IM__AC)
    REGISTER_OPERATION(1, OpCodeType_SUB_IM__AC)
    REGISTER_OPERATION(1, OpCodeType_CMP_IM__AC)

    REGISTER_OPERATION(4, OpCodeType_MOV_IM_REG)
    REGISTER_OPERATION(1, OpCodeType_MOV_AC_MEM)
    REGISTER_OPERATION(0, OpCodeType_MOV_RM_SEG)
    REGISTER_OPERATION(0, OpCodeType_MOV_SEG_RM)

    REGISTER_OPERATION(0, OpCodeType_JE)
    REGISTER_OPERATION(0, OpCodeType_JL)
    REGISTER_OPERATION(0, OpCodeType_JLE)
    REGISTER_OPERATION(0, OpCodeType_JB)
    REGISTER_OPERATION(0, OpCodeType_JBE)
    REGISTER_OPERATION(0, OpCodeType_JP)
    REGISTER_OPERATION(0, OpCodeType_JO)
    REGISTER_OPERATION(0, OpCodeType_JS)
    REGISTER_OPERATION(0, OpCodeType_JNE)
    REGISTER_OPERATION(0, OpCodeType_JNL)
    REGISTER_OPERATION(0, OpCodeType_JG)
    REGISTER_OPERATION(0, OpCodeType_JNB)
    REGISTER_OPERATION(0, OpCodeType_JA)
    REGISTER_OPERATION(0, OpCodeType_JNP)
    REGISTER_OPERATION(0, OpCodeType_JNO)
    REGISTER_OPERATION(0, OpCodeType_JNS)
    REGISTER_OPERATION(0, OpCodeType_LOOP)
    REGISTER_OPERATION(0, OpCodeType_LOOPZ)
    REGISTER_OPERATION(0, OpCodeType_LOOPNZ)
    REGISTER_OPERATION(0, OpCodeType_JCXZ)

    return 0;
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
            fprintf(outputFile, "label_%zu: ", i);

            uint8_t opCodeByte = buffer[i++];

            OpCodeType opCodeType;

            if (!GetOpCodeType(&opCodeType, opCodeByte))
            {
                printf("Unknown instruction: %#x\n", opCodeByte);
                RETURN_ERROR(1, "Encountered unknown instruction.");
            }

            switch(opCodeType)
            {
                case OpCodeType_MOV_RM_REG:
                case OpCodeType_ADD_RM_REG:
                case OpCodeType_SUB_RM_REG:
                case OpCodeType_CMP_RM_REG:
                {
                    uint8_t isDestinationInReg = (0b00000010 & opCodeByte) >> 1;
                    uint8_t isWordData         = (0b00000001 & opCodeByte) >> 0;

                    uint8_t dataByte = buffer[i++];

                    uint8_t mod = (dataByte & 0b11000000) >> 6;
                    uint8_t reg = (dataByte & 0b00111000) >> 3;
                    uint8_t rm  = (dataByte & 0b00000111) >> 0;

                    char opStr[4] = {0};
                    if (opCodeType == OpCodeType_ADD_RM_REG) strncpy(opStr, "ADD", sizeof(opStr));
                    if (opCodeType == OpCodeType_SUB_RM_REG) strncpy(opStr, "SUB", sizeof(opStr));
                    if (opCodeType == OpCodeType_MOV_RM_REG) strncpy(opStr, "MOV", sizeof(opStr));
                    if (opCodeType == OpCodeType_CMP_RM_REG) strncpy(opStr, "CMP", sizeof(opStr));

                    if (mod == ModType_RegisterMode)
                    {
                        if (isDestinationInReg)
                        {
                            fprintf(outputFile, "%s %s, %s\n", opStr, RegToString(reg, isWordData), RegToString(rm, isWordData));
                        }
                        else
                        {
                            fprintf(outputFile, "%s %s, %s\n", opStr, RegToString(rm, isWordData), RegToString(reg, isWordData));
                        }
                    }
                    else if (isDestinationInReg)
                    {
                        RmToStringContext ctx = { .rmstr = {0}, .buffer = buffer, .i = &i };
                        fprintf(outputFile, "%s %s, %s\n", opStr, RegToString(reg, isWordData), RmToString(rm, mod, &ctx));
                    }
                    else
                    {
                        RmToStringContext ctx = { .rmstr = {0}, .buffer = buffer, .i = &i };
                        fprintf(outputFile, "%s %s, %s\n", opStr, RmToString(rm, mod, &ctx), RegToString(reg, isWordData));
                    }
                    break;
                }
                case OpCodeType_MOV_IM__RM:
                case OpCodeType_ADDSUB_CMP:
                {
                    uint8_t isSignExtended = (0b00000010 & opCodeByte) >> 1;
                    uint8_t isWordData     = (0b00000001 & opCodeByte) >> 0;

                    uint8_t dataByte = buffer[i++];
                    uint8_t mod  = (dataByte & 0b11000000) >> 6;
                    uint8_t type = (dataByte & 0b00111000) >> 3;
                    uint8_t rm   = (dataByte & 0b00000111) >> 0;

                    char opStr[4] = {0};
                    strncpy(opStr, "MOV", sizeof(opStr));
                    if (opCodeType == OpCodeType_ADDSUB_CMP)
                    {
                        if (type == 0b00000111) strncpy(opStr, "CMP", sizeof(opStr));
                        if (type == 0b00000000) strncpy(opStr, "ADD", sizeof(opStr));
                        if (type == 0b00000101) strncpy(opStr, "SUB", sizeof(opStr));
                    }

                    const char* rmStr = NULL;
                    if (mod == ModType_RegisterMode)
                    {
                        rmStr = RegToString(rm, isWordData);
                    }
                    else
                    {
                        RmToStringContext ctx = { .rmstr = {0}, .buffer = buffer, .i = &i };
                        rmStr = RmToString(rm, mod, &ctx);
                    }
                    
                    if (opCodeType == OpCodeType_ADDSUB_CMP)
                    {
                        if (isSignExtended)
                        {
                            int8_t data = (int8_t)buffer[i++];
                            fprintf(outputFile, "%s %s %s, %u\n", opStr, isWordData ? "word" : "byte", rmStr, data);
                        }
                        else
                        {
                            uint16_t data = ReadData(buffer, &i, isWordData);
                            fprintf(outputFile, "%s %s %s, %u\n", opStr, isWordData ? "word" : "byte", rmStr, data);
                        }
                    }
                    else
                    {
                        uint16_t data = ReadData(buffer, &i, isWordData);
                        fprintf(outputFile, "%s %s, %s %u\n", opStr, rmStr, isWordData ? "word" : "byte", data);
                    }
                    break;
                }
                case OpCodeType_MOV_IM_REG:
                {
                    uint8_t isWordData = (0b00001000 & opCodeByte) >> 3;
                    uint8_t reg        = (opCodeByte & 0b00000111) >> 0;
                    fprintf(outputFile, "MOV %s, %u\n", RegToString(reg, isWordData), ReadData(buffer, &i, isWordData));
                    break;
                }
                case OpCodeType_ADD_IM__AC:
                case OpCodeType_SUB_IM__AC:
                case OpCodeType_CMP_IM__AC:
                {
                    char opStr[4] = {0};
                    if (opCodeType == OpCodeType_CMP_IM__AC) strncpy(opStr, "CMP", sizeof(opStr));
                    if (opCodeType == OpCodeType_ADD_IM__AC) strncpy(opStr, "ADD", sizeof(opStr));
                    if (opCodeType == OpCodeType_SUB_IM__AC) strncpy(opStr, "SUB", sizeof(opStr));

                    uint8_t isWordData = (0b00000001 & opCodeByte);
                    fprintf(outputFile, "%s %s, %u\n", opStr, RegToString(RegType_AX, isWordData), ReadData(buffer, &i, isWordData));
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
                case OpCodeType_JE:
                case OpCodeType_JL:
                case OpCodeType_JLE:
                case OpCodeType_JB:
                case OpCodeType_JBE:
                case OpCodeType_JP:
                case OpCodeType_JO:
                case OpCodeType_JS:
                case OpCodeType_JNE:
                case OpCodeType_JNL:
                case OpCodeType_JG:
                case OpCodeType_JNB:
                case OpCodeType_JA:
                case OpCodeType_JNP:
                case OpCodeType_JNO:
                case OpCodeType_JNS:
                case OpCodeType_LOOP:
                case OpCodeType_LOOPZ:
                case OpCodeType_LOOPNZ:
                case OpCodeType_JCXZ:
                {
                    char opStr[8] = {0};
                    switch(opCodeType)
                    {
                        case OpCodeType_JE: strncpy(opStr, "JE", sizeof(opStr)); break;
                        case OpCodeType_JL: strncpy(opStr, "JL", sizeof(opStr)); break;
                        case OpCodeType_JLE: strncpy(opStr, "JLE", sizeof(opStr)); break;
                        case OpCodeType_JB: strncpy(opStr, "JB", sizeof(opStr)); break;
                        case OpCodeType_JBE: strncpy(opStr, "JBE", sizeof(opStr)); break;
                        case OpCodeType_JP: strncpy(opStr, "JP", sizeof(opStr)); break;
                        case OpCodeType_JO: strncpy(opStr, "JO", sizeof(opStr)); break;
                        case OpCodeType_JS: strncpy(opStr, "JS", sizeof(opStr)); break;
                        case OpCodeType_JNE: strncpy(opStr, "JNE", sizeof(opStr)); break;
                        case OpCodeType_JNL: strncpy(opStr, "JNL", sizeof(opStr)); break;
                        case OpCodeType_JG: strncpy(opStr, "JG", sizeof(opStr)); break;
                        case OpCodeType_JNB: strncpy(opStr, "JNB", sizeof(opStr)); break;
                        case OpCodeType_JA: strncpy(opStr, "JA", sizeof(opStr)); break;
                        case OpCodeType_JNP: strncpy(opStr, "JNP", sizeof(opStr)); break;
                        case OpCodeType_JNO: strncpy(opStr, "JNO", sizeof(opStr)); break;
                        case OpCodeType_JNS: strncpy(opStr, "JNS", sizeof(opStr)); break;
                        case OpCodeType_LOOP: strncpy(opStr, "LOOP", sizeof(opStr)); break;
                        case OpCodeType_LOOPZ: strncpy(opStr, "LOOPZ", sizeof(opStr)); break;
                        case OpCodeType_LOOPNZ: strncpy(opStr, "LOOPNZ", sizeof(opStr)); break;
                        case OpCodeType_JCXZ: strncpy(opStr, "JCXZ", sizeof(opStr)); break;
                        default: assert(0); break;
                    }

                    int8_t offset = (int8_t)buffer[i++];
                    size_t labelNum = i + offset;

                    fprintf(outputFile, "%s label_%zu\n", opStr, labelNum);
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