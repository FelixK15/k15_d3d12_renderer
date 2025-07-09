
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
    UNSIGNED_BYTE = 1,
    SHORT = 2,
    UNSIGNED_SHORT = 3,
    UNSIGNED_INT = 5,
    FLOAT = 6
};

enum class gltf_alpha_t
{
    MODE_OPAQUE = 0,
    MODE_MASK,
    MODE_BLEND
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
    bool                    normalized          = false;
    int                     bufferViewIndex     = ~0;
    int                     byteOffset          = 0;
    gltf_component_type_t   componentType;
    int                     count;
    gltf_type_t             type;
};

struct gltf_attribute_t
{
    const char* pAttributeSemantic  = nullptr;
    int         accessorIndex       = ~0;
};

struct gltf_primitive_t
{
    gltf_attribute_t attributes[8];
    int attributeCount              = 0;
    int indices                     = 0;
    int mode                        = 4;
    int materialIndex               = 0;
};

struct gltf_mesh_t
{
    int                 numPrimitives;
    gltf_primitive_t*   pPrimitives;
    const char*         pName;
};

struct gltf_texture_info_t
{
    int textureIndex    = ~0;
    int texCoordIndex   = 0;
};

struct gltf_occlusion_texture_info_t
{
    int textureIndex    = ~0;
    int texCoordIndex   = 0;
    double strength     = 1.0;
};

struct gltf_normal_texture_info_t
{
    int textureIndex    = ~0;
    int texCoordIndex   = 0;
    double scale        = 1.0;
};

struct gltf_material_pbr_metallic_roughness_t
{
    double                          baseColorFactor[4]          = { 1.0, 1.0, 1.0, 1.0 };
    double                          metallicFactor              = 1.0;
    double                          roughnessFactor             = 1.0;
    gltf_occlusion_texture_info_t   occlusionTexture;
    gltf_normal_texture_info_t      normalTexture;
    gltf_texture_info_t             metallicRoughnessTexture;
    gltf_texture_info_t             baseColorTexture;
};

struct gltf_material_t
{
    bool                                    doubleSided             = false;
    double                                  alphaCutoff             = 0.5;
    double                                  emissiveFactor[3]       = { 0.0, 0.0, 0.0};
    const char*                             pName                   = nullptr;
    gltf_alpha_t                            alphaMode               = gltf_alpha_t::MODE_OPAQUE;
    gltf_texture_info_t                     emissiveTexture;
    gltf_material_pbr_metallic_roughness_t  pbrMetallicRoughness;
};

struct gltf_buffer_view_t
{
    const char* pName = nullptr;
    int         bufferIndex = ~0;
    int         byteOffset = 0;
    int         byteLength = 0;
    int         byteStride = 0;
    int         target = 0;
};

struct gltf_buffer_t
{
    const char* pName   = nullptr;
    const char* pUri    = nullptr;
    int byteLength      = 0;
};

struct gltf_description_t
{
    gltf_material_t*    pMaterials      = nullptr;
    gltf_accessor_t*    pAccessors      = nullptr;
    gltf_mesh_t*        pMeshes         = nullptr;
    gltf_buffer_view_t* pBufferViews    = nullptr;
    gltf_buffer_t*      pBuffers        = nullptr;
    int                 accessorCount   = 0;
    int                 meshCount       = 0;
    int                 materialCount   = 0;
    int                 bufferCount     = 0;
    int                 bufferViewCount = 0;
};

struct gltf_object_t
{
    const void* pPos;
};

struct gltf_array_t
{
    const void* pPos;
};

struct gltf_array_iterator_t
{
    bool        hasNextMember;
    const void* pPos;
};

struct gltf_object_member_iterator_t
{
    bool        hasNextMember;
    const void* pPos;
};

static bool operator!=(const gltf_object_t& objectA, const gltf_object_t& objectB)
{
    return objectA.pPos != objectB.pPos;
}

static bool operator!=(const gltf_array_t& arrayA, const gltf_array_t& arrayB)
{
    return arrayA.pPos != arrayB.pPos;
}

static bool operator==(const gltf_object_t& objectA, const gltf_object_t& objectB)
{
    return objectA.pPos == objectB.pPos;
}

static bool operator==(const gltf_array_t& arrayA, const gltf_array_t& arrayB)
{
    return arrayA.pPos == arrayB.pPos;
}

union parse_pos_t
{
    __m256i vec;
    char    chars[32];    //for better debugging
};
static_assert(sizeof(parse_pos_t::vec) == sizeof(parse_pos_t::chars));

static constexpr gltf_object_t invalidGltfObject = { nullptr };
static constexpr gltf_array_t invalidGltfArray = { nullptr };
static constexpr gltf_object_member_iterator_t invalidGltfObjectMemberIterator = { nullptr };

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
    const void*         pStartGltfBuffer;
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

