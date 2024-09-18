#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#define USE_D3D12_DEBUG 1
#define CLEAR_NEW_MEMORY_WITH_ZEROES 1
#define USE_DEBUG_ASSERTS 1
#include <d3d12.h>
#include <d3d12sdklayers.h>
//#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <dxcapi.h>

#include <stdio.h>
#include <stdint.h>

#include <limits>

#include "include/WinPixEventRuntime/pix3.h"

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "x64/dxcompiler.lib")
//#pragma comment(lib, "x64/D3dcompiler_47.lib")
#pragma comment(lib, "x64/WinPixEventRuntime.lib")

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#if USE_D3D12_DEBUG
#define COM_CALL(func) logOnHResultError(func, #func, __FILE__, __LINE__)
#else
#define COM_CALL(func) func
#endif

#define COM_RELEASE(ptr)if(ptr != nullptr) { (ptr)->Release(); ptr = nullptr; }


#define ASSERT_ALWAYS_MSG(x, msg)               \
{                                               \
    if(!(x))                                    \
    {                                           \
        switch(handleAssert(#x, msg))           \
        {                                       \
            case assert_result_debug:           \
                DebugBreak();                   \
                break;                          \
            case assert_result_continue:        \
                break;                          \
            case assert_result_exit:            \
                exit(-1);                       \
            default:                            \
                break;                          \
        }                                       \
    }                                           \
}


#if USE_DEBUG_ASSERTS
    #define ASSERT_DEBUG_MSG(x, msg)       ASSERT_ALWAYS_MSG(x, msg)
    #define ASSERT_DEBUG_EXECUTE_ALWAYS(x) ASSERT_DEBUG_MSG(x, nullptr)
#else
    #define ASSERT_DEBUG_MSG(x, msg)
    #define ASSERT_DEBUG_EXECUTE_ALWAYS(x) x
#endif

#define ASSERT_ALWAYS(x)                ASSERT_ALWAYS_MSG(x, nullptr)
#define ASSERT_DEBUG(x)                 ASSERT_DEBUG_MSG(x, nullptr)
#define ASSERT_DEBUG_UNREACHABLE_CODE() ASSERT_DEBUG(false)

#define UNREACHABLE_CODE()      __assume(0)
#define UNUSED_PARAMETER(var)   (void)(var)

struct memory_allocator_t;
struct render_context_t;

typedef void*(*allocate_from_memory_allocator_fnc)(memory_allocator_t*, uint64_t, uint64_t);
typedef void(*free_from_memory_allocator_fnc)(memory_allocator_t*, void*);

typedef ID3D12Device10  D3D12DeviceType;
typedef ID3D12Debug6    D3D12DebugType;
typedef IDXGIFactory7   DXGIFactoryType;
typedef IDXGISwapChain4 DXGISwapChainType;

enum pipeline_stream_field_flags_t : uint16_t
{
    vs = 0x01,
    ps = 0x02,
    cs = 0x04
};

enum alloc_flags_t : uint8_t
{
    alloc_flag_none         = 0x0,
    alloc_flag_clear_memory = 0x1
};

enum result_status_t : uint32_t
{
    success = 0,
    out_of_memory,
    file_not_found,
    invalid_arguments,
    internal_error,
    compilation_error
};


template<typename T, typename BASE_TYPE>
struct flags_t
{
    flags_t<T, BASE_TYPE>& operator=(const BASE_TYPE flagsValue)
    {
        value = flagsValue;
        return *this;
    }

    BASE_TYPE value;

    static_assert(sizeof(T) == sizeof(BASE_TYPE));
};

template<typename T, typename BASE_TYPE>
flags_t<T, BASE_TYPE>& operator|=(flags_t<T, BASE_TYPE>& a, const T& b)
{
    a.value |= (BASE_TYPE)b;
    return a;
}

template<typename T, typename BASE_TYPE>
flags_t<T, BASE_TYPE>& operator&=(flags_t<T, BASE_TYPE>& a, const T& b)
{
    a.value &= (BASE_TYPE)b;
    return a;
}

template<typename T, typename BASE_TYPE>
BASE_TYPE operator&(const flags_t<T, BASE_TYPE>& flags, T flag)
{
    BASE_TYPE rawFlags = flags.value;
    return rawFlags & flag;
}


template<typename T>
struct flags8_t : flags_t<T, uint8_t>
{
    flags8_t<T>& operator=(const uint8_t flagsValue)
    {
        value = flagsValue;
        return *this;
    }
};

template<typename T>
struct flags16_t : flags_t<T, uint16_t>
{
    flags16_t<T>& operator=(const uint16_t flagsValue)
    {
        value = flagsValue;
        return *this;
    }
};

template<typename T>
struct flags32_t : flags_t<T, uint32_t>
{
    flags32_t<T>& operator=(const uint32_t flagsValue)
    {
        value = flagsValue;
        return *this;
    }
};

struct memory_allocator_t
{
    allocate_from_memory_allocator_fnc  allocateFnc;
    free_from_memory_allocator_fnc      freeFnc;
};

struct d3d12_resource_t
{
    ID3D12Resource* pResource;
    D3D12_RESOURCE_STATES currentState;
};

struct buffer_slice_t
{
    uint64_t startByteIndex;
    uint64_t endByteIndex;
};

struct staging_buffer_slice_t : buffer_slice_t
{

};

struct memory_buffer_t
{
    void* pData;
    uint64_t sizeInBytes;
};

template <typename T>
struct result_t
{
    result_status_t status;
    T               value;

    result_t(result_status_t status)
    {
        this->status = status;
        memset(&value, 0, sizeof(T));
    }

    result_t(T value)
    {
        this->status = result_status_t::success;
        this->value  = value;
    }

    operator T() const
    {
        return value;
    }

    operator result_status_t() const
    {
        return status;
    }
};

const char* getResultString(const result_status_t result)
{
    switch(result)
    {
        case result_status_t::compilation_error:
            return "compilation error";
        case result_status_t::file_not_found:
            return "file not found";
        case result_status_t::internal_error:
            return "internal error";
        case result_status_t::invalid_arguments:
            return "invalid arguments";
        case result_status_t::out_of_memory:
            return "out of memory";
        case result_status_t::success:
            return "success";
        default:
            DebugBreak();
            break;
    }

    UNREACHABLE_CODE();
    return nullptr;
}

struct render_target_t
{
    d3d12_resource_t            resource;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
};

struct shader_compiler_context_t
{
    IDxcCompiler3*      pShaderCompiler;
    IDxcIncludeHandler* pIncludeHandler;
};

struct d3d12_descriptor_heap_t
{
    const uint8_t* pCPUBaseAddress;
    const uint8_t* pGPUBaseAddress;
    const uint8_t* pCPUEndAddress;
    const uint8_t* pGPUEndAddress;
    uint8_t* pCPUCurrent;
    uint8_t* pGPUCurrent;

    ID3D12DescriptorHeap* pDescriptorHeap;
    uint64_t incrementSizeInBytes;
};

struct d3d12_swap_chain_t
{
    d3d12_descriptor_heap_t         backBufferRenderTargetDescriptorHeap;
    memory_allocator_t*             pMemoryAllocator;
    render_target_t*                pBackBuffers;
    DXGISwapChainType*              pSwapChain;
    D3D12_CPU_DESCRIPTOR_HANDLE*    pBackBufferRenderTargetHandles;
    uint32_t                        width;
    uint32_t                        height;
    uint8_t                         backBufferCount;
};

struct render_pass_t
{
    ID3D12CommandAllocator*     pGraphicsCommandAllocator;
    ID3D12GraphicsCommandList*  pGraphicsCommandList;
    const char*                 pName;
    bool                        isOpen;
    render_target_t*            pRenderTarget;
    render_pass_t*              pNext;
};

struct graphics_pipeline_state_t
{
    ID3D12PipelineState* pPipelineState;
    ID3D12RootSignature* pRootSignature;
};

enum upload_buffer_flags_t : uint8_t
{
    upload_buffer_flag_none                = 0x0
};

struct upload_buffer_t
{
    uint32_t            sizeInBytes;
    void*               pData;
    d3d12_resource_t    bufferResource;
    upload_buffer_t*    pNext;
};

enum vertex_attribute_t : uint8_t
{
    position,
    color
};

enum vertex_attribute_type_t : uint8_t
{
    float32
};

struct vertex_attribute_entry_t
{
    vertex_attribute_t      attribute;
    vertex_attribute_type_t type;
    uint32_t                count;
};

struct vertex_format_t
{
    vertex_attribute_entry_t    pVertexAttributes[12];
    uint32_t                    vertexAttributeCount;
};

struct vertex_buffer_t
{
    d3d12_resource_t bufferResource;
    uint32_t         sizeInBytes;
};

struct shader_binary_t
{
    const uint8_t* pShaderBlob;
    uint32_t shaderBlobSizeInBytes;
    shader_binary_t* pNext;
};

struct render_pass_parameters_t
{
    render_target_t* pRenderTarget; // nullptr = backbuffer
    shader_binary_t* pVertexShader;
    shader_binary_t* pPixelShader;
};

struct graphics_pipeline_state_parameters_t
{
    const char*         pName;
    shader_binary_t*    pVertexShader;
    shader_binary_t*    pPixelShader;
    vertex_format_t*    pVertexFormat;
};

struct base_dynamic_array_t
{
    memory_allocator_t* pMemoryAllocator;
    void*               pData;
    uint32_t            count;
    uint32_t            capacity;
    uint32_t            elementSizeInBytes;
};

template<typename T>
struct auto_resource_t
{
    T operator()
    {
        return resource;
    }

    T resource;
};

template<typename T>
struct dynamic_array_t : base_dynamic_array_t
{
};

enum render_resource_flags_t : uint8_t
{
    none                    = 0,
    notify_on_array_grow    = 0x01
};

struct render_resource_cache_t
{
    memory_allocator_t*                 pMemoryAllocator;
    dynamic_array_t<vertex_buffer_t>    vertexBuffers;
    dynamic_array_t<render_pass_t>      renderPasses;
    dynamic_array_t<render_target_t>    renderTargets;
    dynamic_array_t<vertex_format_t>    vertexFormats;
    dynamic_array_t<graphics_pipeline_state_t>   pipelineStates;
    dynamic_array_t<upload_buffer_t>    uploadBuffers;
    dynamic_array_t<shader_binary_t>    shaderBinaries;

    render_pass_t*                      pFirstFreeRenderPass;

    flags8_t<render_resource_flags_t>   flags;
};

struct graphics_frame_t
{
    memory_allocator_t                      tempMemoryAllocator;
    render_resource_cache_t*                pRenderResourceCache;
    shader_compiler_context_t*              pShaderCompilerContext;
    render_target_t*                        pBackBuffer;
    memory_allocator_t*                     pMemoryAllocator;
    render_pass_t*                          pFirstRenderPassToExecute;
    render_pass_t*                          pLastRenderPassToExecute;
    upload_buffer_t*                        pFirstUploadBuffer;
    uint64_t                                frameIndex;
    uint32_t                                openRenderPassCount;
    D3D12DeviceType*                        pDevice;
    ID3D12Fence*                            pFrameFence;
    ID3D12GraphicsCommandList*              pFrameGeneralGraphicsQueue;
    ID3D12CommandAllocator*                 pFrameGeneralGraphicsCommandAllocator;
    ID3D12CommandQueue*                     pFrameCommandQueue;
    HANDLE                                  pFrameFinishedEvent;
};

struct graphics_frame_collection_t
{
    memory_allocator_t* pMemoryAllocator;
    graphics_frame_t*   pGraphicsFrames;
    uint8_t             frameCount;
};

struct graphics_frame_parameters_t
{
    uint32_t maxRenderPassCount;
    uint32_t maxVertexBufferCount;
    uint32_t maxUploadBufferCount;
    uint32_t defaultStagingBufferSizeInBytes;
};

struct render_context_t
{
    D3D12DeviceType*            pDevice;
    D3D12DebugType*             pDebugLayer;
    DXGIFactoryType*            pFactory;

    render_resource_cache_t     renderResourceCache;
    shader_compiler_context_t   shaderCompilerContext;
    memory_allocator_t          defaultAllocator;
    graphics_frame_collection_t graphicsFramesCollection;
    const graphics_frame_t*     pCurrentGraphicsFrame;

    d3d12_swap_chain_t          swapChain;
    ID3D12CommandQueue*         pDefaultDirectCommandQueue;
    ID3D12CommandQueue*         pDefaultCopyCommandQueue;

    uint64_t                    frameIndex;
};

template<typename T>
struct com_auto_release_t
{
    com_auto_release_t(T* pComPointer)
    {
        pPointer = pComPointer;
    }

    ~com_auto_release_t()
    {
        COM_RELEASE(pPointer);
    }

    T* operator->()
    {
        return pPointer;
    }

    T& operator*()
    {
        return *pPointer;
    }

    T** operator&()
    {
        return &pPointer;
    }

    T* pPointer;
};

enum assert_result_t
{
    assert_result_debug,
    assert_result_continue,
    assert_result_exit
};

constexpr uint64_t          defaultAllocationAlignment      = 16u;
constexpr uint32_t          invalidResourceHandleValue      = ~0u;
constexpr memory_buffer_t   emptyMemoryBuffer               = {nullptr, 0u};

template<typename T>
T createInvalidResourceHandle()
{
    T handle = {invalidResourceHandleValue};
    return handle;
}

template<typename T>
T createResourceHandle(uint32_t resourceIndex)
{
    T handle = {resourceIndex};
    return handle;
}

template<typename T>
bool isInvalidResourceHandle(const T& resourceHandle)
{
    return resourceHandle.index == invalidResourceHandleValue;
}

void copyMemoryNonOverlapping(void* pDst, const void* pSrc, const uint64_t sizeInBytes)
{
    memcpy(pDst, pSrc, sizeInBytes);
}

void logError(const char* pErrorFormat, ...)
{
    printf("Error: ");

    va_list vaList;
    va_start(vaList, pErrorFormat);
    vprintf(pErrorFormat, vaList);
    fflush(stdout);
    va_end(vaList);

    printf("\n");
}

void logWarning(const char* pErrorFormat, ...)
{
    printf("Warning: ");

    va_list vaList;
    va_start(vaList, pErrorFormat);
    vprintf(pErrorFormat, vaList);
    fflush(stdout);
    va_end(vaList);

    printf("\n");
}

assert_result_t handleAssert(const char* pExpression, const char* pUserMessage)
{
    char messageBuffer[1024] = {0};
    if(pUserMessage == nullptr)
    {
        sprintf_s(messageBuffer, sizeof(messageBuffer), "Error in Expression '%s'\n\nPress Try Again to debug\nPress Continue to ignore this assert\nPress Cancel to exit application.", pExpression);
    }
    else
    {
        sprintf_s(messageBuffer, sizeof(messageBuffer), "Error in Expression '%s'\n===============\n%s\n===============\n\nPress Try Again to continue\nPress Continue to ignore this assert\nPress Cancel to exit application.", pExpression, pUserMessage);
    }
    const int messageBoxResult = MessageBoxA(nullptr, messageBuffer, "DEBUG ASSERT", MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_DEFBUTTON1);
    if(messageBoxResult == IDCANCEL)
    {
        return assert_result_exit;
    }
    else if(messageBoxResult == IDCONTINUE)
    {
        return assert_result_continue;
    }

    return assert_result_debug;
}

const char* getHResultString(HRESULT result)
{
    switch(result)
    {
        case S_OK:
            return "Ok";
        case S_FALSE:
            return "False";
        case E_FAIL:
            return "Fail - Attempted to create a device with the debug layer enabled and the layer is not installed.";
        case E_INVALIDARG:
            return "An invalid parameter was passed to the returning function.";
        case E_OUTOFMEMORY:
            return "Direct3D could not allocate sufficient memory to complete the call.";
        case E_NOTIMPL:
            return "The method call isn't implemented with the passed parameter combination.";
        case D3D12_ERROR_ADAPTER_NOT_FOUND:
            return "D3D12 Adapter not found - The specified cached PSO was created on a different adapter and cannot be reused on the current adapter.";
        case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
            return "D3D12 Driver version mismatch - The specified cached PSO was created on a different driver version and cannot be reused on the current adapter.";
        case DXGI_ERROR_INVALID_CALL:
            return "D3D12 invalid call - The method call is invalid. For example, a method's parameter may not be a valid pointer.";
        case DXGI_ERROR_WAS_STILL_DRAWING:
            return "D3D12 was still drawing - The previous blit operation that is transferring information to or from this surface is incomplete.";

        default:
            break;        
    }

    return "unknown";
}

void addBeginMarker(ID3D12GraphicsCommandList* pCommandList, const char* pName)
{
    PIXBeginEvent(pCommandList, PIX_COLOR(100, 100, 100), pName);
}

void addEndMarker(ID3D12GraphicsCommandList* pCommandList)
{
    PIXEndEvent(pCommandList);
}

template <typename T>
result_t<T> createResult(T value, const result_status_t resultError)
{
    result_t<T> result = {};
    result.error = resultError;
    result.value;

    return result;
}

bool isResultSuccessful(const result_status_t resultError)
{
    return resultError == result_status_t::success;
}

void setD3D12ObjectDebugName(ID3D12Object* pObject, const char* pName)
{
    wchar_t wideNameBuffer[256] = {};
    mbstowcs(wideNameBuffer, pName, sizeof(wideNameBuffer) / sizeof(wchar_t));
    pObject->SetName(wideNameBuffer);
}

template<typename T>
void clearMemoryWithZeroes(T* pMemory)
{
    ASSERT_DEBUG(pMemory);
    memset(pMemory, 0, sizeof(T));
}

HRESULT logOnHResultError(const HRESULT originalResult, const char* pFunctionCall, const char* pFile, const uint32_t lineNumber)
{
    if(originalResult != S_OK)
    {
        logError("Error during call '%s' in %s:%u\nError:%s\n", pFunctionCall, pFile, lineNumber, getHResultString(originalResult));
    }

    return originalResult;
}

void resetAllocator(memory_allocator_t* pAllocator)
{
    
}

void* allocateFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, alloc_flags_t flags = alloc_flag_none)
{
    void* pMemory = pAllocator->allocateFnc(pAllocator, sizeInBytes, defaultAllocationAlignment);
    
#if FORCE_CLEAR_ALLOCATIONS
    const bool clearMemory = true;
#else
    const bool clearMemory = (flags & alloc_flag_clear_memory);
#endif

    if(pMemory && clearMemory)
    {
        memset(pMemory, 0, sizeInBytes);
    }

    return pMemory;
}

void* allocateAlignedFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, uint64_t alignmentInBytes, alloc_flags_t flags = alloc_flag_none)
{
    void* pMemory = pAllocator->allocateFnc(pAllocator, sizeInBytes, alignmentInBytes);
    
#if FORCE_CLEAR_ALLOCATIONS
    const bool clearMemory = true;
#else
    const bool clearMemory = (flags & alloc_flag_clear_memory);
#endif

    if(pMemory && clearMemory)
    {
        memset(pMemory, 0, sizeInBytes);
    }
    
    return pMemory;
}

