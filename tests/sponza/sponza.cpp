
#include <immintrin.h>
#include <intrin.h>

const char sponzaPixelShader[] = R"(
SamplerState mySampler : register(s0, space1);
Texture2D texture : register(t0, space2);

struct PixelInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

float4 main(PixelInput input) : SV_Target
{
    return texture.Sample(mySampler, input.uv);
}   
)";

const char sponzaVertexShader[] = R"(
cbuffer SpinningCubeData : register(b0, space0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix2;
    float4x4 modelMatrix;
};

struct VertexInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

VertexOutput main(VertexInput vertexInput)
{
    float4x4 viewProjMatrix = mul(viewMatrix, projMatrix);
    float4x4 modelViewProjMatrix = mul(modelMatrix, viewProjMatrix);

    VertexOutput output;
    output.pos = mul(float4(vertexInput.pos, 1.0f), modelViewProjMatrix);
    output.uv = vertexInput.uv;
    output.normal = vertexInput.normal;
    return output;
}
)";

void doSponzaSample(sample_frame_parameter_t* pFrameParameter)
{

}

struct file_mapping_t
{
	uint8_t* pFileBaseAddress		= nullptr;
	HANDLE	 pFileMappingHandle		= nullptr;
	HANDLE   pFileHandle			= nullptr;
	uint32_t fileSizeInBytes		= 0u;
};

void unmapFileMapping(file_mapping_t* pFileMapping)
{
	if(pFileMapping->pFileBaseAddress != nullptr)
	{
		UnmapViewOfFile( pFileMapping->pFileBaseAddress);
		pFileMapping->pFileBaseAddress = nullptr;
	}

	if(pFileMapping->pFileMappingHandle != nullptr)
	{
		CloseHandle(pFileMapping->pFileMappingHandle);
		pFileMapping->pFileMappingHandle = nullptr;
	}

	if(pFileMapping->pFileHandle != nullptr)
	{
		CloseHandle(pFileMapping->pFileHandle);
		pFileMapping->pFileHandle = nullptr;
	}
}

bool mapFileForReading(file_mapping_t* pOutFileMapping, const char* pFileName)
{
	const HANDLE pFileHandle = CreateFileA(pFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0u, nullptr);
	if(pFileHandle== INVALID_HANDLE_VALUE)
	{
		const DWORD lastError = GetLastError();
		printf("Could not open file handle to '%s'. CreateFileA() error = %lu\n", pFileName, lastError);
		return false;
	}

	const HANDLE pFileMappingHandle = CreateFileMapping(pFileHandle, nullptr, PAGE_READONLY, 0u, 0u, nullptr);
	if(pFileMappingHandle == INVALID_HANDLE_VALUE)
	{
		const DWORD lastError = GetLastError();
		printf("Could not create file mapping handle for file '%s'. CreateFileMapping() error = %lu.\n", pFileName, lastError);

		CloseHandle(pFileHandle);
		return false;
	}

	uint8_t* pFileBaseAddress = (uint8_t*)MapViewOfFile(pFileMappingHandle, FILE_MAP_READ, 0u, 0u, 0u);
	if(pFileBaseAddress == nullptr)
	{
		const DWORD lastError = GetLastError();
		printf("Could not create map view of file '%s'. MapViewOfFile() error = %lu.\n", pFileName, lastError);

		CloseHandle(pFileHandle);
		CloseHandle(pFileMappingHandle);
		return false;
	}

	pOutFileMapping->pFileHandle 		= pFileHandle;
	pOutFileMapping->pFileMappingHandle = pFileMappingHandle;
	pOutFileMapping->pFileBaseAddress	= pFileBaseAddress;
	pOutFileMapping->fileSizeInBytes 	= (uint32_t)GetFileSize(pFileHandle, nullptr);

	return true;
}

const char* getAbsoluteFilePath(const char* pFilePath, memory_allocator_t* pMemoryAllocator)
{
    char* pAbsoluteFilePath = (char*)allocateFromAllocator(pMemoryAllocator, 1024*1024);
    if(pAbsoluteFilePath == nullptr)
    {
        return nullptr;
    }

    const char* pAbsoluteFilePathEnd = pAbsoluteFilePath + 1024*1024;
    DWORD fileBufferSize = 1024*1024;
    DWORD filePathSize = 0u;
    DWORD relativeFilePathLength = (DWORD)strlen(pFilePath);

    const DWORD result = GetModuleFileNameA(nullptr, pAbsoluteFilePath, fileBufferSize);
    filePathSize = result;

    if(result == fileBufferSize)
    {
        const DWORD lastError = GetLastError();
        if(lastError == ERROR_INSUFFICIENT_BUFFER)
        {
            return nullptr;
        }

        return nullptr;
    }

    char* pEndOfModulePath = pAbsoluteFilePath + fileBufferSize;
    while(*pEndOfModulePath != '\\' && *pEndOfModulePath != '/')
    {
        --pEndOfModulePath;
    }

    pEndOfModulePath[1] = 0;

    if(pEndOfModulePath + relativeFilePathLength >= pAbsoluteFilePathEnd)
    {
        return nullptr;
    }

    strcat(pEndOfModulePath, pFilePath);
    return pAbsoluteFilePath;
}

bool isAbsoluteFilePath(const char* pFilePath)
{
    return isalpha(pFilePath[0]) && pFilePath[1] == ':';
}

enum class gltf_component_type_t : int
{
    BYTE = 0,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    UNSIGNED_INT,
    FLOAT
};