void skipGltfString(gltf_parser_context_t* pParserContext)
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

    advanceGltfParserForCharAmount(pParserContext, closeQuotesPos+1);
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
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

bool areStringsEqual32(const char* pStringA, const char* pStringsB)
{
    const __m256i stringA = _mm256_loadu_epi8(pStringA);
    const __m256i stringB = _mm256_loadu_epi8(pStringsB);

    const int stringMask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(stringA, _mm256_setzero_si256()));
    unsigned long stringLength = 0;
    _BitScanForward(&stringLength, stringMask);

    const __m256i compareResult = _mm256_cmpeq_epi8(stringA, stringB);
    const int compareMask = _mm256_movemask_epi8(compareResult);
    const int validBitsMask = (1 << stringLength) - 1;
    const int matchingChars = __popcnt(compareMask & validBitsMask);
    return matchingChars == stringLength;
}

parser_state_t mapObjectNameToParserState(const char* pObjectName)
{
    #if 0
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
    #endif
    return parser_state_t::read_accessors;
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
        //setGltfParserContextError(pParserContext, "Internal gtlf object type stack overflow");
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

void skipNextGltfToken(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    advanceGltfParserForCharAmount(pParserContext, 1);
}

gltf_object_t openGltfObject(gltf_parser_context_t* pParserContext)
{
    if(isInvalidParserState(pParserContext))
    {
        return invalidGltfObject;
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
    skipNextGltfToken(pParserContext);
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    //advanceGltfParserForCharAmount(pParserContext, 1);

    return { (const void*)pParserContext->pCurrentParsePos };
}


void setGltfParserPosition(gltf_parser_context_t* pParserContext, const void* pPos)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pPos >= pParserContext->pEndGltfBuffer || pPos < pParserContext->pStartGltfBuffer)
    {
        const ptrdiff_t offset = (ptrdiff_t)pPos - (ptrdiff_t)pParserContext->pStartGltfBuffer;
        setGltfParserContextError(pParserContext, "Trying to jump to out-of-bounds position during setGltfParserPosition for offset '%lld' (pos: %llx)", offset, pPos);
        return;
    }

    pParserContext->pCurrentParsePos = (const parse_pos_t*)pPos;
}

void skipNextGltfProperty(gltf_parser_context_t* pParserContext)
{
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    if(isNextGltfToken(pParserContext, '"'))
    {
        skipGltfString(pParserContext);
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }

    if(isNextGltfToken(pParserContext, ':'))
    {
        skipNextGltfToken(pParserContext);
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }
    
    if(isNextGltfToken(pParserContext, '['))
    {
        skipNextGltfToken(pParserContext);
        skipGltfBracketedProperty(pParserContext, '[', ']');
    }
    else if(isNextGltfToken(pParserContext, '{'))
    {
        skipNextGltfToken(pParserContext);
        skipGltfBracketedProperty(pParserContext, '{', '}');
    }
    else
    {
        advanceGtlfParserToNextSpaceOrComma(pParserContext);
    }
}

bool isNextGltfTokenNumerical(const gltf_parser_context_t* pParserContext)
{
    return pParserContext->pCurrentParsePos->chars[0] >= '0' && pParserContext->pCurrentParsePos->chars[0] <= '9';
}

bool isNextGltfTokenWhitespace(const gltf_parser_context_t* pParserContext)
{
    if(pParserContext->pCurrentParsePos->chars[0] == ' ' || 
       pParserContext->pCurrentParsePos->chars[0] == '\t' || 
       pParserContext->pCurrentParsePos->chars[0] == '\n' ||
       pParserContext->pCurrentParsePos->chars[0] == '\r')
    {
        return true;
    }

    return false;
}

gltf_array_iterator_t openGltfArrayIterator(const gltf_array_t array)
{
    return { true, array.pPos };
}

bool hasMoreObjectsInGltfArray(const gltf_array_iterator_t* pIterator)
{
    if(pIterator->pPos == nullptr)
    {
        return false;
    }

    return pIterator->hasNextMember;
}

void moveToNextObjectInArray(gltf_parser_context_t* pParserContext, gltf_array_iterator_t* pIterator)
{
    setGltfParserPosition(pParserContext, pIterator->pPos);
    skipNextGltfProperty(pParserContext);

    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(isNextGltfTokenWhitespace(pParserContext))
    {
        pIterator->hasNextMember = false;
    }
    else
    {
        skipNextGltfToken(pParserContext);
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }

    pIterator->pPos = pParserContext->pCurrentParsePos;
}

gltf_object_member_iterator_t openGltfObjectMemberIterator(const gltf_object_t object)
{
    if(object.pPos == nullptr)
    {
        return { false, nullptr };
    }

    return { true, object.pPos };
}

