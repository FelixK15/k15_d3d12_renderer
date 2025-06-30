
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

struct gltf_accessor_t
{

};

struct gltf_attribute_t
{
    char attributeSemantic[15];
    uint8_t accessorIndex;
};
static_assert(sizeof(gltf_attribute_t) == 16);

struct gltf_primitive_t
{
    gltf_attribute_t attributes[8];
    uint32_t attributeCount;
    uint32_t indices;
    uint32_t mode;
    uint32_t materialIndex;
};

struct gltf_mesh_t
{
    uint32_t            numPrimitives;
    gltf_primitive_t*   pPrimitives;
    char                name[64];
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
    uint32_t            accessorCount;
    uint32_t            meshCount;
    uint32_t            materialCount;
    uint32_t            bufferCount;
};

union parse_pos_t
{
    __m256i vec;
    char    chars[32];    //for better debugging
};
static_assert(sizeof(parse_pos_t::vec) == sizeof(parse_pos_t::chars));

struct gltf_load_result_t
{
    bool success;
    char errorMessage[256];
};

struct gltf_parser_context_t
{
    int                 lineIndex;
    int                 charIndex;
    gltf_load_result_t  result;
    gltf_description_t* pGltfDescription;
    const parse_pos_t*  pCurrentParsePos;
    const void*         pEndGltfBuffer;
};

enum class parser_state_t
{
    read_object_name,
    eat_leading_whitespaces,
    skip_object,
    read_meshes,
    read_materials,
    read_buffer_views,
    read_buffers,
    read_accessors
};

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

bool advanceGltfParserForCharAmount(gltf_parser_context_t* pParserContext, const uint32_t numCharsToSkip)
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
        return false;
    }

    pParserContext->pCurrentParsePos = pNextParsePos;
    return true;
}

bool advanceGltfParserToTokenAndIgnoreAllOtherTokens(gltf_parser_context_t* pParserContext, const char tokenToFind)
{
    const __m256i token = _mm256_set1_epi8(tokenToFind);
    while(true)
    {
        const __m256i compareResult = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, token);
        const int tokenMask = _mm256_movemask_epi8(compareResult);

        if(tokenMask == 0)
        {
            if(!advanceGltfParserForCharAmount(pParserContext, 32))
            {
                return false;
            }

            continue;
        }
        
        unsigned long tokenPos = 0;
        _BitScanForward(&tokenPos, tokenMask);
        if(tokenPos == 0)
        {
            break;
        }

        if(!advanceGltfParserForCharAmount(pParserContext, tokenPos))
        {
            return false;
        }
    }

    return true;
}

bool advanceGltfParserAndIgnoreTokens(gltf_parser_context_t* pParserContext, const char* pTokensToIgnore, const int tokenCount)
{
    while(true)
    {
        __m256i filter = _mm256_setzero_si256();
        for(int i = 0; i < tokenCount; ++i)
        {
            filter = _mm256_or_si256(filter, _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, _mm256_set1_epi8(pTokensToIgnore[i])));
        }

        const unsigned int filterMask = _mm256_movemask_epi8(filter);
        if(filterMask == 0xFFFFFFFF)
        {
            if(!advanceGltfParserForCharAmount(pParserContext, 32u))
            {
                return false;
            }
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

        if(!advanceGltfParserForCharAmount(pParserContext, tokenPos))
        {
            return false;
        }
        break;
    }

    return true;
}

bool advanceGltfParserToNextNonWhitespaceToken(gltf_parser_context_t* pParserContext)
{
    const char whitespaceTokens[] = {
        ' ', '\n', '\r', '\t'
    };

    return advanceGltfParserAndIgnoreTokens(pParserContext, whitespaceTokens, ARRAY_SIZE(whitespaceTokens));
}

bool advanceGltfParserToNextLine(gltf_parser_context_t* pParserContext)
{
    if(!advanceGltfParserToTokenAndIgnoreAllOtherTokens(pParserContext, '\n'))
    {
        return false;
    }

    return advanceGltfParserForCharAmount(pParserContext, 1);
}

bool advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(gltf_parser_context_t* pParserContext, const char expectedToken)
{
    if(!advanceGltfParserToNextNonWhitespaceToken(pParserContext))
    {
        return false;
    }

    const char token = pParserContext->pCurrentParsePos->chars[0];
    if(token != expectedToken)
    {
        setGltfParserContextError(pParserContext, "Unexpected token, read '%c' but expected '%c'", token, expectedToken);
        return false;
    }

    return advanceGltfParserForCharAmount(pParserContext, 1);
}