void freeFromAllocator(memory_allocator_t* pAllocator, void* pMemory)
{
    pAllocator->freeFnc(pAllocator, pMemory);
}

void* allocateFromDefaultAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, uint64_t alignmentInBytes)
{
    return _aligned_malloc(sizeInBytes, alignmentInBytes);
}

void freeFromDefaultAllocator(memory_allocator_t* pAllocator, void* pMemory)
{
    return _aligned_free(pMemory);
}

void createDefaultMemoryAllocator(memory_allocator_t* pAllocator)
{
    pAllocator->allocateFnc = allocateFromDefaultAllocator;
    pAllocator->freeFnc     = freeFromDefaultAllocator;
}

shader_binary_t* allocateShaderBinary(graphics_frame_t* pGraphicsFrame)
{
    return (shader_binary_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, sizeof(shader_binary_t));
}

void freeShaderBinary(graphics_frame_t* pGraphicsFrame, shader_binary_t* pShaderBinary)
{
    freeFromAllocator(pGraphicsFrame->pMemoryAllocator, pShaderBinary);
}

void freeUploadBuffer(render_resource_cache_t* pRenderResourceCache, upload_buffer_t* pUploadBuffer)
{
    //TODO
}

void initializeRenderTarget(render_target_t* pRenderTarget, ID3D12Resource* pRenderTargetResource, D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorHandle, D3D12_RESOURCE_STATES state)
{
    pRenderTarget->resource.pResource       = pRenderTargetResource;
    pRenderTarget->resource.currentState    = state;
    pRenderTarget->cpuDescriptorHandle      = *pDescriptorHandle;
}

void destroyRenderTarget(render_target_t* pRenderTarget)
{
    COM_RELEASE(pRenderTarget->resource.pResource);
}