enum class gltf_type_t
{
    NONE,
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4
};

struct gltf_accessor_t
{
    int                     bufferViewIndex;
    int                     byteOffset;
    gltf_component_type_t   componentType;
    int                     count;
    gltf_type_t             type;
};

struct gltf_attribute_t
{
    char*   pAttributeSemantic;
    int     accessorIndex;
};

struct gltf_primitive_t
{
    gltf_attribute_t attributes[8];
    int attributeCount;
    int indices;
    int mode;
    int materialIndex;
};

struct gltf_mesh_t
{
    int                 numPrimitives;
    gltf_primitive_t*   pPrimitives;
    char*               pName;
};

struct gltf_material_t
{

};

struct gltf_buffer_view_t
{

};

struct gltf_buffer_t
{

};

struct gltf_description_t
{
    gltf_material_t*    pMaterials;
    gltf_accessor_t*    pAccessors;
    gltf_mesh_t*        pMeshes;
    gltf_buffer_view_t* pBufferViews;
    gltf_buffer_t*      pBuffers;
    int                 accessorCount;
    int                 meshCount;
    int                 materialCount;
    int                 bufferCount;
};

union parse_pos_t
{
    __m256i vec;
    char    chars[32];    //for better debugging
};
static_assert(sizeof(parse_pos_t::vec) == sizeof(parse_pos_t::chars));

static int gltfObjectTypeStackCapacity = ( sizeof(uint64_t) * 8 ) / 4;

struct gltf_load_result_t
{
    bool success;
    char errorMessage[256];
};

struct gltf_parser_context_t
{
    int                 lineIndex;
    int                 charIndex;
    int                 objectDepth;
    uint64_t            objectTypeStack;
    gltf_load_result_t  result;
    gltf_description_t* pGltfDescription;
    memory_allocator_t  memoryAllocator;
    const parse_pos_t*  pCurrentParsePos;
    const void*         pEndGltfBuffer;
};

enum class parser_state_t
{
    read_object,
    eat_leading_whitespaces,
    skip_property,
    read_meshes,
    read_materials,
    read_buffer_views,
    read_buffers,
    read_accessors,
    finish_parsing
};

enum class gltf_object_type_t
{
    none,
    object,
    array,
    property
};

bool isNextGltfToken(gltf_parser_context_t* pParserContext, const char token)
{
    return pParserContext->pCurrentParsePos->chars[0] == token;
}

bool isInvalidParserState(gltf_parser_context_t* pParserContext)
{
    return pParserContext->result.success == false;
}

void setGltfParserContextError(gltf_parser_context_t* pParserContext, const char* pFormat, ...)
{
    pParserContext->result.success = false;

    int errorMessageBufferSize = sizeof(gltf_load_result_t::errorMessage);
    const int lineIndex = pParserContext->lineIndex + 1;
    const int charIndex = pParserContext->charIndex + 1;
    const int errorMessagePrefixSize = sprintf_s(pParserContext->result.errorMessage, errorMessageBufferSize, "line:%d char:%d: ", lineIndex, charIndex);

    errorMessageBufferSize -= errorMessagePrefixSize;

    va_list vaList;
    va_start(vaList, pFormat);
    vsprintf_s(pParserContext->result.errorMessage + errorMessagePrefixSize, errorMessageBufferSize, pFormat, vaList);
    va_end(vaList);
}

void advanceGltfParserForCharAmount(gltf_parser_context_t* pParserContext, const uint32_t numCharsToSkip)
{
    const __m256i newLines = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, _mm256_set1_epi8('\n'));
    const int validCharMask = (1 << numCharsToSkip) - 1;
    const int newLineMask = _mm256_movemask_epi8(newLines) & validCharMask;
    const int newLineCount = __popcnt(newLineMask);

    if(newLineCount > 0)
    {
        unsigned long lastNewLineIndex = 0;
        _BitScanReverse(&lastNewLineIndex, newLineMask);
        pParserContext->lineIndex += newLineCount;
        pParserContext->charIndex = lastNewLineIndex == numCharsToSkip ? 0 : numCharsToSkip - lastNewLineIndex;
    }
    else
    {
        pParserContext->charIndex += numCharsToSkip;
    }
    
    const parse_pos_t* pNextParsePos = (parse_pos_t*)((char*)pParserContext->pCurrentParsePos + numCharsToSkip);
    if(pNextParsePos >= pParserContext->pEndGltfBuffer)
    {
        setGltfParserContextError(pParserContext, "Trying to read past gltf buffer - current parse pos: '%s'", pParserContext->pCurrentParsePos->chars);
        return;
    }

    pParserContext->pCurrentParsePos = pNextParsePos;
    return;
}

void advanceGltfParserToTokenAndIgnoreAllOtherTokens(gltf_parser_context_t* pParserContext, const char tokenToFind)
{
    const __m256i token = _mm256_set1_epi8(tokenToFind);
    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }
        
        const __m256i compareResult = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, token);
        const int tokenMask = _mm256_movemask_epi8(compareResult);

        if(tokenMask == 0)
        {
            advanceGltfParserForCharAmount(pParserContext, 32);
            continue;
        }
        
        unsigned long tokenPos = 0;
        _BitScanForward(&tokenPos, tokenMask);
        if(tokenPos == 0)
        {
            break;
        }

        advanceGltfParserForCharAmount(pParserContext, tokenPos);
    }
    
    advanceGltfParserForCharAmount(pParserContext, 1);
}