bool hasNextGltfObjectMember(const gltf_object_member_iterator_t* pIterator)
{
    if(pIterator->pPos == nullptr)
    {
        return nullptr;
    }

    return pIterator->hasNextMember;
}

const char* readGltfObjectMemberName(gltf_parser_context_t* pParserContext, const gltf_object_member_iterator_t* pIterator)
{
    if(isInvalidParserState(pParserContext))
    {
        return nullptr;
    }

    if(pIterator->pPos == nullptr)
    {
        return nullptr;
    }

    setGltfParserPosition(pParserContext, pIterator->pPos);
    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '\"');

    char* pMemberName = nullptr;
    readGltfString(pParserContext, &pMemberName);

    return pMemberName;
}

gltf_array_t openGltfArrayMember(gltf_parser_context_t* pParserContext, const gltf_object_member_iterator_t* pIterator)
{
    setGltfParserPosition(pParserContext, pIterator->pPos);
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    if(isNextGltfToken(pParserContext, '"'))
    {
        skipGltfString(pParserContext);
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }

    if(!isNextGltfToken(pParserContext, ':'))
    {
        setGltfParserContextError(pParserContext, "Expected token ':' but got '%c'", pParserContext->pCurrentParsePos->chars[0]);
        return invalidGltfArray;
    }

    skipNextGltfToken(pParserContext);
    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '[');
    if(isInvalidParserState(pParserContext))
    {
        return invalidGltfArray;
    }

    return { (const void*)pParserContext->pCurrentParsePos };
}

void moveToNextGltfObjectMember(gltf_parser_context_t* pParserContext, gltf_object_member_iterator_t* pIterator)
{
    setGltfParserPosition(pParserContext, pIterator->pPos);
    skipNextGltfProperty(pParserContext);

    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(isNextGltfTokenWhitespace(pParserContext))
    {
        pIterator->hasNextMember = false;
    }
    else
    {
        skipNextGltfToken(pParserContext);
        advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    }

    pIterator->pPos = pParserContext->pCurrentParsePos;
}

gltf_object_t openGltfObjectMemberObject(gltf_parser_context_t* pParserContext, gltf_object_member_iterator_t* pIterator)
{
    ASSERT_DEBUG(pIterator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return invalidGltfObject;
    }

    setGltfParserPosition(pParserContext, pIterator->pPos);
    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '\"');

    char* pMemberName = nullptr;
    readGltfString(pParserContext, &pMemberName);

    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    if(!isNextGltfToken(pParserContext, '{'))
    {
        setGltfParserContextError(pParserContext, "Property '%s' is not an object");
        return invalidGltfObject;
    }

    return openGltfObject(pParserContext);
}

gltf_object_t openGltfObjectMemberObject(gltf_parser_context_t* pParserContext, gltf_object_t rootObject, const char* pObjectName)
{
    ASSERT_DEBUG(rootObject != invalidGltfObject);
    ASSERT_DEBUG(pObjectName != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return invalidGltfObject;
    }

    setGltfParserPosition(pParserContext, rootObject.pPos);
    gltf_object_member_iterator_t memberIterator = openGltfObjectMemberIterator(rootObject);
    while(hasNextGltfObjectMember(&memberIterator))
    {
        const char* pMemberName = readGltfObjectMemberName(pParserContext, &memberIterator);
        if(areStringsEqual32(pMemberName, pObjectName))
        {
            advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
            advanceGltfParserToNextNonWhitespaceToken(pParserContext);

            if(!isNextGltfToken(pParserContext, '{'))
            {
                setGltfParserContextError(pParserContext, "Property '%s' is not an object");
                return invalidGltfObject;
            }

            return openGltfObject(pParserContext);
        }

        moveToNextGltfObjectMember(pParserContext, &memberIterator);
    }

    return invalidGltfObject;
}

int readGltfObjectMemberInteger(gltf_parser_context_t* pParserContext, gltf_object_member_iterator_t* pIterator)
{
    ASSERT_DEBUG(pParserContext != nullptr);
    ASSERT_DEBUG(pIterator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return ~0;
    }

    setGltfParserPosition(pParserContext, pIterator->pPos);

    char* pMemberName = nullptr;
    readGltfString(pParserContext, &pMemberName);
    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    if(!isNextGltfTokenNumerical(pParserContext))
    {
        setGltfParserContextError(pParserContext, "Property '%s' is not a number", pMemberName);
        return ~0;
    }

    int integer = 0;
    readGltfInteger(pParserContext, &integer);
    return integer;
}