void resetDescriptorHeap(d3d12_descriptor_heap_t* pDescriptorHeap)
{
    pDescriptorHeap->pCPUCurrent = (uint8_t*)pDescriptorHeap->pCPUBaseAddress;
    pDescriptorHeap->pGPUCurrent = (uint8_t*)pDescriptorHeap->pGPUBaseAddress;
}

D3D12_CPU_DESCRIPTOR_HANDLE getNextCPUDescriptorHandle(d3d12_descriptor_heap_t* pDescriptorHeap)
{
    ASSERT_DEBUG(pDescriptorHeap->pCPUCurrent != pDescriptorHeap->pCPUEndAddress);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    cpuHandle.ptr = (size_t)pDescriptorHeap->pCPUCurrent;
    pDescriptorHeap->pCPUCurrent += pDescriptorHeap->incrementSizeInBytes;

    return cpuHandle;
}

void markRenderPassChainAsFree(render_resource_cache_t* pRenderResourceCache, render_pass_t* pFirstRenderPassInChain)
{
    render_pass_t* pCurrentRenderPass = pFirstRenderPassInChain;
    while(pCurrentRenderPass != nullptr)
    {
        render_pass_t* pNextRenderPass = pCurrentRenderPass->pNext;
        pCurrentRenderPass->pNext = pRenderResourceCache->pFirstFreeRenderPass;
        pRenderResourceCache->pFirstFreeRenderPass = pCurrentRenderPass;

        pCurrentRenderPass = pNextRenderPass;
    }
}

void flushFrame(graphics_frame_t* pGraphicsFrame)
{
    const DWORD waitResult = WaitForSingleObject(pGraphicsFrame->pFrameFinishedEvent, INFINITE);
    ASSERT_DEBUG(waitResult == WAIT_OBJECT_0);
}

void resetFrame(graphics_frame_t* pGraphicsFrame)
{
    COM_CALL(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator->Reset());
    COM_CALL(pGraphicsFrame->pFrameGeneralGraphicsQueue->Reset(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator, nullptr));

    markRenderPassChainAsFree(pGraphicsFrame->pRenderResourceCache, pGraphicsFrame->pFirstRenderPassToExecute);
    pGraphicsFrame->pFirstRenderPassToExecute = nullptr;
    pGraphicsFrame->pLastRenderPassToExecute = nullptr;
}

graphics_frame_t* getGraphicsFrameFromGraphicsFrameCollection(graphics_frame_collection_t* pGraphicsFrameCollection, const uint64_t frameIndex)
{
    ASSERT_DEBUG(pGraphicsFrameCollection != nullptr);
    ASSERT_DEBUG(pGraphicsFrameCollection->frameCount > frameIndex);

    return pGraphicsFrameCollection->pGraphicsFrames + frameIndex;
}

void flushAllFrames(render_context_t* pRenderContext)
{
    for(uint32_t frameIndex = 0u; frameIndex < pRenderContext->graphicsFramesCollection.frameCount; ++frameIndex)
    {
        graphics_frame_t* pGraphicsFrame = getGraphicsFrameFromGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, frameIndex);
        flushFrame(pGraphicsFrame);
    }
}

void transitionResource(ID3D12GraphicsCommandList* pGraphicsCommandList, d3d12_resource_t* pResource, D3D12_RESOURCE_STATES newState)
{
    if(pResource->currentState == newState)
    {
        return;
    }
    
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = pResource->pResource;
    barrier.Transition.StateBefore = pResource->currentState;
    barrier.Transition.StateAfter = newState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    pGraphicsCommandList->ResourceBarrier(1u, &barrier);
    pResource->currentState = newState;
}

void transitionResource(ID3D12GraphicsCommandList* pGraphicsCommandList, d3d12_resource_t* pResource, D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES newState)
{
    ASSERT_DEBUG(pResource->currentState != currentState);
    transitionResource(pGraphicsCommandList, pResource, newState);
}

bool createD3D12Device(D3D12DeviceType** pOutDevice)
{
    if(COM_CALL(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(pOutDevice))) != S_OK)
    {
        return false;
    }

    (*pOutDevice)->SetName(L"D3D12Device");
    return true;
}

bool enableD3D12DebugLayer(ID3D12Debug6** pOutDebugLayer)
{
    if(COM_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(pOutDebugLayer))) != S_OK)
    {
        return false;
    }

    (*pOutDebugLayer)->EnableDebugLayer();
    (*pOutDebugLayer)->SetEnableAutoName(TRUE);

    return true;
}

bool setupD3D12DebugLayer(D3D12DeviceType* pDevice)
{
    com_auto_release_t<ID3D12InfoQueue> pInfoQueue = nullptr;
    if(COM_CALL(pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue))) != S_OK)
    {
        return false;
    }

    if(COM_CALL(pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE)) != S_OK)
    {
        return false;
    }
    
    if(COM_CALL(pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE)) != S_OK)
    {
        return false;
    }

    return true;
}

bool createD3D12Factory(DXGIFactoryType** pOutFactory)
{
    uint32_t factoryFlags = 0u;

#if USE_D3D12_DEBUG
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    if(COM_CALL(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(pOutFactory))) != S_OK)
    {
        return false;
    }

    return true;
}

bool createCommandQueue(D3D12DeviceType* pDevice, ID3D12CommandQueue** pOutCommandQueue, D3D12_COMMAND_LIST_TYPE commandListType)
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type        = commandListType;
    commandQueueDesc.Priority    = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags       = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if(COM_CALL(pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(pOutCommandQueue))) != S_OK)
    {
        return false;
    }

    return true;
}

void destroyDescriptorHeap(d3d12_descriptor_heap_t* pDescriptorHeap)
{
    COM_RELEASE(pDescriptorHeap->pDescriptorHeap);
    clearMemoryWithZeroes(pDescriptorHeap);
}

bool createDescriptorHeap(d3d12_descriptor_heap_t* pOutDescriptorHeap, D3D12DeviceType* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t descriptorCount )
{
    ID3D12DescriptorHeap* pDescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.Type             = type;
    descriptorHeapDesc.NumDescriptors   = descriptorCount;
    descriptorHeapDesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(COM_CALL(pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&pDescriptorHeap))) != S_OK)
    {
        return false;
    }

    const uint64_t cpuDescriptorHeapStartAddress = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    const uint64_t gpuDescriptorHeapStartAddress = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    const uint64_t incrementSizeInBytes = pDevice->GetDescriptorHandleIncrementSize(type);

    pOutDescriptorHeap->incrementSizeInBytes = incrementSizeInBytes;
    pOutDescriptorHeap->pCPUBaseAddress = (const uint8_t*)cpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pGPUBaseAddress = (const uint8_t*)gpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pCPUCurrent = (uint8_t*)cpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pGPUCurrent = (uint8_t*)gpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pCPUEndAddress = (const uint8_t*)cpuDescriptorHeapStartAddress + incrementSizeInBytes * descriptorCount;
    pOutDescriptorHeap->pGPUEndAddress = (const uint8_t*)gpuDescriptorHeapStartAddress + incrementSizeInBytes * descriptorCount;
    pOutDescriptorHeap->pDescriptorHeap = pDescriptorHeap;
    return true;
}

void destroySwapChain(d3d12_swap_chain_t* pSwapChain)
{
    COM_RELEASE(pSwapChain->pSwapChain);
    for(uint32_t bufferIndex = 0u; bufferIndex < pSwapChain->backBufferCount; ++bufferIndex)
    {
        destroyRenderTarget(&pSwapChain->pBackBuffers[bufferIndex]);
    }

    freeFromAllocator(pSwapChain->pMemoryAllocator, pSwapChain->pBackBuffers);
    destroyDescriptorHeap(&pSwapChain->backBufferRenderTargetDescriptorHeap);
}

bool createSwapChain(d3d12_swap_chain_t* pOutSwapChain, memory_allocator_t* pMemoryAllocator, IDXGIFactory6* pFactory, D3D12DeviceType* pDevice, ID3D12CommandQueue* pCommandQueue, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, const uint32_t frameBufferCount)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount   = frameBufferCount;
    swapChainDesc.SampleDesc    = {1, 0};
    swapChainDesc.Stereo        = FALSE;
    swapChainDesc.BufferUsage   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect    = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Height        = windowHeight;
    swapChainDesc.Width         = windowWidth;
    swapChainDesc.Scaling       = DXGI_SCALING_NONE;
    swapChainDesc.AlphaMode     = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags         = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    
    d3d12_swap_chain_t swapChain = {};
    swapChain.backBufferCount = frameBufferCount;
    swapChain.pMemoryAllocator = pMemoryAllocator;

    com_auto_release_t<IDXGISwapChain1> pTempSwapChain = nullptr;
    
    if(!createDescriptorHeap(&swapChain.backBufferRenderTargetDescriptorHeap, pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, frameBufferCount))
    {
        goto cleanup_and_exit_failure;
    }

    if(COM_CALL(pFactory->CreateSwapChainForHwnd(pCommandQueue, pWindowHandle, &swapChainDesc, nullptr, nullptr, &pTempSwapChain)) != S_OK)
    {
        goto cleanup_and_exit_failure;
    }

    if(COM_CALL(pTempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain.pSwapChain))) != S_OK)
    {
        goto cleanup_and_exit_failure;
    }

    swapChain.pBackBuffers = (render_target_t*)allocateFromAllocator(pMemoryAllocator, sizeof(render_target_t) * frameBufferCount);
    if(swapChain.pBackBuffers == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    for(uint32_t bufferIndex = 0u; bufferIndex < frameBufferCount; ++bufferIndex)
    {
        ID3D12Resource* pFrameBuffer = nullptr;
        if(COM_CALL(swapChain.pSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pFrameBuffer))) != S_OK)
        {
            goto cleanup_and_exit_failure;
        }

        setD3D12ObjectDebugName(pFrameBuffer, "Frame Buffer");
        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = getNextCPUDescriptorHandle(&swapChain.backBufferRenderTargetDescriptorHeap);
        pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, descriptorHandle);

        initializeRenderTarget(&swapChain.pBackBuffers[bufferIndex], pFrameBuffer, &descriptorHandle, D3D12_RESOURCE_STATE_PRESENT);
    }
    
    swapChain.width = windowWidth;
    swapChain.height = windowHeight;

    *pOutSwapChain = swapChain;
    return true;

    cleanup_and_exit_failure:
        destroySwapChain(&swapChain);
        return false;
}