void advanceGltfParserToTokens(gltf_parser_context_t* pParserContext, const char* pTokensToFind, const int tokenCount)
{
    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        __m256i filter = _mm256_setzero_si256();
        for(int i = 0; i < tokenCount; ++i)
        {
            filter = _mm256_or_si256(filter, _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, _mm256_set1_epi8(pTokensToFind[i])));
        }

        const unsigned int filterMask = _mm256_movemask_epi8(filter);
        if(filterMask == 0)
        {
            advanceGltfParserForCharAmount(pParserContext, 32u);
            continue;
        }
        else if(filterMask == 1)
        {
            break;
        }

        unsigned long tokenPos = 0;
        _BitScanForward(&tokenPos, filterMask);
        if(tokenPos == 0)
        {
            break;
        }

        advanceGltfParserForCharAmount(pParserContext, tokenPos);
        break;
    }
}

void advanceGltfParserAndIgnoreTokens(gltf_parser_context_t* pParserContext, const char* pTokensToIgnore, const int tokenCount)
{
    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        __m256i filter = _mm256_setzero_si256();
        for(int i = 0; i < tokenCount; ++i)
        {
            filter = _mm256_or_si256(filter, _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, _mm256_set1_epi8(pTokensToIgnore[i])));
        }

        const unsigned int filterMask = _mm256_movemask_epi8(filter);
        if(filterMask == 0xFFFFFFFF)
        {
            advanceGltfParserForCharAmount(pParserContext, 32u);
            continue;
        }
        else if(filterMask == 0)
        {
            break;
        }

        unsigned long tokenPos = 0;
        _BitScanForward(&tokenPos, ~filterMask);
        if(tokenPos == 0)
        {
            break;
        }

        advanceGltfParserForCharAmount(pParserContext, tokenPos);
        break;
    }
}

void advanceGtlfParserToNextSpaceOrComma(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    const char commaOrSpace[] = {
        ',', ' '
    };

    advanceGltfParserToTokens(pParserContext, commaOrSpace, ARRAY_SIZE(commaOrSpace));
}

void advanceGltfParserToNextNonWhitespaceToken(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    const char whitespaceTokens[] = {
        ' ', '\n', '\r', '\t'
    };

    advanceGltfParserAndIgnoreTokens(pParserContext, whitespaceTokens, ARRAY_SIZE(whitespaceTokens));
}

void advanceGltfParserToNextLine(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    advanceGltfParserToTokenAndIgnoreAllOtherTokens(pParserContext, '\n');
}

void advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(gltf_parser_context_t* pParserContext, const char expectedToken)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(isNextGltfToken(pParserContext, expectedToken))
    {
        advanceGltfParserForCharAmount(pParserContext, 1);
        return;
    }

    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    const char token = pParserContext->pCurrentParsePos->chars[0];
    if(token != expectedToken)
    {
        setGltfParserContextError(pParserContext, "Unexpected token, read '%c' but expected '%c'", token, expectedToken);
        return;
    }

    advanceGltfParserForCharAmount(pParserContext, 1);
}

void readGltfString(gltf_parser_context_t* pParserContext, char** ppOutStringBuffer)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(isNextGltfToken(pParserContext, '"'))
    {
        advanceGltfParserForCharAmount(pParserContext, 1);
    }

    const __m256i quoteMask = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, _mm256_set1_epi8('"'));
    unsigned int quoteMaskBits = _mm256_movemask_epi8(quoteMask);
    if(__popcnt(quoteMaskBits) < 1)
    {
        setGltfParserContextError(pParserContext, "Error trying to read object name in line '%s' - couldn't find matching quotes.", pParserContext->pCurrentParsePos->chars);
        return;
    }

    unsigned long closeQuotesPos= 0;
    _BitScanForward(&closeQuotesPos, quoteMaskBits);

    char* pStringBuffer = nullptr;
    
    if(ppOutStringBuffer != nullptr)
    {
        pStringBuffer = (char*)allocateFromAllocator(&pParserContext->memoryAllocator, 32);
    }

    const __m256i objectName = _mm256_andnot_si256(quoteMask, pParserContext->pCurrentParsePos->vec);
    _mm256_store_si256((__m256i*)pStringBuffer, objectName);
    
    advanceGltfParserForCharAmount(pParserContext, closeQuotesPos+1);
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    if(ppOutStringBuffer != nullptr)
    {
        *ppOutStringBuffer = pStringBuffer;
    }
}