int readGltfObjectMemberInteger(gltf_parser_context_t* pParserContext, gltf_object_t rootObject, const char* pIntMemberName)
{
    ASSERT_DEBUG(pParserContext != nullptr);
    ASSERT_DEBUG(rootObject != invalidGltfObject);
    ASSERT_DEBUG(pIntMemberName != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return ~0;
    }

    setGltfParserPosition(pParserContext, rootObject.pPos);
    gltf_object_member_iterator_t memberIterator = openGltfObjectMemberIterator(rootObject);
    while(hasNextGltfObjectMember(&memberIterator))
    {
        const char* pMemberName = readGltfObjectMemberName(pParserContext, &memberIterator);
        if(areStringsEqual32(pMemberName, pIntMemberName))
        {
            return readGltfObjectMemberInteger(pParserContext, &memberIterator);
        }

        moveToNextGltfObjectMember(pParserContext, &memberIterator);
    }

    return ~0;
}

const char* readGltfObjectMemberString(gltf_parser_context_t* pParserContext, gltf_object_member_iterator_t* pObjectMemberIterator)
{
    ASSERT_DEBUG(pParserContext != nullptr);
    ASSERT_DEBUG(pObjectMemberIterator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return nullptr;
    }

    setGltfParserPosition(pParserContext, pObjectMemberIterator->pPos);

    char* pMemberName = nullptr;
    readGltfString(pParserContext, &pMemberName);
    advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);

    if(!isNextGltfToken(pParserContext, '\"'))
    {
        setGltfParserContextError(pParserContext, "Property '%s' is not a string", pMemberName);
        return nullptr;
    }

    char* pString = nullptr;
    readGltfString(pParserContext, &pString);
    return pString;
}

const char* readGltfObjectMemberString(gltf_parser_context_t* pParserContext, gltf_object_t rootObject, const char* pStringMemberName)
{
    ASSERT_DEBUG(rootObject != invalidGltfObject);
    ASSERT_DEBUG(pStringMemberName != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return nullptr;
    }

    setGltfParserPosition(pParserContext, rootObject.pPos);
    gltf_object_member_iterator_t memberIterator = openGltfObjectMemberIterator(rootObject);
    while(hasNextGltfObjectMember(&memberIterator))
    {
        const char* pMemberName = readGltfObjectMemberName(pParserContext, &memberIterator);
        if(areStringsEqual32(pMemberName, pStringMemberName))
        {
            advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
            advanceGltfParserToNextNonWhitespaceToken(pParserContext);

            if(!isNextGltfToken(pParserContext, '"'))
            {
                setGltfParserContextError(pParserContext, "Property '%s' is not a string");
                return nullptr;
            }
            char* pString = nullptr;
            readGltfString(pParserContext, &pString);
            return pString;
        }

        moveToNextGltfObjectMember(pParserContext, &memberIterator);
    }

    return nullptr;
}

gltf_array_t openGltfArray(gltf_parser_context_t* pParserContext, const gltf_object_t rootObject, const char* pArrayName)
{
    setGltfParserPosition(pParserContext, rootObject.pPos);
    gltf_object_member_iterator_t memberIterator = openGltfObjectMemberIterator(rootObject);
    while(hasNextGltfObjectMember(&memberIterator))
    {
        const char* pMemberName = readGltfObjectMemberName(pParserContext, &memberIterator);
        if(areStringsEqual32(pMemberName, pArrayName))
        {
            return openGltfArrayMember(pParserContext, &memberIterator);
        }

        moveToNextGltfObjectMember(pParserContext, &memberIterator);
    }

    return invalidGltfArray;
}

int readGltfArrayObjectCount(gltf_parser_context_t* pParserContext, const gltf_array_t array)
{
    if(isInvalidParserState(pParserContext))
    {
        return 0;
    }

    if(array.pPos == nullptr)
    {
        return 0;
    }

    setGltfParserPosition(pParserContext, array.pPos);

    const parse_pos_t* pCurrentParsePos = pParserContext->pCurrentParsePos;
    int objectCount = 0;

    gltf_array_iterator_t arrayIterator = openGltfArrayIterator(array);
    while(hasMoreObjectsInGltfArray(&arrayIterator))
    {
        ++objectCount;
        moveToNextObjectInArray(pParserContext, &arrayIterator);
    }

    return objectCount;
}

#if 0
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
#endif

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

gltf_object_t openCurrentGltfArrayIteratorObject(gltf_parser_context_t* pParserContext, gltf_array_iterator_t* pArrayIterator)
{
    setGltfParserPosition(pParserContext, pArrayIterator->pPos);
    return openGltfObject(pParserContext);
}

void readGltfObjectCountInArrayWithoutAdvancingParser(gltf_parser_context_t* pParserContext, int* pOutObjectCount)
{
    #if 0
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
    #endif
    return;
}

void readGltfMeshPrimitiveAttributes(gltf_parser_context_t* pParserContext, gltf_primitive_t* pPrimitive)
{
    #if 0
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
    #endif
}

