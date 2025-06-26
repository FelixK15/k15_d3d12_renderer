
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
static_assert(sizeof(__m256i) == sizeof(parse_pos_t::chars));

struct gltf_parser_context_t
{
    bool                success;
    gltf_description_t* pGltfDescription;
    const parse_pos_t*  pCurrentParsePos;
    const void*         pEndGltfBuffer;
    char                errorMessage[256];
};

struct gltf_load_result_t
{
    bool success;
    char errorMessage[256];
};
static_assert(sizeof(gltf_load_result_t::errorMessage) == sizeof(gltf_parser_context_t::errorMessage));

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

void setGltfParserContextError(gltf_parser_context_t* pParseContext, const char* pFormat, ...)
{
    pParseContext->success = false;

    va_list vaList;
    va_start(vaList, pFormat);
    vsprintf_s(pParseContext->errorMessage, sizeof(gltf_parser_context_t::errorMessage), pFormat, vaList);
    va_end(vaList);
}

bool skipGltfForCharAmount(gltf_parser_context_t* pParseContext, const uint32_t numCharsToSkip)
{
    const parse_pos_t* pNextParsePos = (parse_pos_t*)((char*)pParseContext->pCurrentParsePos + numCharsToSkip);
    if(pNextParsePos >= pParseContext->pEndGltfBuffer)
    {
        setGltfParserContextError(pParseContext, "Trying to read past gltf buffer - current parse pos: '%s'", pParseContext->pCurrentParsePos->chars);
        return false;
    }

    pParseContext->pCurrentParsePos = pNextParsePos;
    return true;
}

bool skipGltfUntilCharToken(gltf_parser_context_t* pParseContext, const char token)
{
    while(true)
    {
        const __m256i tokenResult = _mm256_cmpeq_epi8(pParseContext->pCurrentParsePos->vec, _mm256_set1_epi8(token));
        unsigned int tokenMask = _mm256_movemask_epi8(tokenResult);
        if(tokenMask == 0)
        {
            const parse_pos_t* pNextParsePos = pParseContext->pCurrentParsePos + 1;
            if(pNextParsePos >= pParseContext->pEndGltfBuffer)
            {
                setGltfParserContextError(pParseContext, "Trying to read past gltf buffer - current parse pos: '%s'", pParseContext->pCurrentParsePos->chars);
                return false;
            }

            pParseContext->pCurrentParsePos = pNextParsePos;
            continue;
        }
        else
        {
            unsigned long tokenPos = 0;
            _BitScanForward(&tokenPos, tokenMask);
            return skipGltfForCharAmount(pParseContext, tokenPos);
        }
    }
    
    return false;
}

bool skipGltfUntilAfterCharToken(gltf_parser_context_t* pParseContext, const char token)
{
    if(!skipGltfUntilCharToken(pParseContext, token))
    {
        return false;
    }

    const parse_pos_t* pNextParsePos = (parse_pos_t*)((char*)pParseContext->pCurrentParsePos + 1);
    if(pNextParsePos == pParseContext->pEndGltfBuffer)
    {
        setGltfParserContextError(pParseContext, "Trying to read past gltf buffer - current parse pos: '%s'", pParseContext->pCurrentParsePos->chars);
        return false;
    }

    pParseContext->pCurrentParsePos = pNextParsePos;
    return true;
}

bool readGltfObjectName(gltf_parser_context_t* pParseContext, char* pObjectNameBuffer, const uint32_t objectNameBufferLength)
{
    if(!skipGltfUntilCharToken(pParseContext, '"'))
    {
        return false;
    }

    const __m256i quoteMask = _mm256_cmpeq_epi8(pParseContext->pCurrentParsePos->vec, _mm256_set1_epi8('"'));
    unsigned int quoteMaskBits = _mm256_movemask_epi8(quoteMask);
    if(__popcnt(quoteMaskBits) < 2)
    {
        setGltfParserContextError(pParseContext, "Error trying to read object name in line '%s' - couldn't find matching quotes.", pParseContext->pCurrentParsePos->chars);
        return false;
    }

    unsigned long openQuoteBitPos = 0;
    unsigned long closeQuoteBitPos = 0;
    _BitScanForward(&openQuoteBitPos, quoteMaskBits);
    quoteMaskBits &= ~(1 << openQuoteBitPos);
    _BitScanForward(&closeQuoteBitPos, quoteMaskBits);

    if(openQuoteBitPos == closeQuoteBitPos)
    {
        setGltfParserContextError(pParseContext, "Error trying to read object name in line '%s'.", pParseContext->pCurrentParsePos->chars);
        return false;
    }

    const uint32_t objectNameLength = (closeQuoteBitPos - openQuoteBitPos) - 1;
    if(objectNameLength > objectNameBufferLength)
    {
        setGltfParserContextError(pParseContext, "Error trying to read object name in line '%s' - object name is too long.", pParseContext->pCurrentParsePos->chars);
        return false;
    }

    memcpy(pObjectNameBuffer, pParseContext->pCurrentParsePos->chars + openQuoteBitPos + 1, objectNameLength);
    return skipGltfUntilAfterCharToken(pParseContext, ':');
}