void readGltfInteger(gltf_parser_context_t* pParserContext, int* pOutInteger)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    static const __m128i numberMaskLut[] = {
        _mm_set_epi32(0x00000000, 0x00000000, 0x00000000, 0x000000FF), _mm_set_epi32(0x00000000, 0x00000000, 0x00000000, 0x0000FFFF), _mm_set_epi32(0x00000000, 0x00000000, 0x00000000, 0x00FFFFFF), _mm_set_epi32(0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF),
        _mm_set_epi32(0x00000000, 0x00000000, 0x000000FF, 0xFFFFFFFF), _mm_set_epi32(0x00000000, 0x00000000, 0x0000FFFF, 0xFFFFFFFF), _mm_set_epi32(0x00000000, 0x00000000, 0x00FFFFFF, 0xFFFFFFFF), _mm_set_epi32(0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF)
    };

    static const __m256i numberExtendLut[] = {
        _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 1), 
        _mm256_set_epi32(0, 0, 0, 0, 0, 0, 1, 10), 
        _mm256_set_epi32(0, 0, 0, 0, 0, 1, 10, 100),
        _mm256_set_epi32(0, 0, 0, 0, 1, 10, 100, 1000), 
        _mm256_set_epi32(0, 0, 0, 1, 10, 100, 1000, 10000),
        _mm256_set_epi32(0, 0, 1, 10, 100, 1000, 10000, 100000),
        _mm256_set_epi32(0, 1, 10, 100, 1000, 10000, 100000, 1000000),
        _mm256_set_epi32(1, 10, 100, 1000, 10000, 100000, 1000000, 10000000),
    };

    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    const bool isNegative = isNextGltfToken(pParserContext, '-');
    if(isNegative)
    {
        advanceGltfParserForCharAmount(pParserContext, 1);
    }

    __m128i numbers = _mm256_castsi256_si128(pParserContext->pCurrentParsePos->vec);
    numbers = _mm_sub_epi8(numbers, _mm_set1_epi8('0'));
    __m128i numMask = _mm_cmpgt_epi8(numbers, _mm_set1_epi8(-1));
    int numberMask = _mm_movemask_epi8(numMask);
    unsigned long lastDigitIndex = 0;
    _BitScanForward(&lastDigitIndex, ~numberMask);
    
    if(lastDigitIndex >= 8)
    {
        setGltfParserContextError(pParserContext, "Number '%s' is too long", pParserContext->pCurrentParsePos->chars);
        return;
    }
    
    numMask = _mm_and_si128(numMask, numberMaskLut[lastDigitIndex-1]);
    __m128i sparseInteger = _mm_and_si128(numMask, numbers);
    __m256i spraseInteger256 = _mm256_cvtepu8_epi32(sparseInteger);
    spraseInteger256 = _mm256_mullo_epi32(spraseInteger256, numberExtendLut[lastDigitIndex-1]);
    spraseInteger256 = _mm256_hadd_epi32(spraseInteger256, _mm256_setzero_si256());
    spraseInteger256 = _mm256_hadd_epi32(spraseInteger256, _mm256_setzero_si256());
    
    advanceGltfParserForCharAmount(pParserContext, lastDigitIndex);
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    *pOutInteger = _mm256_extract_epi32(spraseInteger256, 0u) + _mm256_extract_epi32(spraseInteger256, 4u);
    return;
}

void readGltfDouble(gltf_parser_context_t* pParserContext, double* pOutDouble)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    int integerPart;
    readGltfInteger(pParserContext, &integerPart);

    if(!isNextGltfToken(pParserContext, '.'))
    {
        *pOutDouble = (double)integerPart;
        return;
    }

    advanceGltfParserForCharAmount(pParserContext, 1);
    
}

bool areStringsEqual32(const char* pStringA, const char** pStringsB, const int stringCount, int* pOutIndex)
{
    const __m256i stringA = _mm256_loadu_epi8(pStringA);
    const int stringMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(stringA, _mm256_setzero_si256()));
    unsigned long stringLength = 0;
    _BitScanForward(&stringLength, stringMask);

    for(int i = 0; i < stringCount; ++i)
    {
        const __m256i stringB = _mm256_loadu_epi8(pStringsB[i]);

        const __m256i compareResult = _mm256_cmpeq_epi8(stringA, stringB);
        const int compareMask = _mm256_movemask_epi8(compareResult);
        const int validBitsMask = (1 << stringLength) - 1;
        const int matchingChars = __popcnt(compareMask & validBitsMask);

        if(matchingChars == stringLength)
        {
            *pOutIndex = i;
            return true;
        }
    }

    return false;
}

parser_state_t mapObjectNameToParserState(const char* pObjectName)
{
    const char* pValidNames[] = {
        "meshes",
        "materials",
        "bufferViews",
        "buffers",
        "accessors"
    };

    parser_state_t parserStateToReturnOnMatch[] = {
        parser_state_t::read_meshes,
        parser_state_t::read_materials,
        parser_state_t::read_buffer_views,
        parser_state_t::read_buffers,
        parser_state_t::read_accessors
    };
    static_assert(ARRAY_SIZE(pValidNames) == ARRAY_SIZE(parserStateToReturnOnMatch));

    int matchingIndex = ~0;
    if(!areStringsEqual32(pObjectName, pValidNames, ARRAY_SIZE(pValidNames), &matchingIndex))
    {
        return parser_state_t::skip_property;
    }

    return parserStateToReturnOnMatch[matchingIndex];
}

void skipGltfBracketedProperty(gltf_parser_context_t* pParserContext, const char openBracketCharacter, const char closedBracketCharacter)
{
    const __m256i openBrackets = _mm256_set1_epi8(openBracketCharacter);
    const __m256i closeBrackets = _mm256_set1_epi8(closedBracketCharacter);

    uint32_t depth = 1;
    while(depth > 0)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        const __m256i openBracketsCmpResult = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, openBrackets);
        const __m256i closingBracketsCmpResult = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, closeBrackets);
        uint32_t openingBracketsMask = _mm256_movemask_epi8(openBracketsCmpResult);
        uint32_t closingBracketsMask = _mm256_movemask_epi8(closingBracketsCmpResult);
        uint32_t bracketsMask = openingBracketsMask | closingBracketsMask;

        if(bracketsMask == 0)
        {
            advanceGltfParserForCharAmount(pParserContext, 32);
            continue;
        }

        unsigned long lastBracketIndex = 0;
        while(bracketsMask != 0)
        {
            unsigned long bracketIndex = 0;
            _BitScanForward(&bracketIndex, bracketsMask);
            const int bracketBit = (1 << bracketIndex);
            bracketsMask &= ~bracketBit;
            if(bracketBit & openingBracketsMask)
            {
                depth += 1;
            }
            else
            {
                depth -= 1;
            }

            lastBracketIndex = bracketIndex;

            if(depth == 0)
            {
                break;
            }
        }

        if(depth == 0)
        {
            advanceGltfParserForCharAmount(pParserContext, lastBracketIndex + 1);
            break;
        }

        advanceGltfParserForCharAmount(pParserContext, 32);
    }

    return;
}