bool readGltfObjectName(gltf_parser_context_t* pParserContext, char* pObjectNameBuffer, const uint32_t objectNameBufferLength)
{
    if(!advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '"'))
    {
        return false;
    }

    const __m256i quoteMask = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, _mm256_set1_epi8('"'));
    unsigned int quoteMaskBits = _mm256_movemask_epi8(quoteMask);
    if(__popcnt(quoteMaskBits) < 1)
    {
        setGltfParserContextError(pParserContext, "Error trying to read object name in line '%s' - couldn't find matching quotes.", pParserContext->pCurrentParsePos->chars);
        return false;
    }
    
    const __m256i objectName = _mm256_and_si256(pParserContext->pCurrentParsePos->vec, quoteMask);
    _mm256_store_si256((__m256i*)pObjectNameBuffer, objectName);
    
    unsigned long closeQuotesPos = 0;
    _BitScanForward(&closeQuotesPos, quoteMaskBits);

    if(!advanceGltfParserForCharAmount(pParserContext, closeQuotesPos))~
    {
        return false;
    }
    return advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, ':');
}

template<int STRING_COUNT>
bool areStringsEqual32(const char* pStringA, const char** pStringsB, int* pOutIndex)
{
    const __m256i stringA = _mm256_loadu_epi8(pStringA);

    for(int i = 0; i < STRING_COUNT; ++i)
    {
        const __m256i stringB = _mm256_loadu_epi8(pStringsB[i]);

        const __m256i compareResult = _mm256_cmpeq_epi8(stringA, stringB);
        const int compareMask = _mm256_movemask_epi8(compareResult);

        unsigned long lastMatchPos = 0;
        _BitScanReverse(&lastMatchPos, compareMask);

        if(pStringA[lastMatchPos] == 0)
        {
            if(STRING_COUNT != 1)
            {
                *pOutIndex = i;
            }
            return true;
        }
    }

    return false;
}

bool areStringsEqual32(const char* pStringA, const char* pStringB)
{
    return areStringsEqual32<1>(pStringA, &pStringB, nullptr);
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
    if(!areStringsEqual32<ARRAY_SIZE(pValidNames)>(pObjectName, pValidNames, &matchingIndex))
    {
        return parser_state_t::skip_object;
    }

    if(matchingIndex == ~0)
    {
        return parser_state_t::read_meshes;
    }

    return parserStateToReturnOnMatch[matchingIndex];
}

bool skipGltfBracketedProperty(gltf_parser_context_t* pParserContext, const char openBracketCharacter, const char closedBracketCharacter)
{
    const __m256i openBrackets = _mm256_set1_epi8(openBracketCharacter);
    const __m256i closeBrackets = _mm256_set1_epi8(closedBracketCharacter);

    uint32_t depth = 1;
    while(depth > 0)
    {
        const __m256i openBracketsCmpResult = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, openBrackets);
        const __m256i closingBracketsCmpResult = _mm256_cmpeq_epi8(pParserContext->pCurrentParsePos->vec, closeBrackets);
        uint32_t openingBracketsMask = _mm256_movemask_epi8(openBracketsCmpResult);
        uint32_t closingBracketsMask = _mm256_movemask_epi8(closingBracketsCmpResult);
        uint32_t openingBracketsCount = __popcnt(openingBracketsMask);
        uint32_t closingBracketsCount = __popcnt(closingBracketsMask);

        depth += openingBracketsCount;
        if(closingBracketsCount > depth)
        {
            unsigned long closingBracketIndex = 0;
            while(closingBracketsCount != depth)
            {
                _BitScanForward(&closingBracketIndex, closingBracketsMask);
                closingBracketsMask &= ~(1 << closingBracketIndex);
                --closingBracketsCount;
            }

            if(!advanceGltfParserForCharAmount(pParserContext, closingBracketIndex + 1))
            {
                return false;
            }
            break;
        }
        else if(closingBracketsCount == depth)
        {
            unsigned long closingBracketIndex = 0;
            _BitScanReverse(&closingBracketIndex, closingBracketsMask);
            
            if(!advanceGltfParserForCharAmount(pParserContext, closingBracketIndex + 1))
            {
                return false;
            }
            break;
        }

        depth -= closingBracketsCount;
        advanceGltfParserForCharAmount(pParserContext, 32);
    }

    return true;
}

bool skipGltfObject(gltf_parser_context_t* pParserContext)
{
    return skipGltfBracketedProperty(pParserContext, '{', '}');
}

bool skipGltfArray(gltf_parser_context_t* pParserContext)
{
    return skipGltfBracketedProperty(pParserContext, '[', ']');
}