parser_state_t mapObjectNameToParserState(const char* pObjectName)
{
    if(strcmp(pObjectName, "meshes") == 0)
    {
        return parser_state_t::read_meshes;
    }
    else if(strcmp(pObjectName, "materials") == 0)
    {
        return parser_state_t::read_materials;
    }
    else if(strcmp(pObjectName, "bufferViews") == 0)
    {
        return parser_state_t::read_buffer_views;
    }
    else if(strcmp(pObjectName, "buffers") == 0)
    {
        return parser_state_t::read_buffers;
    }
    else if(strcmp(pObjectName, "accessors") == 0)
    {
        return parser_state_t::read_accessors;
    }

    return parser_state_t::skip_object;
}

bool skipGltfObject(gltf_parser_context_t* pParseContext)
{
    if(pParseContext->pCurrentParsePos->chars[0] == ' ')
    {
        if(!skipGltfUntilAfterCharToken(pParseContext, ' '))
        {
            return false;
        }
    }

    const bool isObjectOrArray = pParseContext->pCurrentParsePos->chars[0] == '{' || pParseContext->pCurrentParsePos->chars[0] == '[';
    if(!isObjectOrArray)
    {
        return skipGltfUntilAfterCharToken(pParseContext, '\n');
    }

    if(!skipGltfForCharAmount(pParseContext, 1))
    {
        return false;
    }

    const __m256i openCurlyBrackets = _mm256_set1_epi8('{');
    const __m256i closeCurlyBrackets = _mm256_set1_epi8('}');
    const __m256i openSquareBrackets = _mm256_set1_epi8('[');
    const __m256i closingSquareBracets = _mm256_set1_epi8(']');

    uint32_t objectDepth = 1;
    while(objectDepth > 0)
    {
        const __m256i openBracketsCmpResult = _mm256_or_si256(_mm256_cmpeq_epi8(pParseContext->pCurrentParsePos->vec, openCurlyBrackets), _mm256_cmpeq_epi8(pParseContext->pCurrentParsePos->vec, openSquareBrackets));
        const __m256i closingBracketsCmpResult = _mm256_or_si256(_mm256_cmpeq_epi8(pParseContext->pCurrentParsePos->vec, closeCurlyBrackets), _mm256_cmpeq_epi8(pParseContext->pCurrentParsePos->vec, closingSquareBracets));
        uint32_t openingBracketsMask = _mm256_movemask_epi8(openBracketsCmpResult);
        uint32_t closingBracketsMask = _mm256_movemask_epi8(closingBracketsCmpResult);
        uint32_t openingBracketsCount = __popcnt(openingBracketsMask);
        uint32_t closingBracketsCount = __popcnt(closingBracketsMask);

        objectDepth += openingBracketsCount;
        if(closingBracketsCount > objectDepth)
        {
            unsigned long closingBracketIndex = 0;
            while(closingBracketsCount != objectDepth)
            {
                _BitScanForward(&closingBracketIndex, closingBracketsMask);
                closingBracketsMask &= ~(1 << closingBracketIndex);
                --closingBracketsCount;
            }

            if(!skipGltfForCharAmount(pParseContext, closingBracketIndex + 1))
            {
                return false;
            }
            break;
        }
        else if(closingBracketsCount == objectDepth)
        {
            unsigned long closingBracketIndex = 0;
            _BitScanReverse(&closingBracketIndex, closingBracketsMask);
            
            if(!skipGltfForCharAmount(pParseContext, closingBracketIndex + 1))
            {
                return false;
            }
            break;
        }

        objectDepth -= closingBracketsCount;
        ++pParseContext->pCurrentParsePos;
    }

    return true;
}

const uint32_t readGltfObjectCountInArrayWithoutAdvancingParser(gltf_parser_context_t* pParseContext)
{
    if(pParseContext->pCurrentParsePos->chars[0] != '[')
    {
        return 0u;
    }

    const parse_pos_t* pCurrentParsePos = pParseContext->pCurrentParsePos;
    
    uint32_t objectCount = 0;
    if(!skipGltfUntilAfterCharToken(pParseContext, '{'))
    {
        return 0;
    }

    pParseContext->pCurrentParsePos = (parse_pos_t*)((char*)pParseContext->pCurrentParsePos - 1);
    do
    {
        if(!skipGltfObject(pParseContext))
        {
            return 0;
        }
        ++objectCount;  
    } while(pParseContext->pCurrentParsePos->chars[0] == ',');

    pParseContext->pCurrentParsePos = pCurrentParsePos;
    return objectCount;
}

bool readGltfMeshes(gltf_parser_context_t* pParserContext, memory_allocator_t* pMemoryAllocator)
{
    if(pParserContext->pGltfDescription->meshCount > 0)
    {
        setGltfParserContextError(pParserContext, "Multiple 'meshes' objects in gltf file.");
        return false;
    }

    if(pParserContext->pCurrentParsePos->chars[0] == ' ')
    {
        if(!skipGltfUntilAfterCharToken(pParserContext, ' '))
        {
            return false;
        }
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

        if(strcmp(propertyName, "primitives") != 0)
        {

        }
        else if(strcmp(propertyName, "name") != 0)
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

    if(gltfParserContext.pGltfDescription == nullptr)
    {
        return createGltfLoadResultError("Out of memory trying to allocate %d bytes for gltf descriptor", sizeof(gltf_description_t));
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

    if(gltfParserContext.success == false)
    {
        return createGltfLoadResultError(gltfParserContext.errorMessage);
    }

    //*pOutDescription = gltfParserContext.pGltfDescription;
    return createGltfLoadResultError("Out of memory trying to allocate %d bytes for gltf descriptor", sizeof(gltf_description_t));
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