void pushGltfObjectTypeToObjectTypeStack(gltf_parser_context_t* pParserContext, const gltf_object_type_t objectType)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->objectDepth >= gltfObjectTypeStackCapacity)
    {
        setGltfParserContextError(pParserContext, "Internal gtlf object type stack overflow");
        return;
    }

    const uint64_t objectDepth = (uint64_t)pParserContext->objectDepth;
    const uint64_t shift = 2 * objectDepth;
    pParserContext->objectTypeStack |= ((uint64_t)objectType << shift);
    ++pParserContext->objectDepth;
}

gltf_object_type_t popGltfObjectTypeFromObjectTypeStack(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return gltf_object_type_t::none;
    }

    if(pParserContext->objectDepth == 0)
    {
        setGltfParserContextError(pParserContext, "Internal gtlf object type stack underflow");
        return gltf_object_type_t::none;
    }

    --pParserContext->objectDepth;
    const uint64_t objectDepth = pParserContext->objectDepth;
    const uint64_t shift = 2 * objectDepth;
    const uint64_t validValueMask = ( 1ull << ( 2ull * pParserContext->objectDepth ) ) - 1ull;
    const gltf_object_type_t ojectType = ( gltf_object_type_t )( ( pParserContext->objectTypeStack >> shift ) & 0x3ull );
    pParserContext->objectTypeStack &= validValueMask;
    return ojectType;
}

gltf_object_type_t getTypeOfCurrentGltfObject(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return gltf_object_type_t::none;
    }

    if(pParserContext->objectDepth == 0)
    {
        setGltfParserContextError(pParserContext, "Internal gtlf object type stack underflow");
        return gltf_object_type_t::none;
    }

    const uint64_t objectDepth = pParserContext->objectDepth - 1;
    const uint64_t shift = 2 * objectDepth;
    return (gltf_object_type_t)(( pParserContext->objectTypeStack >> shift ) & 0x3ull);
}

void openGltfObject(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    if(isNextGltfToken(pParserContext, ','))
    {
        advanceGltfParserForCharAmount(pParserContext, 1);
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }

    gltf_object_type_t objectType = gltf_object_type_t::none;
    if(isNextGltfToken(pParserContext, '{'))
    {
        objectType = gltf_object_type_t::object;
    }
    else if(isNextGltfToken(pParserContext, '['))
    {
        objectType = gltf_object_type_t::array;
    }
    else
    {
        objectType = gltf_object_type_t::property;
    }

    pushGltfObjectTypeToObjectTypeStack(pParserContext, objectType);
    advanceGltfParserForCharAmount(pParserContext, 1);
}

void openGltfObject(gltf_parser_context_t* pParserContext, char** pOutObjectName)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    if(isNextGltfToken(pParserContext, ','))
    {
        advanceGltfParserForCharAmount(pParserContext, 1);
    }

    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '"');
    readGltfString(pParserContext, pOutObjectName);
    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    gltf_object_type_t objectType = gltf_object_type_t::none;
    if(isNextGltfToken(pParserContext, '{'))
    {
        objectType = gltf_object_type_t::object;
        advanceGltfParserForCharAmount(pParserContext, 1);
    }
    else if(isNextGltfToken(pParserContext, '['))
    {
        objectType = gltf_object_type_t::array;
        advanceGltfParserForCharAmount(pParserContext, 1);
    }
    else
    {
        objectType = gltf_object_type_t::property;
    }

    pushGltfObjectTypeToObjectTypeStack(pParserContext, objectType);
}

void closeGltfObject(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    gltf_object_type_t objectType = popGltfObjectTypeFromObjectTypeStack(pParserContext);
    if(objectType == gltf_object_type_t::none)
    {
        setGltfParserContextError(pParserContext, "Mismatch of openGltfObject/closeGltfObject calls");
        return;
    }

    if(objectType == gltf_object_type_t::object)
    {
        skipGltfBracketedProperty(pParserContext, '{', '}');
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }
    else if(objectType == gltf_object_type_t::array)
    {
        skipGltfBracketedProperty(pParserContext, '[', ']');
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }
    else if(objectType == gltf_object_type_t::property)
    {
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
        if(isNextGltfToken(pParserContext, '"'))
        {
            readGltfString(pParserContext, nullptr);
            advanceGtlfParserToNextSpaceOrComma(pParserContext);
        }
        else if(!isNextGltfToken(pParserContext, '}'))
        {
            advanceGtlfParserToNextSpaceOrComma(pParserContext);
        }
    }
}

bool hasMoreObjectsInGltfArray(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return false;
    }

    gltf_object_type_t objectType = getTypeOfCurrentGltfObject(pParserContext);
    if(objectType != gltf_object_type_t::array)
    {
        setGltfParserContextError(pParserContext, "Called 'hasMoreObjectsInGltfArray' but current object is not an array");
        return false;
    }

    return isNextGltfToken(pParserContext, ',');
}