bool createCommandAllocator(D3D12DeviceType* pDevice, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator** pOutCommandAllocator)
{
    if(COM_CALL(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(pOutCommandAllocator))) != S_OK)
    {
        return false;
    }

    return true;
}

bool createCommandList(D3D12DeviceType* pDevice, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pCommandAllocator, ID3D12GraphicsCommandList** pOutCommandList)
{
    if(COM_CALL(pDevice->CreateCommandList(0, type, pCommandAllocator, nullptr, IID_PPV_ARGS(pOutCommandList))) != S_OK)
    {
        return false;
    }

    (*pOutCommandList)->Close();

    return true;
}

bool createFence(D3D12DeviceType* pDevice, ID3D12Fence** pOutFence, const uint32_t initialValue)
{
    if(COM_CALL(pDevice->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(pOutFence))) != S_OK)
    {
        return false;
    }

    return true;
}

void destroyGraphicsFrame(graphics_frame_t* pGraphicsFrame)
{
    flushFrame(pGraphicsFrame);
    if(pGraphicsFrame->pFrameFinishedEvent != nullptr)
    {
        CloseHandle(pGraphicsFrame->pFrameFinishedEvent);
    }

#if 0
    for(uint32_t renderPassIndex = 0u; renderPassIndex < pGraphicsFrame->renderPassCount; ++renderPassIndex)
    {
        COM_RELEASE(pGraphicsFrame->pRenderPassBuffer[renderPassIndex].pGraphicsCommandList);
        COM_RELEASE(pGraphicsFrame->pRenderPassBuffer[renderPassIndex].pGraphicsCommandAllocator);
    }

    freeFromAllocator(pGraphicsFrame->pMemoryAllocator, pGraphicsFrame->pRenderPassBuffer);
#endif

    COM_RELEASE(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator);
    COM_RELEASE(pGraphicsFrame->pFrameGeneralGraphicsQueue);
    COM_RELEASE(pGraphicsFrame->pFrameCommandQueue);
    COM_RELEASE(pGraphicsFrame->pFrameFence);   
}

void destroyGraphicsFrameCollection(graphics_frame_collection_t* pGraphicsFrameCollection)
{
    for(uint32_t frameIndex = 0u; frameIndex < pGraphicsFrameCollection->frameCount; ++frameIndex)
    {
        graphics_frame_t* pGraphicsFrame = &pGraphicsFrameCollection->pGraphicsFrames[frameIndex];
        destroyGraphicsFrame(pGraphicsFrame);
    }

    freeFromAllocator(pGraphicsFrameCollection->pMemoryAllocator, pGraphicsFrameCollection->pGraphicsFrames);
}

bool isValidGraphicsFrameParameters(const graphics_frame_parameters_t* pGraphicsFrameParameters)
{
    if(pGraphicsFrameParameters->maxRenderPassCount == 0u)
    {
        return false;
    }

    if(pGraphicsFrameParameters->maxUploadBufferCount == 0u)
    {
        return false;
    }

    if(pGraphicsFrameParameters->maxVertexBufferCount == 0u)
    {
        return false;
    }

    return true;
}

bool createGraphicsFrame(graphics_frame_t* pOutGraphicFrame, const graphics_frame_parameters_t* pGraphicsFrameParameters, memory_allocator_t* pMemoryAllocator, render_resource_cache_t* pRenderResourceCache, shader_compiler_context_t* pShaderCompilerContext, D3D12DeviceType* pDevice)
{
    ASSERT_DEBUG(pGraphicsFrameParameters != nullptr);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    ASSERT_DEBUG(pDevice != nullptr);
    ASSERT_DEBUG(isValidGraphicsFrameParameters(pGraphicsFrameParameters));

    graphics_frame_t graphicsFrame = {0};
    graphicsFrame.pMemoryAllocator = pMemoryAllocator;
    graphicsFrame.pFrameFinishedEvent = CreateEvent(nullptr, FALSE, TRUE, "");
    graphicsFrame.pShaderCompilerContext = pShaderCompilerContext;
    graphicsFrame.pRenderResourceCache = pRenderResourceCache;
    if(graphicsFrame.pFrameFinishedEvent == nullptr)
    {
        return false;
    }

    createDefaultMemoryAllocator(&graphicsFrame.tempMemoryAllocator);

    render_pass_t* pRenderPasses = (render_pass_t*)allocateFromAllocator(pMemoryAllocator, (sizeof(render_pass_t) * pGraphicsFrameParameters->maxRenderPassCount), alloc_flag_clear_memory);
    if(pRenderPasses == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    vertex_buffer_t* pVertexBuffers = (vertex_buffer_t*)allocateFromAllocator(pMemoryAllocator, sizeof(vertex_buffer_t) * pGraphicsFrameParameters->maxVertexBufferCount, alloc_flag_clear_memory);
    if(pVertexBuffers == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    upload_buffer_t* pUploadBuffers = (upload_buffer_t*)allocateFromAllocator(pMemoryAllocator, sizeof(upload_buffer_t) * pGraphicsFrameParameters->maxUploadBufferCount, alloc_flag_clear_memory);
    if(pUploadBuffers == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &graphicsFrame.pFrameGeneralGraphicsCommandAllocator))
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, graphicsFrame.pFrameGeneralGraphicsCommandAllocator, &graphicsFrame.pFrameGeneralGraphicsQueue))
    {
        goto cleanup_and_exit_failure;
    }

    for(uint32_t renderPassIndex = 0u; renderPassIndex < pGraphicsFrameParameters->maxRenderPassCount; ++renderPassIndex)
    {
        if(!createCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &pRenderPasses[renderPassIndex].pGraphicsCommandAllocator))
        {
            goto cleanup_and_exit_failure;
        }

        if(!createCommandList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, pRenderPasses[renderPassIndex].pGraphicsCommandAllocator, &pRenderPasses[renderPassIndex].pGraphicsCommandList))
        {
            goto cleanup_and_exit_failure;
        }
    }

    graphicsFrame.pDevice = pDevice;

    if(!createFence(pDevice, &graphicsFrame.pFrameFence, 0))
    {
        goto cleanup_and_exit_failure;
    }

    *pOutGraphicFrame = graphicsFrame;
    return true;

    cleanup_and_exit_failure:
        destroyGraphicsFrame(&graphicsFrame);
        return false;
}

bool createGraphicsFrameCollection(graphics_frame_collection_t* pOutGraphicFrameCollection, const graphics_frame_parameters_t* pGraphicsFrameParameters, memory_allocator_t* pMemoryAllocator, render_resource_cache_t* pRenderResourceCache, shader_compiler_context_t* pShaderCompilerContext, D3D12DeviceType* pDevice, const uint8_t frameCount)
{
    graphics_frame_collection_t graphicFrameCollection = {};
    graphicFrameCollection.pMemoryAllocator = pMemoryAllocator;
    graphicFrameCollection.frameCount       = frameCount;
    graphicFrameCollection.pGraphicsFrames  = (graphics_frame_t*)allocateFromAllocator(pMemoryAllocator, sizeof(graphics_frame_t) * frameCount);
    if(graphicFrameCollection.pGraphicsFrames == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    for(uint32_t frameIndex = 0u; frameIndex < frameCount; ++frameIndex)
    {
        if(!createGraphicsFrame(&graphicFrameCollection.pGraphicsFrames[frameIndex], pGraphicsFrameParameters, pMemoryAllocator, pRenderResourceCache, pShaderCompilerContext, pDevice))
        {
            goto cleanup_and_exit_failure;
        }
    }

    *pOutGraphicFrameCollection = graphicFrameCollection;
    return true;

    cleanup_and_exit_failure:
        destroyGraphicsFrameCollection(pOutGraphicFrameCollection);
        freeFromAllocator(pMemoryAllocator, graphicFrameCollection.pGraphicsFrames);

        return false;
}

graphics_frame_t* getCurrentFrame(render_context_t* pRenderContext)
{
    const uint64_t frameIndex = pRenderContext->frameIndex % pRenderContext->graphicsFramesCollection.frameCount;
    return getGraphicsFrameFromGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, frameIndex);
}

enum render_context_flags_t : uint8_t
{
    use_debug_layer = 0x01,
    notify_on_limit_reach = 0x02
};

struct render_context_parameters_t
{
    memory_allocator_t*                 pAllocator;
    HWND                                pWindowHandle;
    uint32_t                            windowWidth;
    uint32_t                            windowHeight;
    uint32_t                            frameBufferCount;
    
    flags8_t<render_context_flags_t>    flags;

    struct limits_t
    {
        uint32_t                        maxVertexBufferCount;
        uint32_t                        maxUploadBufferCount;
        uint32_t                        maxVertexFormatCount;
        uint32_t                        maxShaderBinaryCount;
        uint32_t                        maxRenderPassCount;
        uint32_t                        maxRenderTargetCount;
        uint32_t                        maxPipelineStateCount;
        uint32_t                        defaultStagingBufferSizeInBytes;
    } limits;
};

struct ComCustomMalloc : IMalloc
{
    ComCustomMalloc(memory_allocator_t* pAllocator) : m_pAllocator(pAllocator) {};
    virtual void* Alloc(SIZE_T sizeInBytes)
    {
        return allocateFromAllocator(m_pAllocator, sizeInBytes);
    }

    virtual void Free(void* pMemory)
    {
        freeFromAllocator(m_pAllocator, pMemory);
    }

    virtual int DidAlloc(void* pMemory)
    {
        return pMemory != nullptr;
    }

    virtual void* Realloc(void* pMemory, SIZE_T newSizeInBytes)
    {
        ASSERT_DEBUG_UNREACHABLE_CODE();
        return nullptr;
    }

    virtual SIZE_T GetSize(void* pMemory)
    {
        ASSERT_DEBUG_UNREACHABLE_CODE();
        return 0u;
    }

    virtual void HeapMinimize()
    {

    }

    memory_allocator_t* m_pAllocator;
};

bool isValidRenderContextParameters(const render_context_parameters_t* pParameters)
{
    if(pParameters->frameBufferCount < 2u)
    {
        return false;
    }

    if(pParameters->pWindowHandle == nullptr || pParameters->pWindowHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if(pParameters->windowHeight <= 0)
    {
        return false;
    }

    if(pParameters->windowWidth <= 0)
    {
        return false;
    }

    if(pParameters->limits.maxUploadBufferCount == 0)
    {
        return false;
    }

    if(pParameters->limits.maxVertexBufferCount == 0)
    {
        return false;
    }

    return true;
}

bool createShaderCompilerContext(memory_allocator_t* pAllocator, shader_compiler_context_t* pShaderCompilerContext)
{
    UNUSED_PARAMETER(pAllocator);
    if(COM_CALL(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pShaderCompilerContext->pShaderCompiler))) != S_OK)
    {
        return false;
    }

    IDxcLibrary* pShaderLibrary = nullptr;
    if(COM_CALL(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pShaderLibrary))) != S_OK)
    {
        return false;
    }

    if(COM_CALL(pShaderLibrary->CreateIncludeHandler(&pShaderCompilerContext->pIncludeHandler) != S_OK))
    {
        pShaderLibrary->Release();
        return false;
    }

    pShaderLibrary->Release();
    return true;
}