void readGltfMeshPrimitive(gltf_parser_context_t* pParserContext, gltf_primitive_t* pPrimitive, gltf_object_t primitive)
{
    pPrimitive->indices = readGltfObjectMemberInteger(pParserContext, primitive, "indices");
    pPrimitive->mode = readGltfObjectMemberInteger(pParserContext, primitive, "mode");
    pPrimitive->materialIndex = readGltfObjectMemberInteger(pParserContext, primitive, "material");

    gltf_object_t attributes = openGltfObjectMemberObject(pParserContext, primitive, "attributes");
    if(attributes == invalidGltfObject)
    {
        return;
    }

    int attributeIndex = 0;

    gltf_object_member_iterator_t attributeIterator = openGltfObjectMemberIterator(attributes);
    while(hasNextGltfObjectMember(&attributeIterator))
    {
        const char* pAttributeName = readGltfObjectMemberName(pParserContext, &attributeIterator);
        const int accessorIndex = readGltfObjectMemberInteger(pParserContext, &attributeIterator);

        pPrimitive->attributes[attributeIndex].accessorIndex = accessorIndex;
        pPrimitive->attributes[attributeIndex].pAttributeSemantic = pAttributeName;
        moveToNextGltfObjectMember(pParserContext, &attributeIterator);
    }
}

void readGltfMeshPrimitives(gltf_parser_context_t* pParserContext, gltf_mesh_t* pMesh, const gltf_array_t array, memory_allocator_t* pMemoryAllocator)
{
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    int numPrimitives = readGltfArrayObjectCount(pParserContext, array);
    if(numPrimitives == 0)
    {
        pMesh->numPrimitives = 0;
        return;
    }

    gltf_primitive_t* pPrimitives = (gltf_primitive_t*)allocateFromAllocator(&pParserContext->memoryAllocator, sizeof(gltf_primitive_t) * numPrimitives);
    if(pPrimitives == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %u primitives", numPrimitives);
        return;
    }

    pMesh->pPrimitives = pPrimitives;
    pMesh->numPrimitives = numPrimitives;

    gltf_array_iterator_t primitivesIterator = openGltfArrayIterator(array);
    int primitiveIndex = 0;
    while(hasMoreObjectsInGltfArray(&primitivesIterator))
    {
        gltf_object_t meshPrimitive = openCurrentGltfArrayIteratorObject(pParserContext, &primitivesIterator);
        readGltfMeshPrimitive(pParserContext, &pMesh->pPrimitives[primitiveIndex], meshPrimitive);
        moveToNextObjectInArray(pParserContext, &primitivesIterator);
        ++primitiveIndex;
    };
    
    return;
}