bool hasMoreMembersInGltfObject(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return false;
    }

    gltf_object_type_t objectType = getTypeOfCurrentGltfObject(pParserContext);
    if(objectType != gltf_object_type_t::object)
    {
        setGltfParserContextError(pParserContext, "Called 'hasMoreMembersInGltfObject' but current object is not an array");
        return false;
    }

    return isNextGltfToken(pParserContext, ',');
}

void readGltfObjectCountInArrayWithoutAdvancingParser(gltf_parser_context_t* pParserContext, int* pOutObjectCount)
{
    if(getTypeOfCurrentGltfObject(pParserContext) != gltf_object_type_t::array)
    {
        setGltfParserContextError(pParserContext, "Called 'readGltfObjectCountInArrayWithoutAdvancingParser()' for non array object");
        return;
    }

    const parse_pos_t* pCurrentParsePos = pParserContext->pCurrentParsePos;
    int objectCount = 0;
    while(true)
    {
        openGltfObject(pParserContext);
        closeGltfObject(pParserContext);
        ++objectCount;

        if(!isNextGltfToken(pParserContext, ','))
        {
            break;
        }
        
        advanceGltfParserForCharAmount(pParserContext, 1);
    }

    pParserContext->pCurrentParsePos = pCurrentParsePos;
    *pOutObjectCount = objectCount;
    return;
}

void readGltfMeshPrimitiveAttributes(gltf_parser_context_t* pParserContext, gltf_primitive_t* pPrimitive)
{
    int attributeIndex = 0;

    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        openGltfObject(pParserContext, &pPrimitive->attributes[attributeIndex].pAttributeSemantic);
            readGltfInteger(pParserContext, &pPrimitive->attributes[attributeIndex].accessorIndex);
        closeGltfObject(pParserContext);

        if(!hasMoreMembersInGltfObject(pParserContext))
        {
            break;
        }
        ++attributeIndex;
    }

    pPrimitive->attributeCount = attributeIndex;
    return;
}

void readGltfMeshPrimitive(gltf_parser_context_t* pParserContext, gltf_primitive_t* pPrimitive)
{
    const char* validPrimitivePropertyNames[] = {
        "attributes",
        "indices",
        "mode",
        "material"
    };

    const int attributePropertyIndex = 0;
    const int indicesPropertyIndex = 1;
    const int modePropertyIndex = 2;
    const int materialPropertyIndex = 3;

    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        char* pObjectName = nullptr;
        openGltfObject(pParserContext, &pObjectName);

        int propertyIndex = 0;
        if(areStringsEqual32(pObjectName, validPrimitivePropertyNames, ARRAY_SIZE(validPrimitivePropertyNames), &propertyIndex))
        {
            switch(propertyIndex)
            {
                case indicesPropertyIndex:
                    readGltfInteger(pParserContext, &pPrimitive->indices);
                    break;

                case modePropertyIndex:
                    readGltfInteger(pParserContext, &pPrimitive->mode);
                    break;

                case materialPropertyIndex:
                    readGltfInteger(pParserContext, &pPrimitive->materialIndex);
                    break;
                
                case attributePropertyIndex:
                    readGltfMeshPrimitiveAttributes(pParserContext, pPrimitive);
                    break;
            }
        }

        closeGltfObject(pParserContext);

        if(!hasMoreMembersInGltfObject(pParserContext))
        {
            break;
        }
    }
    return;
}

void readGltfMeshPrimitives(gltf_parser_context_t* pParserContext, gltf_mesh_t* pMesh, memory_allocator_t* pMemoryAllocator)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    int numPrimitives = 0;
    readGltfObjectCountInArrayWithoutAdvancingParser(pParserContext, &numPrimitives);

    gltf_primitive_t* pPrimitives = (gltf_primitive_t*)allocateFromAllocator(&pParserContext->memoryAllocator, sizeof(gltf_primitive_t) * numPrimitives);
    if(pPrimitives == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %u primitives", numPrimitives);
        return;
    }

    pMesh->pPrimitives = pPrimitives;
    pMesh->numPrimitives = numPrimitives;

    int primitiveIndex = 0;
    while(true)
    {
        openGltfObject(pParserContext);
            readGltfMeshPrimitive(pParserContext, pPrimitives + primitiveIndex);
        closeGltfObject(pParserContext);

        if(!hasMoreObjectsInGltfArray(pParserContext))
        {
            break;
        }

        ++primitiveIndex;
    };
    
    return;
}