template<typename T>
void createDynamicArrayWithPreallocatedMemory(base_dynamic_array_t* pOutArray, memory_allocator_t* pMemoryAllocator, void* pPreallocatedMemory, const uint32_t elementCapacity)
{
    pOutArray->pMemoryAllocator     = pMemoryAllocator;
    pOutArray->pData                = (T*)pPreallocatedMemory;
    pOutArray->count                = 0u;
    pOutArray->capacity             = elementCapacity;
    pOutArray->elementSizeInBytes   = sizeof(T);
}

void* pushBackFromDynamicArrayDontGrow(base_dynamic_array_t* pArray, const uint32_t count)
{
    const uint32_t newArrayCount = pArray->count + count;
    if(newArrayCount >= pArray->capacity)
    {
        return nullptr;
    }

    void* pData = (uint8_t*)pArray->pData + pArray->count * pArray->elementSizeInBytes;
    pArray->count += count;

    return pData;
}

uint32_t calculateNewDynamicArrayCapacity(const base_dynamic_array_t* pArray)
{
    return pArray->capacity * 2u;
}

bool tryToGrowDynamicArray(base_dynamic_array_t* pArray)
{
    const uint32_t newCapacity = calculateNewDynamicArrayCapacity(pArray);
    void* pNewData = allocateFromAllocator(pArray->pMemoryAllocator, newCapacity * pArray->elementSizeInBytes);
    if(pNewData == nullptr)
    {
        return false;
    }

    copyMemoryNonOverlapping(pNewData, pArray->pData, pArray->capacity * pArray->elementSizeInBytes);
    freeFromAllocator(pArray->pMemoryAllocator, pArray->pData);
    pArray->pData = pNewData;
    pArray->capacity = newCapacity;

    return true;
}

render_pass_t* createRenderPassChain(render_pass_t* pRenderPasses, const uint32_t renderPassCount)
{
    ASSERT_DEBUG(pRenderPasses != nullptr);
    ASSERT_DEBUG(renderPassCount > 0u);

    render_pass_t* pCurrentRenderPass = pRenderPasses;
    render_pass_t* pNextRenderPass = pRenderPasses + 1;
    uint32_t renderPassIndex = 0u;
    while(true)
    {
        if((renderPassIndex + 1) == renderPassCount)
        {
            pCurrentRenderPass->pNext = nullptr;
            break;
        }

        pCurrentRenderPass->pNext = pNextRenderPass;
        pCurrentRenderPass = pNextRenderPass;
        pNextRenderPass = pNextRenderPass + 1;
        ++renderPassIndex;
    }

    return pRenderPasses;
}

bool initRenderPassChain(D3D12DeviceType* pDevice, render_pass_t* pFirstRenderPassInChain)
{
    render_pass_t* pCurrentRenderPass = pFirstRenderPassInChain;
    while(pCurrentRenderPass != nullptr)
    {
        
        if(!createCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &pCurrentRenderPass->pGraphicsCommandAllocator))
        {
            return false;
        }

        if(!createCommandList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, pCurrentRenderPass->pGraphicsCommandAllocator, &pCurrentRenderPass->pGraphicsCommandList))
        {
            return false;
        }

        pCurrentRenderPass = pCurrentRenderPass->pNext;
    }

    return true;
}

bool createRenderResourceCache(D3D12DeviceType* pDevice, render_resource_cache_t* pOutRenderResourceCache, memory_allocator_t* pMemoryAllocator, const render_context_parameters_t::limits_t* pLimits, const bool notifyOnLimitReach)
{
    uint64_t totalAllocationSizeInBytes = 0u;
    totalAllocationSizeInBytes += sizeof(vertex_buffer_t) * pLimits->maxVertexBufferCount;
    totalAllocationSizeInBytes += sizeof(vertex_format_t) * pLimits->maxVertexFormatCount;
    totalAllocationSizeInBytes += sizeof(shader_binary_t) * pLimits->maxShaderBinaryCount;
    totalAllocationSizeInBytes += sizeof(render_target_t) * pLimits->maxRenderTargetCount;
    totalAllocationSizeInBytes += sizeof(graphics_pipeline_state_t) * pLimits->maxPipelineStateCount;
    totalAllocationSizeInBytes += sizeof(upload_buffer_t) * pLimits->maxUploadBufferCount;
    totalAllocationSizeInBytes += sizeof(render_pass_t) * pLimits->maxRenderPassCount;

    uint8_t* pResourceBlob = (uint8_t*)allocateFromAllocator(pMemoryAllocator, totalAllocationSizeInBytes, alloc_flag_clear_memory);
    if(pResourceBlob == nullptr)
    {
        return false;
    }

    uint64_t offsetInBytes = 0u;
    createDynamicArrayWithPreallocatedMemory<vertex_buffer_t>(&pOutRenderResourceCache->vertexBuffers, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxVertexBufferCount);
    offsetInBytes += sizeof(vertex_buffer_t) * pLimits->maxVertexBufferCount;

    createDynamicArrayWithPreallocatedMemory<vertex_format_t>(&pOutRenderResourceCache->vertexFormats, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxVertexFormatCount);
    offsetInBytes += sizeof(vertex_format_t) * pLimits->maxVertexFormatCount;
    
    createDynamicArrayWithPreallocatedMemory<shader_binary_t>(&pOutRenderResourceCache->shaderBinaries, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxShaderBinaryCount);
    offsetInBytes += sizeof(shader_binary_t) * pLimits->maxShaderBinaryCount;

    createDynamicArrayWithPreallocatedMemory<render_target_t>(&pOutRenderResourceCache->renderTargets, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxRenderTargetCount);
    offsetInBytes += sizeof(render_target_t) * pLimits->maxRenderTargetCount;

    createDynamicArrayWithPreallocatedMemory<graphics_pipeline_state_t>(&pOutRenderResourceCache->pipelineStates, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxPipelineStateCount);
    offsetInBytes += sizeof(graphics_pipeline_state_t) * pLimits->maxPipelineStateCount;

    createDynamicArrayWithPreallocatedMemory<upload_buffer_t>(&pOutRenderResourceCache->uploadBuffers, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxUploadBufferCount);
    offsetInBytes += sizeof(upload_buffer_t) * pLimits->maxUploadBufferCount;

    createDynamicArrayWithPreallocatedMemory<render_pass_t>(&pOutRenderResourceCache->renderPasses, pMemoryAllocator, pResourceBlob + offsetInBytes, pLimits->maxRenderPassCount);
    offsetInBytes += sizeof(render_pass_t) * pLimits->maxRenderPassCount;

    pOutRenderResourceCache->flags = 0u;
    pOutRenderResourceCache->pMemoryAllocator = pMemoryAllocator;
    
    if(notifyOnLimitReach)
    {
        pOutRenderResourceCache->flags |= render_resource_flags_t::notify_on_array_grow;
    }

    pOutRenderResourceCache->pFirstFreeRenderPass = createRenderPassChain((render_pass_t*)pOutRenderResourceCache->renderPasses.pData, pOutRenderResourceCache->renderPasses.capacity);
    if(!initRenderPassChain(pDevice, pOutRenderResourceCache->pFirstFreeRenderPass))
    {
        return false;
    }

    return true;
}

bool createRenderContext(render_context_t* pRenderContext, const render_context_parameters_t* pParameters)
{
    ASSERT_DEBUG(isValidRenderContextParameters(pParameters));

    memory_allocator_t* pAllocator = pParameters->pAllocator;
    if(pAllocator != nullptr)
    {
        pRenderContext->defaultAllocator = *pAllocator;
    }
    else
    {
        createDefaultMemoryAllocator(&pRenderContext->defaultAllocator);
    }

    bool useDebugLayer = pParameters->flags & render_context_flags_t::use_debug_layer;
    if(useDebugLayer)
    {
        useDebugLayer = enableD3D12DebugLayer(&pRenderContext->pDebugLayer);
    }

    if(!createD3D12Factory(&pRenderContext->pFactory))
    {
        return false;
    }

    if(!createD3D12Device(&pRenderContext->pDevice))
    {
        return false;
    }

    if(!createCommandQueue(pRenderContext->pDevice, &pRenderContext->pDefaultDirectCommandQueue, D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    if(!createCommandQueue(pRenderContext->pDevice, &pRenderContext->pDefaultCopyCommandQueue, D3D12_COMMAND_LIST_TYPE_COPY))
    {
        return false;
    }

    if(!createSwapChain(&pRenderContext->swapChain, &pRenderContext->defaultAllocator, pRenderContext->pFactory, pRenderContext->pDevice, pRenderContext->pDefaultDirectCommandQueue, pParameters->pWindowHandle, pParameters->windowWidth, pParameters->windowHeight, pParameters->frameBufferCount))
    {
        return false;
    }

    if(!createShaderCompilerContext(pAllocator, &pRenderContext->shaderCompilerContext))
    {
        return false;
    }

    graphics_frame_parameters_t graphicsFrameParameters = {};
    graphicsFrameParameters.maxRenderPassCount              = pParameters->limits.maxRenderPassCount;
    graphicsFrameParameters.maxUploadBufferCount            = pParameters->limits.maxUploadBufferCount;
    graphicsFrameParameters.maxVertexBufferCount            = pParameters->limits.maxVertexBufferCount;
    graphicsFrameParameters.defaultStagingBufferSizeInBytes = pParameters->limits.defaultStagingBufferSizeInBytes;
    if(!createGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, &graphicsFrameParameters, &pRenderContext->defaultAllocator, &pRenderContext->renderResourceCache, &pRenderContext->shaderCompilerContext, pRenderContext->pDevice, pParameters->frameBufferCount))
    {
        return false;
    }

    if(useDebugLayer)
    {
        if(!setupD3D12DebugLayer(pRenderContext->pDevice))
        {

        }
    }

    const bool notifyOnLimitReach = pParameters->flags & render_context_flags_t::notify_on_limit_reach;
    if(!createRenderResourceCache(pRenderContext->pDevice, &pRenderContext->renderResourceCache, &pRenderContext->defaultAllocator, &pParameters->limits, notifyOnLimitReach))
    {
        return false;
    }

    pRenderContext->frameIndex = 1u;

    return true;
}

graphics_frame_t* beginNextFrame(render_context_t* pRenderContext)
{
    ASSERT_DEBUG(pRenderContext != nullptr);
    ASSERT_DEBUG(pRenderContext->pCurrentGraphicsFrame == nullptr);

    const uint64_t previousFrameIndex = (pRenderContext->frameIndex - 1) % pRenderContext->graphicsFramesCollection.frameCount;
    const uint64_t frameIndex = pRenderContext->frameIndex % pRenderContext->graphicsFramesCollection.frameCount;

    graphics_frame_t* pPreviousFrame = getGraphicsFrameFromGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, previousFrameIndex);
    graphics_frame_t* pGraphicsFrame = getGraphicsFrameFromGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, frameIndex);
    
    flushFrame(pGraphicsFrame);
    resetFrame(pGraphicsFrame);

    const uint32_t currentBackBufferIndex = pRenderContext->swapChain.pSwapChain->GetCurrentBackBufferIndex();

    pRenderContext->pCurrentGraphicsFrame = pGraphicsFrame;
    pGraphicsFrame->frameIndex = pRenderContext->frameIndex;
    pGraphicsFrame->pBackBuffer = pRenderContext->swapChain.pBackBuffers + currentBackBufferIndex;
    ++pRenderContext->frameIndex;

    resetAllocator(&pGraphicsFrame->tempMemoryAllocator);
    return pGraphicsFrame;
}