void readGltfMeshes(gltf_parser_context_t* pParserContext, gltf_array_t meshArray, memory_allocator_t* pMemoryAllocator)
{
    ASSERT_DEBUG(meshArray != invalidGltfArray);
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->meshCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'meshes' objects in gltf file.");
        return;
    }

    setGltfParserPosition(pParserContext, meshArray.pPos);

    const int numMeshes = readGltfArrayObjectCount(pParserContext, meshArray);

    gltf_mesh_t* pMeshes = (gltf_mesh_t*)allocateFromAllocator(&pParserContext->memoryAllocator, sizeof(gltf_mesh_t) * numMeshes);
    if(pMeshes == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %u meshes", numMeshes);
        return;
    }

    pParserContext->pGltfDescription->pMeshes = pMeshes;
    pParserContext->pGltfDescription->meshCount = numMeshes;

    int meshIndex = 0;
    gltf_array_iterator_t meshIterator = openGltfArrayIterator(meshArray);
    while(hasMoreObjectsInGltfArray(&meshIterator))
    {
        gltf_object_t meshObject = openCurrentGltfArrayIteratorObject(pParserContext, &meshIterator);
        pMeshes[meshIndex].pName = readGltfObjectMemberString(pParserContext, meshObject, "name");
        
        gltf_array_t primitives = openGltfArray(pParserContext, meshObject, "primitives");
        readGltfMeshPrimitives(pParserContext, &pMeshes[meshIndex], primitives, pMemoryAllocator);
        
        //closeGltfObject(meshObject);
        moveToNextObjectInArray(pParserContext, &meshIterator);

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

void readGltfAccessors(gltf_parser_context_t* pParserContext, gltf_array_t accessors, memory_allocator_t* pMemoryAllocator)
{
    ASSERT_DEBUG(accessors != invalidGltfArray);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->accessorCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'accessor' objects in gltf file.");
        return;
    }

    const int numAccessors = readGltfArrayObjectCount(pParserContext, accessors);
    if(numAccessors == 0)
    {
        return;
    }

    gltf_accessor_t* pAccessors = (gltf_accessor_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_accessor_t) * numAccessors);
    if(pAccessors == nullptr)
    {
        pParserContext->pGltfDescription->accessorCount = 0;
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %d accessors", numAccessors);
        return;
    }

    pParserContext->pGltfDescription->pAccessors = pAccessors;
    pParserContext->pGltfDescription->accessorCount = numAccessors;

    int accessorIndex = 0;
    gltf_array_iterator_t accessorIterator = openGltfArrayIterator(accessors);
    while(hasMoreObjectsInGltfArray(&accessorIterator))
    {
        if(isInvalidParserState(pParserContext))
        {
            return;
        }

        gltf_object_t accessorObject = openCurrentGltfArrayIteratorObject(pParserContext, &accessorIterator);
        gltf_object_member_iterator_t accessorObjectMemberIterator = openGltfObjectMemberIterator(accessorObject);
        while(hasNextGltfObjectMember(&accessorObjectMemberIterator))
        {
            const char* pMemberName = readGltfObjectMemberName(pParserContext, &accessorObjectMemberIterator);
            if(areStringsEqual32(pMemberName, "bufferView"))
            {
                pAccessors[accessorIndex].bufferViewIndex = readGltfObjectMemberInteger(pParserContext, &accessorObjectMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "byteOffset"))
            {
                pAccessors[accessorIndex].byteOffset = readGltfObjectMemberInteger(pParserContext, &accessorObjectMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "componentType"))
            {
                const int componentType = readGltfObjectMemberInteger(pParserContext, &accessorObjectMemberIterator);
                pAccessors[accessorIndex].componentType = (gltf_component_type_t)(componentType - 5120);
            }
            else if(areStringsEqual32(pMemberName, "count"))
            {
                pAccessors[accessorIndex].count = readGltfObjectMemberInteger(pParserContext, &accessorObjectMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "type"))
            {
                const char* pTypeName = readGltfObjectMemberString(pParserContext, &accessorObjectMemberIterator);
                pAccessors[accessorIndex].type = parseGltfTypeName(pTypeName);

                if(pAccessors[accessorIndex].type == gltf_type_t::NONE)
                {
                    setGltfParserContextError(pParserContext, "Unknown accessor type '%s' for %d. accessor.", pTypeName, accessorIndex+1);
                    return;
                }
            }
            moveToNextGltfObjectMember(pParserContext, &accessorObjectMemberIterator);
        }

        ++accessorIndex;
        moveToNextObjectInArray(pParserContext, &accessorIterator);
    }

    return;
}

void readGltfPbrMetallicRoughness(gltf_parser_context_t* pParserContext, gltf_material_pbr_metallic_roughness_t* pPbrMetallicRoughness, gltf_object_t pbrMetallicRoughness)
{
    ASSERT_DEBUG(pbrMetallicRoughness != invalidGltfObject);
    ASSERT_DEBUG(pPbrMetallicRoughness != nullptr);

    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    gltf_object_member_iterator_t iterator = openGltfObjectMemberIterator(pbrMetallicRoughness);
    while(hasNextGltfObjectMember(&iterator))
    {
        const char* pMemberName = readGltfObjectMemberName(pParserContext, &iterator);
        if(areStringsEqual32(pMemberName, "baseColorFactor"))
        {
            gltf_array_t baseColorFactorArray = openGltfArrayMember(pParserContext, &iterator);
            //pPbrMetallicRoughness->baseColorFactor[0] = readGltfNumberFromArrayEntry(pParserContext, baseColorFactorArray, 0);
            //pPbrMetallicRoughness->baseColorFactor[1] = readGltfNumberFromArrayEntry(pParserContext, baseColorFactorArray, 1);
            //pPbrMetallicRoughness->baseColorFactor[2] = readGltfNumberFromArrayEntry(pParserContext, baseColorFactorArray, 2);
            //pPbrMetallicRoughness->baseColorFactor[3] = readGltfNumberFromArrayEntry(pParserContext, baseColorFactorArray, 3);
        }
        else if(areStringsEqual32(pMemberName, "metallicFactor"))
        {
            //pPbrMetallicRoughness->metallicFactor = readGltfNumber(pParserContext, &iterator);
        }
        moveToNextGltfObjectMember(pParserContext, &iterator);
    }
}