void readGltfMeshes(gltf_parser_context_t* pParserContext, memory_allocator_t* pMemoryAllocator)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->meshCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'meshes' objects in gltf file.");
        return;
    }

    int numMeshes = 0;
    readGltfObjectCountInArrayWithoutAdvancingParser(pParserContext, &numMeshes);

    gltf_mesh_t* pMeshes = (gltf_mesh_t*)allocateFromAllocator(&pParserContext->memoryAllocator, sizeof(gltf_mesh_t) * numMeshes);
    if(pMeshes == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %u meshes", numMeshes);
        return;
    }

    pParserContext->pGltfDescription->pMeshes = pMeshes;
    pParserContext->pGltfDescription->meshCount = numMeshes;

    const char* validMeshPropertyNames[] = {
        "primitives",
        "name"
    };

    const int meshPrimitivesIndex = 0;
    const int meshNameIndex = 1;
    int meshIndex = 0;
    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        openGltfObject(pParserContext);
        while(true)
        {
            char* pObjectName = nullptr;
            openGltfObject(pParserContext, &pObjectName);

                int propertyIndex = ~0;
                if(areStringsEqual32(pObjectName, validMeshPropertyNames, ARRAY_SIZE(validMeshPropertyNames), &propertyIndex))
                {
                    if(propertyIndex == meshPrimitivesIndex)
                    {
                        readGltfMeshPrimitives(pParserContext, pMeshes, pMemoryAllocator);
                    }
                    else if(propertyIndex == meshNameIndex)
                    {
                        readGltfString(pParserContext, &pMeshes[meshIndex].pName);
                    }
                }

            closeGltfObject(pParserContext);

            if(!hasMoreMembersInGltfObject(pParserContext))
            {
                break;
            }
        }

        closeGltfObject(pParserContext);

        if(!hasMoreObjectsInGltfArray(pParserContext))
        {
            break;
        }

        ++meshIndex;
    }
    return;
}

gltf_type_t parseGltfTypeName(const char* pTypeName)
{
    const __m128i scalarTypeName = _mm_loadu_epi8((const void*)"SCALAR\0");
    const int scalarTypeMask = (1<<7)-1;
    const int compareMask = _mm_movemask_epi8(_mm_cmpeq_epi8(scalarTypeName, *(__m128i*)pTypeName)) & scalarTypeMask;

    if(compareMask == scalarTypeMask)
    {
        return gltf_type_t::SCALAR;
    }

    const int typeName = *(const int*)pTypeName;
    if(typeName == MAKEFOURCC('V', 'E', 'C', '2'))
    {
        return gltf_type_t::VEC2;
    }
    
    if(typeName == MAKEFOURCC('V', 'E', 'C', '3'))
    {
        return gltf_type_t::VEC3;
    }

    if(typeName == MAKEFOURCC('V', 'E', 'C', '4'))
    {
        return gltf_type_t::VEC4;
    }

    if(typeName == MAKEFOURCC('M', 'A', 'T', '2'))
    {
        return gltf_type_t::MAT2;
    }

    if(typeName == MAKEFOURCC('M', 'A', 'T', '3'))
    {
        return gltf_type_t::MAT2;
    }

    if(typeName == MAKEFOURCC('M', 'A', 'T', '4'))
    {
        return gltf_type_t::MAT2;
    }

    return gltf_type_t::NONE;
}

void readGltfAccessors(gltf_parser_context_t* pParserContext, memory_allocator_t* pMemoryAllocator)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->accessorCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'accessor' objects in gltf file.");
        return;
    }

    int numAccessors = 0;
    readGltfObjectCountInArrayWithoutAdvancingParser(pParserContext, &numAccessors);

    gltf_accessor_t* pAccessors = (gltf_accessor_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_accessor_t) * numAccessors);
    if(pAccessors == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %d accessors", numAccessors);
        return;
    }

    pParserContext->pGltfDescription->pAccessors = pAccessors;
    pParserContext->pGltfDescription->accessorCount = numAccessors;

    const char* validAccessorPropertyNames[] = {
        "bufferView",
        "byteOffset",
        "componentType",
        "count",
        "type"
    };

    const int bufferViewIndex = 0;
    const int byteOffsetIndex = 1;
    const int componentTypeIndex = 2;
    const int countIndex = 3;
    const int typeIndex = 4;

    int accessorIndex = 0;
    while(true)
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        openGltfObject(pParserContext);
        while(true)
        {
            char* pObjectName = nullptr;
            openGltfObject(pParserContext, &pObjectName);
            int propertyIndex = 0;
            if(areStringsEqual32(pObjectName, validAccessorPropertyNames, ARRAY_SIZE(validAccessorPropertyNames), &propertyIndex))
            {
                if(propertyIndex == bufferViewIndex)
                {
                    readGltfInteger(pParserContext, &pAccessors[accessorIndex].bufferViewIndex);
                }
                else if(propertyIndex == byteOffsetIndex)
                {
                    readGltfInteger(pParserContext, &pAccessors[accessorIndex].byteOffset);
                }
                else if(propertyIndex == componentTypeIndex)
                {
                    int componentType;
                    readGltfInteger(pParserContext, &componentType);
                    pAccessors[accessorIndex].componentType = (gltf_component_type_t)(componentType - 5120);
                }
                else if(propertyIndex == countIndex)
                {
                    readGltfInteger(pParserContext, &pAccessors[accessorIndex].count);
                }
                else if(propertyIndex == typeIndex)
                {
                    char* pTypeName = nullptr;
                    readGltfString(pParserContext, &pTypeName);
                    pAccessors[accessorIndex].type = parseGltfTypeName(pTypeName);

                    if(pAccessors[accessorIndex].type == gltf_type_t::NONE)
                    {
                        setGltfParserContextError(pParserContext, "Unknown accessor type '%s' for %d. accessor.", pTypeName, accessorIndex+1);
                        return;
                    }
                }
            }
            closeGltfObject(pParserContext);

            if(!hasMoreMembersInGltfObject(pParserContext))
            {
                break;
            }
        }

        closeGltfObject(pParserContext);

        if(!hasMoreObjectsInGltfArray(pParserContext))
        {
            break;
        }

        ++accessorIndex;
    }

    return;
}