void finishFrame(render_context_t* pRenderContext, graphics_frame_t* pGraphicsFrame)
{
    ASSERT_DEBUG(pRenderContext != nullptr);
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderContext->pCurrentGraphicsFrame == pGraphicsFrame);
    ASSERT_DEBUG(pGraphicsFrame->openRenderPassCount == 0u);

    const uint32_t frameBufferIndex = pRenderContext->swapChain.pSwapChain->GetCurrentBackBufferIndex();
    transitionResource(pGraphicsFrame->pFrameGeneralGraphicsQueue, &pRenderContext->swapChain.pBackBuffers[frameBufferIndex].resource, D3D12_RESOURCE_STATE_PRESENT);

    pGraphicsFrame->pFrameGeneralGraphicsQueue->Close();
    pRenderContext->pDefaultDirectCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pGraphicsFrame->pFrameGeneralGraphicsQueue);

    render_pass_t* pRenderPass = pGraphicsFrame->pFirstRenderPassToExecute;
    while(pRenderPass)
    {
        pRenderContext->pDefaultDirectCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pRenderPass->pGraphicsCommandList);
        pRenderPass = pRenderPass->pNext;
    }

    COM_CALL(pRenderContext->pDefaultDirectCommandQueue->Signal(pGraphicsFrame->pFrameFence, pGraphicsFrame->frameIndex));

    COM_CALL(pRenderContext->swapChain.pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    COM_CALL(pGraphicsFrame->pFrameFence->SetEventOnCompletion(pGraphicsFrame->frameIndex, pGraphicsFrame->pFrameFinishedEvent));

    pRenderContext->pCurrentGraphicsFrame = nullptr;
}

void* allocateFromRenderResourceCacheGeneric(base_dynamic_array_t* pResourceArray, const bool notifyOnGrow, const char* pResourceName)
{
    void* pRenderResourceData = pushBackFromDynamicArrayDontGrow(pResourceArray, 1u);
    if(pRenderResourceData == nullptr)
    {
        if(notifyOnGrow)
        {
            logWarning("Reached limit of %u %s. Need to allocate from main memory to make room for more %s, new limit = %u", pResourceArray->capacity, pResourceName, calculateNewDynamicArrayCapacity(pResourceArray), pResourceName);
        }

        if(!tryToGrowDynamicArray(pResourceArray))
        {
            return nullptr;
        }

        pRenderResourceData = pushBackFromDynamicArrayDontGrow(pResourceArray, 1u);
        if(pRenderResourceData == nullptr)
        {
            return nullptr;
        }
    }

    return pRenderResourceData;
}

shader_binary_t* allocateShaderBinary(render_resource_cache_t* pRenderResourceCache)
{
    return (shader_binary_t*)allocateFromRenderResourceCacheGeneric(&pRenderResourceCache->shaderBinaries, pRenderResourceCache->flags & render_resource_flags_t::notify_on_array_grow, "shader binaries");
}

vertex_format_t* allocateVertexFormat(render_resource_cache_t* pRenderResourceCache)
{
    return (vertex_format_t*)allocateFromRenderResourceCacheGeneric(&pRenderResourceCache->vertexFormats, pRenderResourceCache->flags & render_resource_flags_t::notify_on_array_grow, "vertex formats");
}

render_pass_t* getFreeRenderPass(render_resource_cache_t* pRenderResourceCache)
{
    if(pRenderResourceCache->pFirstFreeRenderPass == nullptr)
    {
        DebugBreak();
    }

    render_pass_t* pFreeRenderPass = pRenderResourceCache->pFirstFreeRenderPass;
    pRenderResourceCache->pFirstFreeRenderPass = pFreeRenderPass->pNext;
    return pFreeRenderPass;
}

graphics_pipeline_state_t* allocatePipelineState(render_resource_cache_t* pRenderResourceCache)
{
    return (graphics_pipeline_state_t*)allocateFromRenderResourceCacheGeneric(&pRenderResourceCache->pipelineStates, pRenderResourceCache->flags & render_resource_flags_t::notify_on_array_grow, "pipeline state objects");
}

upload_buffer_t* allocateUploadBuffer(render_resource_cache_t* pRenderResourceCache)
{
    return (upload_buffer_t*)allocateFromRenderResourceCacheGeneric(&pRenderResourceCache->uploadBuffers, pRenderResourceCache->flags & render_resource_flags_t::notify_on_array_grow, "upload buffers");
}

render_pass_t* startRenderPass(graphics_frame_t* pGraphicsFrame, const char* pRenderPassName, render_target_t* pRenderTarget)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);

    render_pass_t* pRenderPass = getFreeRenderPass(pGraphicsFrame->pRenderResourceCache);
    if(pRenderPass == nullptr)
    {
        return nullptr;
    }

    ++pGraphicsFrame->openRenderPassCount;

    COM_CALL(pRenderPass->pGraphicsCommandAllocator->Reset());
    COM_CALL(pRenderPass->pGraphicsCommandList->Reset(pRenderPass->pGraphicsCommandAllocator, nullptr));
    pRenderPass->isOpen = true;
    pRenderPass->pName = pRenderPassName;
    pRenderPass->pRenderTarget = pRenderTarget;

    setD3D12ObjectDebugName(pRenderPass->pGraphicsCommandList, pRenderPassName);    
    addBeginMarker(pRenderPass->pGraphicsCommandList, pRenderPassName);
    return pRenderPass;
}

void endRenderPass(graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGraphicsFrame->openRenderPassCount > 0);
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderPass->isOpen);

    pRenderPass->isOpen = false;
    --pGraphicsFrame->openRenderPassCount;

    transitionResource(pRenderPass->pGraphicsCommandList, &pGraphicsFrame->pBackBuffer->resource, D3D12_RESOURCE_STATE_PRESENT);

    addEndMarker(pRenderPass->pGraphicsCommandList);
    COM_CALL(pRenderPass->pGraphicsCommandList->Close());
}

bool isColorRenderTarget(render_target_t* pRenderTarget)
{
    //FK: TODO
    return true;
}

uint32_t getVertexAttributeTypeSizeInBytes(const vertex_attribute_type_t attributeType)
{
    switch(attributeType)
    {
        case vertex_attribute_type_t::float32:
            return 4u;
        default:
            DebugBreak();
    }

    UNREACHABLE_CODE();
    return 0u;
}

uint32_t calculateVertexStrideSizeInBytes(const vertex_format_t* pVertexFormat)
{
    uint32_t strideSizeInBytes = 0u;
    for(uint32_t attributeIndex = 0u; attributeIndex < pVertexFormat->vertexAttributeCount; ++attributeIndex)
    {
        strideSizeInBytes += pVertexFormat->pVertexAttributes[attributeIndex].count * getVertexAttributeTypeSizeInBytes(pVertexFormat->pVertexAttributes[attributeIndex].type);
    }

    return strideSizeInBytes;
}

void bindVertexBuffer(render_pass_t* pRenderPass, vertex_buffer_t* pVertexBuffer, const vertex_format_t* pVertexFormat, uint32_t slotIndex)
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = pVertexBuffer->bufferResource.pResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes    = pVertexBuffer->sizeInBytes;
    vertexBufferView.StrideInBytes  = calculateVertexStrideSizeInBytes(pVertexFormat);
    pRenderPass->pGraphicsCommandList->IASetVertexBuffers(slotIndex, 1u, &vertexBufferView);
}

void clearColorRenderTarget(render_pass_t* pRenderPass, render_target_t* pRenderTarget, const float r, const float g, const float b, const float a)
{
    ASSERT_DEBUG(pRenderTarget != nullptr);
    ASSERT_DEBUG(isColorRenderTarget(pRenderTarget));
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderPass->isOpen);

    const FLOAT colorValues[4] = {r, g, b, a};

    transitionResource(pRenderPass->pGraphicsCommandList, &pRenderTarget->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
    pRenderPass->pGraphicsCommandList->ClearRenderTargetView(pRenderTarget->cpuDescriptorHandle, colorValues, 0, nullptr);
}

void executeRenderPass(graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(!pRenderPass->isOpen);

    if(pGraphicsFrame->pFirstRenderPassToExecute == nullptr)
    {
        pRenderPass->pNext = pGraphicsFrame->pFirstRenderPassToExecute;
        pGraphicsFrame->pFirstRenderPassToExecute = pRenderPass;
    }

    if(pGraphicsFrame->pLastRenderPassToExecute == nullptr)
    {
        pGraphicsFrame->pLastRenderPassToExecute = pRenderPass;
    }
    else
    {
        pGraphicsFrame->pLastRenderPassToExecute->pNext = pRenderPass;
        pGraphicsFrame->pLastRenderPassToExecute = pRenderPass;
    }
}