bool skipGltfProperty(gltf_parser_context_t* pParserContext)
{
    advanceGltfParserToNextNonWhitespaceToken(pParserContext);
    const bool isObject = advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '{');
    if(isObject) 
    {
        return skipGltfObject(pParserContext);
    }
    
    const bool isArray = advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(pParserContext, '[');
    if(isArray)
    {
        return skipGltfArray(pParserContext);
    }

    return advanceGltfParserToNextLine(pParserContext);
}

const uint32_t readGltfObjectCountInArrayWithoutAdvancingParser(gltf_parser_context_t* pParserContext)
{
    #if 0
    if(pParserContext->pCurrentParsePos->chars[0] != '[')
    {
        return 0u;
    }

    const parse_pos_t* pCurrentParsePos = pParserContext->pCurrentParsePos;
    
    uint32_t objectCount = 0;
    if(!skipGltfUntilAfterCharToken(pParserContext, '{'))
    {
        return 0;
    }

    pParserContext->pCurrentParsePos = (parse_pos_t*)((char*)pParserContext->pCurrentParsePos - 1);
    do
    {
        if(!skipGltfObject(pParserContext))
        {
            return 0;
        }
        ++objectCount;  
    } while(pParserContext->pCurrentParsePos->chars[0] == ',');

    pParserContext->pCurrentParsePos = pCurrentParsePos;
    return objectCount;
    #else

    return 0;
    #endif
}

bool readGltfMeshes(gltf_parser_context_t* pParserContext, memory_allocator_t* pMemoryAllocator)
{
    if(pParserContext->pGltfDescription->meshCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'meshes' objects in gltf file.");
        return false;
    }

    const uint32_t numMeshes = readGltfObjectCountInArrayWithoutAdvancingParser(pParserContext);
    pParserContext->pGltfDescription->pMeshes = (gltf_mesh_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_mesh_t) * numMeshes);
    pParserContext->pGltfDescription->meshCount = numMeshes;

    char propertyName[64] = {};
    uint32_t meshIndex = 0;
    while(true)
    {
        if(meshIndex == numMeshes)
        {
            break;
        }

        if(!readGltfObjectName(pParserContext, propertyName, sizeof(propertyName)))
        {
            return false;
        }

        if(areStringsEqual32(propertyName, "primitives"))
        {

        }
        else if(areStringsEqual32(propertyName, "name"))
        {

        }
        else
        {
            if(!skipGltfObject(pParserContext))
            {
                return false;
            }
        }

        propertyName[0] = 0;
    }

    return true;
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

gltf_load_result_t loadGltfDescriptionFromBuffer(gltf_description_t* pOutDescription, const uint8_t* pGltfBuffer, const uint32_t gltfBufferSize, memory_allocator_t* pMemoryAllocator)
{
    parser_state_t parserState = parser_state_t::read_object_name;

    gltf_parser_context_t gltfParserContext = {};
    gltfParserContext.pCurrentParsePos = (parse_pos_t*)pGltfBuffer;
    gltfParserContext.pEndGltfBuffer = pGltfBuffer + gltfBufferSize;
    gltfParserContext.pGltfDescription = (gltf_description_t*)allocateFromAllocator(pMemoryAllocator, sizeof(gltf_description_t), alloc_flags_t::clear_memory);
    gltfParserContext.lineIndex = 0;
    gltfParserContext.charIndex = 0;
    if(gltfParserContext.pGltfDescription == nullptr)
    {
        return createGltfLoadResultError("Out of memory trying to allocate %d bytes for gltf descriptor", sizeof(gltf_description_t));
    }

    if(!advanceGltfParserToExpectedTokenAndIgnoreWhitespaces(&gltfParserContext, '{'))
    {
        return gltfParserContext.result;
    }

    while(gltfParserContext.pCurrentParsePos < gltfParserContext.pEndGltfBuffer)
    {
        switch(parserState)
        {
            case parser_state_t::read_object_name:
            {
                char objectName[64] = {};
                if(!readGltfObjectName(&gltfParserContext, objectName, sizeof(objectName)))
                {
                    break;
                }
                parserState = mapObjectNameToParserState(objectName);
                break;
            }

            case parser_state_t::skip_object:
            {
                if(!skipGltfObject(&gltfParserContext))
                {
                    break;
                }
                parserState = parser_state_t::read_object_name;
                break;
            }

            case parser_state_t::read_meshes:
            {
                if(!readGltfMeshes(&gltfParserContext, pMemoryAllocator))
                {
                    break;
                }
                parserState = parser_state_t::eat_leading_whitespaces;
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