gltf_load_result_t createGltfLoadResultError(const char* pFormat, ...)
{
    gltf_load_result_t result = {};
    result.success = false;

    va_list vaList;
    va_start(vaList, pFormat);
    vsprintf_s(result.errorMessage, sizeof(result.errorMessage), pFormat, vaList);
    va_end(vaList);

    return result;
}

void createGltfParserContext(gltf_parser_context_t* pOutContext, const void* pGltfBuffer, const uint32_t gltfBufferSizeInBytes, void* pParserTemporaryMemory, const uint32_t parserTemporaryMemorySizeInBytes)
{
    gltf_parser_context_t gltfParserContext = {};
    createStackMemoryAllocator(&gltfParserContext.memoryAllocator, parserTemporaryMemorySizeInBytes, pParserTemporaryMemory);

    gltfParserContext.pCurrentParsePos = (parse_pos_t*)pGltfBuffer;
    gltfParserContext.pEndGltfBuffer = (char*)pGltfBuffer + gltfBufferSizeInBytes;
    gltfParserContext.pGltfDescription = (gltf_description_t*)allocateFromAllocator(&gltfParserContext.memoryAllocator, sizeof(gltf_description_t), alloc_flags_t::clear_memory);
    gltfParserContext.lineIndex = 0;
    gltfParserContext.charIndex = 0;
    gltfParserContext.result.success = true;

    *pOutContext = gltfParserContext;
}

gltf_load_result_t loadGltfDescriptionFromBuffer(gltf_description_t* pOutDescription, const uint8_t* pGltfBuffer, const uint32_t gltfBufferSize, memory_allocator_t* pMemoryAllocator)
{
    parser_state_t parserState = parser_state_t::read_object;

    const uint32_t parserTempMemorySizeInBytes = 1024*1024*1024;
    void* pParserTempMemory = allocateFromAllocator(pMemoryAllocator, parserTempMemorySizeInBytes, alloc_flags_t::clear_memory);

    gltf_parser_context_t gltfParserContext = {};
    createGltfParserContext(&gltfParserContext, pGltfBuffer, gltfBufferSize, pParserTempMemory, parserTempMemorySizeInBytes);
    if(gltfParserContext.pGltfDescription == nullptr)
    {
        return createGltfLoadResultError("Out of memory trying to allocate %d bytes for gltf descriptor", sizeof(gltf_description_t));
    }

    openGltfObject(&gltfParserContext);
    while(true)
    {
        if(parserState == parser_state_t::finish_parsing)
        {
            break;
        }

        switch(parserState)
        {
            case parser_state_t::read_object:
            {
                char* pObjectName = nullptr;
                openGltfObject(&gltfParserContext, &pObjectName);
                parserState = mapObjectNameToParserState(pObjectName);
                break;
            }

            case parser_state_t::skip_property:
            {
                closeGltfObject(&gltfParserContext);
                parserState = parser_state_t::read_object;
                break;
            }

             case parser_state_t::read_meshes:
            {
                readGltfMeshes(&gltfParserContext, pMemoryAllocator);
                closeGltfObject(&gltfParserContext);
                parserState = parser_state_t::read_object;
                break;
            }

            case parser_state_t::read_accessors:
            {
                readGltfAccessors(&gltfParserContext, pMemoryAllocator);
                closeGltfObject(&gltfParserContext);
                parserState = parser_state_t::read_object;
                break;
            }
        }
    }

    return gltfParserContext.result;
}

bool loadGltfMeshFromMemory(const uint8_t* pGltfBuffer, const uint32_t gltfBufferSizeInBytes, memory_allocator_t* pMemoryAllocator, const char* pGltfBasePath = nullptr)
{
    gltf_description_t gltfDescription = {};
    const gltf_load_result_t loadResult = loadGltfDescriptionFromBuffer(&gltfDescription, pGltfBuffer, gltfBufferSizeInBytes, pMemoryAllocator);
    return loadResult.success;
}

gltf_mesh_t* loadGltfMesh(const char* pGltfFilePath, memory_allocator_t* pMemoryAllocator)
{
    const char* pAbsoluteFilePath = pGltfFilePath;
    const bool absoluteFilePath = isAbsoluteFilePath(pAbsoluteFilePath);
    if(!absoluteFilePath)
    {
        pAbsoluteFilePath = getAbsoluteFilePath(pGltfFilePath, pMemoryAllocator);
    }

    gltf_mesh_t* pMesh = nullptr;

    file_mapping_t gltfFileMapping;
    if(!mapFileForReading(&gltfFileMapping, pAbsoluteFilePath))
    {
        goto cleanup_and_exit;
    }

    loadGltfMeshFromMemory(gltfFileMapping.pFileBaseAddress, gltfFileMapping.fileSizeInBytes, pMemoryAllocator, pGltfFilePath);
    unmapFileMapping(&gltfFileMapping);

cleanup_and_exit:
    if(absoluteFilePath)
    {
        freeFromAllocator(pMemoryAllocator, (void*)pAbsoluteFilePath);
        pAbsoluteFilePath = nullptr;
    }

    return pMesh;
}

bool initSponzaSample(sample_frame_parameter_t* pFrameParameter)
{
    gltf_mesh_t* pMesh = loadGltfMesh("Box.gltf", pFrameParameter->pAllocator);
    if(pMesh == nullptr)
    {
        return false;
    }
    return true;
}

void shutdownSponzaSample(sample_frame_parameter_t* pFrameParameter)
{
    
}