void readGltfMaterials(gltf_parser_context_t* pParserContext, gltf_array_t materials, memory_allocator_t* pMemoryAllocator)
{
    ASSERT_DEBUG(materials != invalidGltfArray);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->materialCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'materials' objects in gltf file.");
        return;
    }

    const int numMaterials = readGltfArrayObjectCount(pParserContext, materials);
    if(numMaterials == 0)
    {
        return;
    }

    gltf_material_t* pMaterials = (gltf_material_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_material_t) * numMaterials);
    if(pMaterials == nullptr)
    {
        pParserContext->pGltfDescription->materialCount = 0;
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %d materials", numMaterials);
        return;
    }

    pParserContext->pGltfDescription->pMaterials = pMaterials;
    pParserContext->pGltfDescription->materialCount = numMaterials;

    int materialIndex = 0;
    gltf_array_iterator_t materialIterator = openGltfArrayIterator(materials);
    while(hasMoreObjectsInGltfArray(&materialIterator))
    {
        gltf_object_t material = openCurrentGltfArrayIteratorObject(pParserContext, &materialIterator);
        gltf_object_member_iterator_t materialMemberIterator = openGltfObjectMemberIterator(material);
        while(hasNextGltfObjectMember(&materialMemberIterator))
        {
            const char* pMemberName = readGltfObjectMemberName(pParserContext, &materialMemberIterator);
            if(areStringsEqual32(pMemberName, "pbrMetallicRoughness"))
            {
                gltf_object_t pbrMetallicRoughness = openGltfObjectMemberObject(pParserContext, &materialMemberIterator);
                readGltfPbrMetallicRoughness(pParserContext, &pMaterials[materialIndex].pbrMetallicRoughness, pbrMetallicRoughness);
            }
            else if(areStringsEqual32(pMemberName, "name"))
            {
                pMaterials[materialIndex].pName = readGltfObjectMemberString(pParserContext, &materialMemberIterator);
            }
            moveToNextGltfObjectMember(pParserContext, &materialMemberIterator);
        }

        ++materialIndex;
        moveToNextObjectInArray(pParserContext, &materialIterator);
    }
}

void readGltfBufferViews(gltf_parser_context_t* pParserContext, gltf_array_t bufferViews, memory_allocator_t* pMemoryAllocator)
{
    ASSERT_DEBUG(bufferViews != invalidGltfArray);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->bufferViewCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'bufferView' objects in gltf file.");
        return;
    }

    const int numBufferViews = readGltfArrayObjectCount(pParserContext, bufferViews);
    if(numBufferViews == 0)
    {
        return;
    }

    gltf_buffer_view_t* pBufferViews = (gltf_buffer_view_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_buffer_view_t) * numBufferViews);
    if(pBufferViews == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %d bufferViews", numBufferViews);
        return;
    }

    pParserContext->pGltfDescription->pBufferViews = pBufferViews;
    pParserContext->pGltfDescription->bufferViewCount = numBufferViews;

    int bufferViewIndex = 0;
    gltf_array_iterator_t bufferViewIterator = openGltfArrayIterator(bufferViews);
    while(hasMoreObjectsInGltfArray(&bufferViewIterator))
    {
        gltf_object_t bufferView = openCurrentGltfArrayIteratorObject(pParserContext, &bufferViewIterator);
        gltf_object_member_iterator_t bufferViewMemberIterator = openGltfObjectMemberIterator(bufferView);
        while(hasNextGltfObjectMember(&bufferViewMemberIterator))
        {
            const char* pMemberName = readGltfObjectMemberName(pParserContext, &bufferViewMemberIterator);
            if(areStringsEqual32(pMemberName, "buffer"))
            {
                pBufferViews[bufferViewIndex].bufferIndex = readGltfObjectMemberInteger(pParserContext, &bufferViewMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "byteOffset"))
            {
                pBufferViews[bufferViewIndex].byteOffset = readGltfObjectMemberInteger(pParserContext, &bufferViewMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "byteLength"))
            {
                pBufferViews[bufferViewIndex].byteLength = readGltfObjectMemberInteger(pParserContext, &bufferViewMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "target"))
            {
                pBufferViews[bufferViewIndex].target = readGltfObjectMemberInteger(pParserContext, &bufferViewMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "byteStride"))
            {
                pBufferViews[bufferViewIndex].byteStride = readGltfObjectMemberInteger(pParserContext, &bufferViewMemberIterator);
            }
            moveToNextGltfObjectMember(pParserContext, &bufferViewMemberIterator);
        }

        ++bufferViewIndex;
        moveToNextObjectInArray(pParserContext, &bufferViewIterator);
    }
}