vertex_buffer_t* createVertexBuffer(graphics_frame_t* pGraphicsFrame, const upload_buffer_t* pUploadBuffer, const uint32_t uploadBufferOffset = 0u, uint32_t sizeInBytes = 0u)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pUploadBuffer != nullptr);

    vertex_buffer_t* pVertexBuffer = (vertex_buffer_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, sizeof(vertex_buffer_t));
    if(pVertexBuffer == nullptr)
    {
        return nullptr;
    }

    if(sizeInBytes == 0u)
    {
        sizeInBytes = pUploadBuffer->sizeInBytes;
    }

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment          = 0u;
    desc.DepthOrArraySize   = 1u;
    desc.Height             = 1u;
    desc.MipLevels          = 1u;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.SampleDesc.Count   = 1u;
    desc.SampleDesc.Quality = 0u;
    desc.Width              = sizeInBytes;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    if(COM_CALL(pGraphicsFrame->pDevice->CreateCommittedResource1(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, nullptr, IID_PPV_ARGS(&pVertexBuffer->bufferResource.pResource))) != S_OK)
    {
        freeFromAllocator(pGraphicsFrame->pMemoryAllocator, pVertexBuffer);
        return nullptr;
    }

    pVertexBuffer->bufferResource.currentState = D3D12_RESOURCE_STATE_COMMON;
    pVertexBuffer->sizeInBytes = sizeInBytes;

    #if 1
    transitionResource(pGraphicsFrame->pFrameGeneralGraphicsQueue, &pVertexBuffer->bufferResource, D3D12_RESOURCE_STATE_COPY_DEST);
    pGraphicsFrame->pFrameGeneralGraphicsQueue->CopyBufferRegion(pVertexBuffer->bufferResource.pResource, 0u, pUploadBuffer->bufferResource.pResource, uploadBufferOffset, sizeInBytes);
    transitionResource(pGraphicsFrame->pFrameGeneralGraphicsQueue, &pVertexBuffer->bufferResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    #else
    transitionResource(pGraphicsFrame->pFrameGeneralCopyQueue, &pVertexBuffer->bufferResource, D3D12_RESOURCE_STATE_COPY_DEST);
    pGraphicsFrame->pFrameGeneralCopyQueue->CopyBufferRegion(pVertexBuffer->bufferResource.pResource, 0u, pUploadBuffer->pBufferResource->pResource, pUploadBuffer->startStagingBufferByteIndex, bufferSizeInBytes);
    #endif

    return pVertexBuffer;
}

vertex_format_t* createVertexFormat(graphics_frame_t* pGraphicsFrame, const vertex_attribute_entry_t* pVertexAttributes, const uint32_t vertexAttributeCount)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pVertexAttributes != nullptr);
    ASSERT_DEBUG(vertexAttributeCount > 0u);
    
    vertex_format_t* pVertexFormat = allocateVertexFormat(pGraphicsFrame->pRenderResourceCache);
    if(pVertexFormat == nullptr)
    {
        return nullptr;
    }

    memcpy(pVertexFormat->pVertexAttributes, pVertexAttributes, sizeof(vertex_attribute_entry_t) * vertexAttributeCount);
    pVertexFormat->vertexAttributeCount = vertexAttributeCount;

    return pVertexFormat;
}

upload_buffer_t* createUploadBuffer(graphics_frame_t* pGraphicsFrame, void* pData, const uint32_t dataSizeInBytes, upload_buffer_flags_t flags = upload_buffer_flag_none)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(dataSizeInBytes > 0u);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment          = 0u;
    desc.Height             = 1u;
    desc.DepthOrArraySize   = 1u;
    desc.MipLevels          = 1u;
    desc.SampleDesc.Count   = 1u;
    desc.SampleDesc.Quality = 0u;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Width              = dataSizeInBytes;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    ID3D12Resource* pResource = nullptr;
    if(COM_CALL(pGraphicsFrame->pDevice->CreateCommittedResource1(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, nullptr, IID_PPV_ARGS(&pResource))) != S_OK)
    {
        return nullptr;
    }

    upload_buffer_t* pUploadBuffer = allocateUploadBuffer(pGraphicsFrame->pRenderResourceCache);
    if(pUploadBuffer == nullptr)
    {
        pResource->Release();
        return nullptr;
    }

    void* pUploadBufferData = nullptr;

    D3D12_RANGE mapRange = {};
    mapRange.End = dataSizeInBytes;
    if(COM_CALL(pResource->Map(0, &mapRange, &pUploadBufferData)) != S_OK)
    {
        pResource->Release();
        freeUploadBuffer(pGraphicsFrame->pRenderResourceCache, pUploadBuffer);
        return nullptr;
    }

    if(pData != nullptr)
    {
        memcpy(pUploadBufferData, pData, dataSizeInBytes);
    }

    pUploadBuffer->sizeInBytes                  = dataSizeInBytes;
    pUploadBuffer->pData                        = pUploadBufferData;
    pUploadBuffer->bufferResource.pResource     = pResource;
    pUploadBuffer->bufferResource.currentState  = D3D12_RESOURCE_STATE_GENERIC_READ;

    pUploadBuffer->pNext = pGraphicsFrame->pFirstUploadBuffer;
    pGraphicsFrame->pFirstUploadBuffer = pUploadBuffer;
    
    return pUploadBuffer;
}

upload_buffer_t* createUploadBuffer(graphics_frame_t* pGraphicsFrame, const uint32_t dataSizeInBytes)
{
    return createUploadBuffer(pGraphicsFrame, nullptr, dataSizeInBytes);
}

struct shader_compilation_parameters_t
{
    const char*     pShaderProfile;
    const char*     pFilePath;
    const char*     pShaderSourceCode;
    const char*     pEntryPoint;
    const char*     pDefines;
};

result_t<memory_buffer_t> readWholeFileIntoNewBuffer(memory_allocator_t* pAllocator, const char* pFilePath)
{
    FILE* pShaderFileHandle = fopen(pFilePath, "rb");
    if(pShaderFileHandle == nullptr)
    {
        return result_status_t::file_not_found;
    }

    fseek(pShaderFileHandle, 0, SEEK_END);
    const uint64_t shaderFileSizeInBytes = ftell(pShaderFileHandle);
    fseek(pShaderFileHandle, 0, SEEK_SET);

    char* pShaderSourceContent = (char*)allocateFromAllocator(pAllocator, shaderFileSizeInBytes + 1);
    if(pShaderSourceContent == nullptr)
    {
        fclose(pShaderFileHandle);
        return result_status_t::out_of_memory;
    }

    fread(pShaderSourceContent, 1u, shaderFileSizeInBytes, pShaderFileHandle);
    fclose(pShaderFileHandle);

    pShaderSourceContent[shaderFileSizeInBytes] = 0;

    memory_buffer_t fileContent = {};
    fileContent.pData = pShaderSourceContent;
    fileContent.sizeInBytes = shaderFileSizeInBytes + 1;

    return fileContent;
}

struct dxc_arguments_t
{
    const wchar_t** ppArguments;
    uint32_t        argumentCount;
};

uint32_t findCharacterCountInString(const char* pString, const char needle)
{
    uint32_t count = 0u;
    while(*pString)
    {
        if(*pString++ == needle)
        {
            ++count;
        }
    }

    return count;
}

result_t<wchar_t*> formatWideStringIntoNewBuffer(memory_allocator_t* pAllocator, const wchar_t* pFormat, ...)
{
    wchar_t tempMemoryBuffer[1024] = {};
    const uint64_t tempMemoryBufferSizeInCharacters = sizeof(tempMemoryBuffer) >> 1;
    va_list vaList;
    va_start(vaList, pFormat);
    const uint32_t charactersPrinted = vswprintf_s(tempMemoryBuffer, tempMemoryBufferSizeInCharacters, pFormat, vaList);
    va_end(vaList);

    wchar_t* pTextBuffer = (wchar_t*)allocateFromAllocator(pAllocator, charactersPrinted * sizeof(wchar_t), alloc_flag_clear_memory);
    if(pTextBuffer == nullptr)
    {
        return result_status_t::out_of_memory;
    }

    wcscpy(pTextBuffer, tempMemoryBuffer);
    return pTextBuffer;
}

const char* findFirstInstanceOfCharacterInString(const char* pString, const char needle)
{
    while(*pString)
    {
        if(*pString == needle)
        {
            return pString;
        }

        ++pString;
    }

    return nullptr;
}

bool isValidCompilerDefines(const char* pCompilerDefines)
{
    bool lastCharWasDelimiter = false;
    while(*pCompilerDefines)
    {
        if(*pCompilerDefines++ == ';')
        {
            if(lastCharWasDelimiter)
            {
                return false;
            }

            lastCharWasDelimiter = true;
        }
        else
        {
            lastCharWasDelimiter = false;
        }
    }

    return true;
}

template<typename T>
T rangeCheckCast(size_t value)
{
    ASSERT_ALWAYS(value <= std::numeric_limits<T>::max());
    ASSERT_ALWAYS(value >= std::numeric_limits<T>::min());
    return (T)value;
}

void freeCompilerArguments(memory_allocator_t* pMemoryAllocator, dxc_arguments_t* pArguments)
{
    for(uint32_t argumentIndex = 0u; argumentIndex < pArguments->argumentCount; ++argumentIndex)
    {
        freeFromAllocator(pMemoryAllocator, (void*)pArguments->ppArguments[argumentIndex]);
    }

    freeFromAllocator(pMemoryAllocator, pArguments->ppArguments);
}

result_t<dxc_arguments_t> generateCompilerArgumentsIntoNewBuffer(memory_allocator_t* pMemoryAllocator, const shader_compilation_parameters_t* pParameters)
{
    uint32_t argumentCount = 2u; //entry point + shader profile
    dxc_arguments_t arguments = {};

    if(pParameters->pDefines != nullptr && pParameters->pDefines[0] != 0)
    {
        if(!isValidCompilerDefines(pParameters->pDefines))
        {
            logError("Invalid shader compiler defines '%s'.", pParameters->pDefines);
            return result_status_t::invalid_arguments;
        }

        argumentCount += 1u;
        argumentCount += findCharacterCountInString(pParameters->pDefines, ';');
    }

    wchar_t** ppArguments = (wchar_t**)allocateFromAllocator(pMemoryAllocator, sizeof(wchar_t*) * argumentCount);
    if(ppArguments == nullptr)
    {
        return result_status_t::out_of_memory;
    }

    ppArguments[0] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-E %hs", pParameters->pEntryPoint);
    ppArguments[1] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-T %hs", pParameters->pShaderProfile);

    if(pParameters->pDefines != nullptr)
    {
        const char* pDefineStart    = pParameters->pDefines;
        const char* pDefineEnd      = pParameters->pDefines + strlen(pParameters->pDefines);

        const char* pCurrentDefineStart = pDefineStart;

        if(ppArguments[0] == nullptr || ppArguments[1] == nullptr)
        {
            goto cleanup_and_return_out_of_memory;
            return result_status_t::out_of_memory;
        }
        
        for(uint32_t argumentIndex = 2u; argumentIndex < argumentCount; ++argumentIndex)
        {
            const char* pCurrentDefineEnd = findFirstInstanceOfCharacterInString(pCurrentDefineStart, ';');
            const int defineLength = rangeCheckCast<int>(pCurrentDefineEnd - pCurrentDefineStart);
            ppArguments[argumentIndex] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-D %.*s", defineLength, pCurrentDefineStart);
            if(ppArguments[argumentIndex] == nullptr)
            {
                goto cleanup_and_return_out_of_memory;
            }
            pCurrentDefineStart = pCurrentDefineEnd + 1;
        }
    }

    arguments.argumentCount = argumentCount;
    arguments.ppArguments   = (const wchar_t**)ppArguments;
    return arguments;

cleanup_and_return_out_of_memory:
    for(uint32_t argumentIndex = 0u; argumentIndex < argumentCount; ++argumentIndex)
    {
        freeFromAllocator(pMemoryAllocator, ppArguments[argumentIndex]);
    }
    freeFromAllocator(pMemoryAllocator, ppArguments);

    return result_status_t::out_of_memory;
}

void destroyShaderBinary(graphics_frame_t* pGraphicsFrame, shader_binary_t* pShaderBinary)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pShaderBinary != nullptr);

    freeFromAllocator(pGraphicsFrame->pMemoryAllocator, (void*)pShaderBinary->pShaderBlob);
    freeShaderBinary(pGraphicsFrame, pShaderBinary);
}

shader_binary_t* loadAndCompileShaderCodeFromFile(graphics_frame_t* pGraphicsFrame, const shader_compilation_parameters_t* pParameters)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pParameters != nullptr);
    ASSERT_DEBUG(pParameters->pEntryPoint != nullptr)
    ASSERT_DEBUG(pParameters->pFilePath != nullptr);
    ASSERT_DEBUG(pParameters->pShaderProfile != nullptr);
    result_t<memory_buffer_t> shaderCodeResult = readWholeFileIntoNewBuffer(&pGraphicsFrame->tempMemoryAllocator, pParameters->pFilePath);
    if(!isResultSuccessful(shaderCodeResult))
    {
        logError("Could not read shader file '%s' - error: %s.", pParameters->pFilePath, getResultString(shaderCodeResult));
        return nullptr;
    }

    DxcBuffer shaderSourceBuffer = {};
    shaderSourceBuffer.Ptr = shaderCodeResult.value.pData;
    shaderSourceBuffer.Size = shaderCodeResult.value.sizeInBytes;

    result_t<dxc_arguments_t> compileArgumentsResult = generateCompilerArgumentsIntoNewBuffer(&pGraphicsFrame->tempMemoryAllocator, pParameters);
    if(!isResultSuccessful(compileArgumentsResult))
    {
        logError("Could not generate compiler arguments for shader file '%s' - error: %s.", pParameters->pFilePath, getResultString(compileArgumentsResult));
        return nullptr;
    }

    IDxcResult* pCompileResult = nullptr;

    shader_compiler_context_t* pShaderCompilerContext = pGraphicsFrame->pShaderCompilerContext;
    const HRESULT compileResult = COM_CALL(pShaderCompilerContext->pShaderCompiler->Compile(&shaderSourceBuffer, compileArgumentsResult.value.ppArguments, compileArgumentsResult.value.argumentCount, pShaderCompilerContext->pIncludeHandler, IID_PPV_ARGS(&pCompileResult)));
    
    freeCompilerArguments(&pGraphicsFrame->tempMemoryAllocator, &compileArgumentsResult.value);
    freeFromAllocator(&pGraphicsFrame->tempMemoryAllocator, shaderCodeResult.value.pData);
    
    if(compileResult != S_OK)
    {
        logError("Shader compiler couldn't compile shader '%s' - error: %s.", pParameters->pFilePath, getHResultString(compileResult));
        return nullptr;
    }

    if(pCompileResult->HasOutput(DXC_OUT_ERRORS))
    {
        IDxcBlobEncoding* pErrorBufferEncoding = nullptr;
        const HRESULT getErrorBufferResult = COM_CALL(pCompileResult->GetErrorBuffer(&pErrorBufferEncoding));
        if(getErrorBufferResult != S_OK)
        {
            logError("Shader compilation of shader '%s' failed but the error couldn't get retrieved - error: %s.", pParameters->pFilePath, getHResultString(getErrorBufferResult));
            pCompileResult->Release();
            return nullptr;
        }

        if(pErrorBufferEncoding != nullptr && pErrorBufferEncoding->GetBufferSize() > 0)
        {
            logError("Shader compilation of shader '%s' failed because: %s\n", pParameters->pFilePath, (char*)pErrorBufferEncoding->GetBufferPointer());
            pErrorBufferEncoding->Release();
            pCompileResult->Release();
            return nullptr;
        }
    }

    IDxcBlob* pCompileShaderBlob = nullptr;
    const HRESULT getBlobOutputResult = COM_CALL(pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pCompileShaderBlob), nullptr));
    pCompileResult->Release();

    if(getBlobOutputResult != S_OK)
    {
        pCompileShaderBlob->Release();
        logError("Shader compilation of shader '%s' was successful but there's no shader blob. GetOutput() error: %s", pParameters->pFilePath, getHResultString(getBlobOutputResult));
        return nullptr;
    }

#if 0
    ID3D12ShaderReflection* pShaderReflection = nullptr;
    const HRESULT shaderReflectionResult = D3DReflect(pCompileShaderBlob->GetBufferPointer(), pCompileShaderBlob->GetBufferSize(), IID_PPV_ARGS(&pShaderReflection));
    if(shaderReflectionResult != S_OK)
    {
        pCompileShaderBlob->Release();
        logError("Could not reflect shader '%s' - %s", pParameters->pFilePath, getHResultString(shaderReflectionResult));
        return nullptr;
    }
#endif
    const uint32_t shaderBlobSizeInBytes = rangeCheckCast<uint32_t>(pCompileShaderBlob->GetBufferSize());
    uint8_t* pShaderBlobCopy = (uint8_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, pCompileShaderBlob->GetBufferSize());
    if(pShaderBlobCopy == nullptr)
    {
        logError("Shader compilation of shader '%s' was successful but we ran out of memory trying to copy the shader blob.", pParameters->pFilePath);
        pCompileShaderBlob->Release();
        return nullptr;
    }

    memcpy(pShaderBlobCopy, pCompileShaderBlob->GetBufferPointer(), pCompileShaderBlob->GetBufferSize());
    pCompileShaderBlob->Release();

    shader_binary_t* pShaderBinary = allocateShaderBinary(pGraphicsFrame->pRenderResourceCache);
    if(pShaderBinary == nullptr)
    {
        return nullptr;
    }

    pShaderBinary->pShaderBlob = pShaderBlobCopy;
    pShaderBinary->shaderBlobSizeInBytes = shaderBlobSizeInBytes;

    return pShaderBinary;
}

void destroyFence(ID3D12Fence* pFence)
{
    COM_RELEASE(pFence);
}

void destroyCommandAllocator(ID3D12CommandAllocator* pCommandAllocator)
{
    COM_RELEASE(pCommandAllocator);
}

void shutdownRenderContext(render_context_t* pRenderContext)
{
    destroyGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection);
    destroySwapChain(&pRenderContext->swapChain);
    COM_RELEASE(pRenderContext->pDefaultDirectCommandQueue);
    COM_RELEASE(pRenderContext->pDefaultCopyCommandQueue);
    COM_RELEASE(pRenderContext->pFactory);
    COM_RELEASE(pRenderContext->pDebugLayer);
    COM_RELEASE(pRenderContext->pDevice);

    clearMemoryWithZeroes(pRenderContext);
}

void resizeBackBuffer(render_context_t* pRenderContext, const uint32_t width, const uint32_t height)
{
    flushAllFrames(pRenderContext);

    for(uint32_t bufferIndex = 0u; bufferIndex < pRenderContext->swapChain.backBufferCount; ++bufferIndex)
    {
        destroyRenderTarget(&pRenderContext->swapChain.pBackBuffers[bufferIndex]);
    }

    pRenderContext->swapChain.pSwapChain->ResizeBuffers(0u, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    pRenderContext->swapChain.width = width;
    pRenderContext->swapChain.height = height;

    resetDescriptorHeap(&pRenderContext->swapChain.backBufferRenderTargetDescriptorHeap);

    for(uint32_t bufferIndex = 0u; bufferIndex < pRenderContext->swapChain.backBufferCount; ++bufferIndex)
    {
        ID3D12Resource* pFrameBuffer = nullptr;
        COM_CALL(pRenderContext->swapChain.pSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pFrameBuffer)));

        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = getNextCPUDescriptorHandle(&pRenderContext->swapChain.backBufferRenderTargetDescriptorHeap);
        pRenderContext->pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, descriptorHandle);

        initializeRenderTarget(&pRenderContext->swapChain.pBackBuffers[bufferIndex], pFrameBuffer, &descriptorHandle, D3D12_RESOURCE_STATE_PRESENT);
    }
}

render_context_parameters_t createDefaultRenderContextParameters(HWND pWindowHandle, const uint32_t frameBufferCount, const uint32_t windowWidth, const uint32_t windowHeight, const bool useDebugLayer)
{
    render_context_parameters_t parameters = {};

    if(useDebugLayer)
    {
        parameters.flags = render_context_flags_t::use_debug_layer;
    }

    parameters.frameBufferCount                         = frameBufferCount;
    parameters.windowHeight                             = windowHeight;
    parameters.windowWidth                              = windowWidth;
    parameters.pWindowHandle                            = pWindowHandle;
    parameters.limits.maxUploadBufferCount              = 32u;
    parameters.limits.maxVertexBufferCount              = 32u;
    parameters.limits.maxRenderPassCount                = 32u;
    parameters.limits.maxPipelineStateCount             = 32u;
    parameters.limits.maxRenderTargetCount              = 32u;
    parameters.limits.maxShaderBinaryCount              = 32u;
    parameters.limits.maxVertexFormatCount              = 32u;
    parameters.limits.defaultStagingBufferSizeInBytes   = 4096u;

    return parameters;
}