void readGltfBuffers(gltf_parser_context_t* pParserContext, gltf_array_t buffers, memory_allocator_t* pMemoryAllocator)
{
    ASSERT_DEBUG(buffers != invalidGltfArray);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    if(isInvalidParserState(pParserContext))
    {
        return;
    }

    if(pParserContext->pGltfDescription->bufferCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'buffer' objects in gltf file.");
        return;
    }

    const int numBuffers = readGltfArrayObjectCount(pParserContext, buffers);
    if(numBuffers == 0)
    {
        return;
    }

    gltf_buffer_t* pBuffers = (gltf_buffer_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_buffer_t) * numBuffers);
    if(pBuffers == nullptr)
    {
        setGltfParserContextError(pParserContext, "Couldn't allocate memory for %d buffers", numBuffers);
        return;
    }

    pParserContext->pGltfDescription->pBuffers = pBuffers;
    pParserContext->pGltfDescription->bufferCount = numBuffers;

    int bufferIndex = 0;
    gltf_array_iterator_t bufferIterator = openGltfArrayIterator(buffers);
    while(hasMoreObjectsInGltfArray(&bufferIterator))
    {
        gltf_object_t buffer = openCurrentGltfArrayIteratorObject(pParserContext, &bufferIterator);
        gltf_object_member_iterator_t bufferMemberIterator = openGltfObjectMemberIterator(buffer);
        while(hasNextGltfObjectMember(&bufferMemberIterator))
        {
            const char* pMemberName = readGltfObjectMemberName(pParserContext, &bufferMemberIterator);
            if(areStringsEqual32(pMemberName, "byteLength"))
            {
                pBuffers[bufferIndex].byteLength = readGltfObjectMemberInteger(pParserContext, &bufferMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "uri"))
            {
                pBuffers[bufferIndex].pUri = readGltfObjectMemberString(pParserContext, &bufferMemberIterator);
            }
            else if(areStringsEqual32(pMemberName, "name"))
            {
                pBuffers[bufferIndex].pName = readGltfObjectMemberString(pParserContext, &bufferMemberIterator);
            }
            moveToNextGltfObjectMember(pParserContext, &bufferMemberIterator);
        }
        moveToNextObjectInArray(pParserContext, &bufferIterator);
    }
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
    gltfParserContext.pStartGltfBuffer = pGltfBuffer;
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

    gltf_object_t gltfRoot = openGltfObject(&gltfParserContext);
    if(gltfRoot == invalidGltfObject)
    {
        return createGltfLoadResultError("Couldn't read root object of gltf buffer", sizeof(gltf_description_t));;
    }

    gltf_object_member_iterator_t rootObjectMemberIterator = openGltfObjectMemberIterator(gltfRoot);
    while(hasNextGltfObjectMember(&rootObjectMemberIterator))
    {
        const char* pMemberName = readGltfObjectMemberName(&gltfParserContext, &rootObjectMemberIterator);
        if(areStringsEqual32(pMemberName, "meshes"))
        {
            gltf_array_t meshArray = openGltfArrayMember(&gltfParserContext, &rootObjectMemberIterator);
            readGltfMeshes(&gltfParserContext, meshArray, pMemoryAllocator);
        }
        else if(areStringsEqual32(pMemberName, "accessors"))
        {
            gltf_array_t accessorArray = openGltfArrayMember(&gltfParserContext, &rootObjectMemberIterator);
            readGltfAccessors(&gltfParserContext, accessorArray, pMemoryAllocator);
        }
        else if(areStringsEqual32(pMemberName, "materials"))
        {
            gltf_array_t materialsArray = openGltfArrayMember(&gltfParserContext, &rootObjectMemberIterator);
            readGltfMaterials(&gltfParserContext, materialsArray, pMemoryAllocator);
        }
        else if(areStringsEqual32(pMemberName, "bufferViews"))
        {
            gltf_array_t bufferViewsArray = openGltfArrayMember(&gltfParserContext, &rootObjectMemberIterator);
            readGltfBufferViews(&gltfParserContext, bufferViewsArray, pMemoryAllocator);
        }
        else if(areStringsEqual32(pMemberName, "buffers"))
        {
            gltf_array_t buffersArray = openGltfArrayMember(&gltfParserContext, &rootObjectMemberIterator);
            readGltfBuffers(&gltfParserContext, buffersArray, pMemoryAllocator);
        }

        moveToNextGltfObjectMember(&gltfParserContext, &rootObjectMemberIterator);
    };


    #if 0
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
    #endif

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
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
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

    QueryPerformanceCounter(&end);

    const LONGLONG frameTimeDelta = end.QuadPart - start.QuadPart;
    float deltaTimeInMs = ((float)frameTimeDelta / (float)freq.QuadPart) * 1000.f;
    printf("Loading '%s' took %.3fms\n", pGltfFilePath, deltaTimeInMs);
    return pMesh;
}

bool initSponzaSample(sample_frame_parameter_t* pFrameParameter)
{
    //gltf_mesh_t* pMesh = loadGltfMesh("Box.gltf", pFrameParameter->pAllocator);
    gltf_mesh_t* pMesh = loadGltfMesh("sponza.gltf", pFrameParameter->pAllocator);
    if(pMesh == nullptr)
    {
        return false;
    }
    return true;
}

void shutdownSponzaSample(sample_frame_parameter_t* pFrameParameter)
{
    
}