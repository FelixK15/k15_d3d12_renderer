#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#define USE_D3D12_DEBUG 1
#define USE_VALIDATION 1
#define CLEAR_NEW_MEMORY_WITH_ZEROES 1
#define USE_DEBUG_ASSERTS 1
#define FORCE_VALIDATION_BREAKS_OFF 0

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
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
#pragma comment(lib, "x64/WinPixEventRuntime.lib")

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#if USE_D3D12_DEBUG
#define COM_CALL(func) logOnHResultError(func, #func, __FILE__, __LINE__)
#else
#define COM_CALL(func) func
#endif

#define COM_RELEASE(ptr)if(ptr != nullptr) { (ptr)->Release(); ptr = nullptr; }
#define NO_DISCARD [[nodiscard]]

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

#define GET_MIN(a,b) (a)>(b)?(b):(a)
#define GET_MAX(a,b) (a)<(b)?(b):(a)

struct memory_allocator_t;
struct render_context_t;
struct graphics_frame_t;

typedef void*(*allocate_from_memory_allocator_fnc)(memory_allocator_t*, uint64_t, uint64_t);
typedef void(*free_from_memory_allocator_fnc)(memory_allocator_t*, void*);

typedef ID3D12Device10  D3D12DeviceType;
typedef ID3D12Debug6    D3D12DebugType;
typedef IDXGIFactory7   DXGIFactoryType;
typedef IDXGISwapChain4 DXGISwapChainType;

typedef uint32_t hash32_t;

constexpr uint64_t  maxVertexAttributeCount         = 16u;
constexpr uint64_t  maxShaderBindingPoints          = 32u;
constexpr uint64_t  maxShaderBindingPointNameLength = 32u;
constexpr uint64_t  defaultAllocationAlignment      = 16u;
constexpr uint32_t  invalidResourceHandleValue      = ~0u;

constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorNullptrHandle = {0u};

enum index_format_t : uint8_t
{
    unsigned_int_16bit,
    unsigned_int_32bit
};

enum alloc_flags_t : uint8_t
{
    none         = 0x0,
    clear_memory = 0x1
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

template<typename T>
struct ComPtr
{
    ComPtr(T* pPointer) : pComPointer(pPointer)
    {

    }
    
    ~ComPtr()
    {
        COM_RELEASE(pComPointer);
    }
    
    ComPtr<T>& operator=(const T* pPointer)
    {
        if(pComPointer)
        {
            COM_RELEASE(pComPointer);
        }

        pComPointer = pPointer;
    }

    ComPtr<T>& operator=(const ComPtr<T>& comPtr)
    {
        if(pComPointer)
        {
            COM_RELEASE(pComPointer);
        }

        pComPointer = comPtr.pComPointer;
    }

    bool operator==(const ComPtr<T>& comPtr) { return pComPointer == comPtr.pComPointer; }
    bool operator==(T* pPointer)             { return pComPointer == pPointer; }

    bool operator!=(const ComPtr<T>& comPtr) { return pComPointer != comPtr.pComPointer; }
    bool operator!=(T* pPointer)             { return pComPointer != pPointer; }

    T*  get()        { return pComPointer; }
    T*  operator->() { return pComPointer; }
    T** operator&()  { return &pComPointer; }

    T* pComPointer;
};

template<typename T, typename BASE_TYPE>
struct flags_t
{
    flags_t<T, BASE_TYPE>& operator=(const BASE_TYPE flagsValue)
    {
        value = flagsValue;
        return *this;
    }

    bool isFlagSet(const T flag) const
    {
        return (value & (BASE_TYPE)flag) > 0;
    }

    void clearFlag(const T flag)
    {
        value &= ~(BASE_TYPE)flag;
    }

    void setFlag(const T flag)
    {
        value |= (BASE_TYPE)flag;
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
    return rawFlags & (BASE_TYPE)flag;
}

template<typename T>
struct flags8_t : flags_t<T, uint8_t>
{
    flags8_t()
    {
        value = 0u;
    }

    flags8_t(T flag)
    {
        value = (uint8_t)flag;
    }

    flags8_t(const flags8_t& other)
    {
        value = other.value;
    }

    flags8_t(const uint8_t flagsValue)
    {
        value = flagsValue;
    }

    flags8_t<T>& operator=(const uint8_t flagsValue)
    {
        value = flagsValue;
        return *this;
    }

    flags8_t<T>& operator=(const flags8_t& other)
    {
        value = other.value;
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

template<typename T>
struct linked_list_node_t
{
    T* pNext;
};

struct d3d12_descriptor_handle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
};

struct d3d12_resource_t
{
    ID3D12Resource*             pResource;
    d3d12_descriptor_handle     descriptorHandle;
    D3D12_RESOURCE_STATES       currentState;
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

struct uint3_t
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct render_target_t : public linked_list_node_t<render_target_t>
{
    uint3_t                     dimensions;
    d3d12_resource_t            resource;
    D3D12_CPU_DESCRIPTOR_HANDLE colorBufferHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE depthBufferHandle;
};

struct shader_compiler_context_t
{
    IDxcUtils*          pUtils;
    IDxcCompiler3*      pShaderCompiler;
    IDxcIncludeHandler* pIncludeHandler;
};

enum class gpu_buffer_flag_t : uint8_t
{
    marked_as_free  = 0x01,
    is_mapped       = 0x02
};

enum class gpu_buffer_usage_t : uint8_t
{
    vertex_buffer,
    index_buffer,
    constant_buffer,
    storage_buffer
};

enum class gpu_memory_usage_hint_t : uint8_t
{
    cpuReadAccess,              //CPU read is optimized
    cpuWriteGpuReadAccess,      //CPU write + GPU read is optimized
    gpuExclusiveAccess,         //CPU can't read or write
};

enum class gpu_texture_format_t : uint8_t
{
    R8,
    R8G8B8A8,
    R16,
    R16G16,
    R16G16B16A16,
    R32,
    R32G32,
    R32G32B32,
    R32G32B32A32,
    R10G10B10A2,
    D24S8,
    D32,
    BC1,
    BC2,
    BC3,
    BC4,
    BC5,
    BC7
};

enum class gpu_texture_flag_t : uint8_t
{
    use_as_color_render_target      = 0x01,
    use_as_depth_render_target      = 0x02,
    use_as_unordered_access_view    = 0x04,
    marked_as_free                  = 0x08
};

enum class gpu_texture_format_type_t : uint8_t
{
    typeless,
    floating_point,
    normalized_unsigned_int,
    unsigned_int,
    normalized_signed_int,
    signed_int,
};

enum gpu_texture_format_type_flag_t : uint8_t
{
    all                         = 0xff,
    typeless                    = 0x02,
    floating_point              = 0x04,
    normalized_unsigned_int     = 0x08,
    unsigned_int                = 0x10,
    normalized_signed_int       = 0x20,
    signed_int                  = 0x40,

    all_except_floating_point   = all & (~floating_point)
};

enum class gpu_buffer_usage_flag_t : uint8_t
{
    none = 0,
    shader_accessible
};

struct gpu_buffer_t : public linked_list_node_t<gpu_buffer_t>
{
    flags8_t<gpu_buffer_flag_t>         flags;
    flags8_t<gpu_buffer_usage_flag_t>   usageFlags;
    gpu_memory_usage_hint_t             memoryUsageHint;
    gpu_buffer_usage_t                  bufferUsage;
    d3d12_resource_t                    resource;
    uint32_t                            sizeInBytes;
    const char*                         pName;
};

struct gpu_texture_t : public linked_list_node_t<gpu_texture_t>
{
    d3d12_resource_t                resource;
    uint3_t                         dimensions;
    gpu_texture_format_t            format;
    gpu_texture_format_type_t       formatType;
    flags8_t<gpu_texture_flag_t>    flags;
    gpu_memory_usage_hint_t         memoryUsageHint;
    uint32_t                        sizeInBytes;
    const char*                     pName;
};

enum class texture_sampler_filter_type_t : uint8_t
{
    point,
    linear,
    anisotropic
};

enum class texture_sampler_address_mode_type_t : uint8_t
{
    wrap,
    mirror,
    clamp,
    border
};

struct texture_sampler_parameter_t
{
    texture_sampler_filter_type_t       mipmapFilter;
    texture_sampler_filter_type_t       minifactionFilter;
    texture_sampler_filter_type_t       magnificationFilter;
    texture_sampler_address_mode_type_t addressModeU;
    texture_sampler_address_mode_type_t addressModeV;
    texture_sampler_address_mode_type_t addressModeW;
};

struct texture_sampler_t : public linked_list_node_t<texture_sampler_t>
{
    d3d12_descriptor_handle     descriptorHandle;
    texture_sampler_filter_type_t       mipmapFilter;
    texture_sampler_filter_type_t       minifactionFilter;
    texture_sampler_filter_type_t       magnificationFilter;
    texture_sampler_address_mode_type_t addressModeU;
    texture_sampler_address_mode_type_t addressModeV;
    texture_sampler_address_mode_type_t addressModeW;
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
    render_target_t*                pBackBufferRenderTargets;
    DXGISwapChainType*              pSwapChain;
    D3D12_CPU_DESCRIPTOR_HANDLE*    pBackBufferRenderTargetHandles;
    uint32_t                        width;
    uint32_t                        height;
    uint8_t                         backBufferCount;
};

struct viewport_t
{
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    float minDepth;
    float maxDepth;
};

struct scissor_t
{
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

enum class topology_t : uint8_t
{
    point_list,
    line_list,
    line_strip,
    triangle_list,
    triangle_strip
};

enum class bound_resource_type_t : uint8_t
{
    constant_buffer,
    texture,
    sampler
};

struct shader_binding_point_t
{
    char                        name[maxShaderBindingPointNameLength];
    uint16_t                    slot;
    uint16_t                    space;
    bound_resource_type_t       type;
};

struct graphics_pipeline_t : public linked_list_node_t<graphics_pipeline_t>
{
    ID3D12PipelineState*    pPipelineState;
    ID3D12RootSignature*    pRootSignature;
    const char*             pName;
    topology_t              topology;
    shader_binding_point_t* pShaderBindingPoints;
    uint32_t                shaderBindingPointCount;
};

struct bound_resource_t
{
    d3d12_resource_t*           pResource;
    d3d12_descriptor_handle     descriptorHandle;
    bound_resource_type_t       type;
    uint32_t                    registerIndex;
    uint32_t                    registerSpace;
};

struct render_state_t
{
    uint8_t                    boundResourceCount;
    bound_resource_t           boundResources[32];
    viewport_t                 viewport;
    scissor_t                  scissor;
    render_target_t*           pRenderTarget;
    const graphics_pipeline_t* pGraphicsPipeline;
};

struct render_pass_t : public linked_list_node_t<render_pass_t>
{
    render_state_t              state;
    ID3D12Fence*                pRenderPassFence;
    ID3D12CommandAllocator*     pGraphicsCommandAllocator;
    ID3D12GraphicsCommandList*  pGraphicsCommandList;
    const char*                 pName;
    bool                        isOpen;
    graphics_frame_t*           pGraphicsFrame;
};

enum class vertex_attribute_t : uint8_t
{
    position,
    color,
    normal,
    texcoord,
    count
};

enum class vertex_attribute_type_t : uint8_t
{
    float32,
    count
};

enum class vertex_attribute_frequency_t : uint8_t
{
    vertex = 0,
    instance,
    count
};

struct vertex_attribute_entry_t
{
    vertex_attribute_t              attribute;
    vertex_attribute_type_t         type;
    vertex_attribute_frequency_t    frequency;
    uint32_t                        offsetInBytes;
    uint32_t                        count;
};

struct vertex_format_t : public linked_list_node_t<vertex_format_t>
{
    D3D12_INPUT_ELEMENT_DESC    pInputElementDescs[maxVertexAttributeCount];
    uint32_t                    inputElementCount;
};

struct shader_binary_t : public linked_list_node_t<shader_binary_t>
{
    shader_binding_point_t  bindingPoints[maxShaderBindingPoints];
    const uint8_t*          pShaderBlob;
    const char*             pName;
    uint32_t                shaderBlobSizeInBytes;
    uint32_t                bindingPointCount;
};

struct render_pass_parameters_t
{
    render_target_t* pRenderTarget; // nullptr = backbuffer
    shader_binary_t* pVertexShader;
    shader_binary_t* pPixelShader;
};

struct graphics_pipeline_parameters_t
{
    const char*         pName;
    shader_binary_t*    pVertexShader;
    shader_binary_t*    pPixelShader;
    vertex_format_t*    pVertexFormat;
    topology_t          topology;
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
struct hash_map_node_t
{
    hash32_t    hash;
    void*       pNext;
    T           value;
};

template<typename T>
struct hash_map_t
{
    memory_allocator_t*  pMemoryAllocator;
    hash_map_node_t<T>** ppBaseNodes;
    hash_map_node_t<T>*  pFreeNodes;
    uint32_t             count;
    uint32_t             capacity;
};

template<typename T>
struct hash_map_entry_t
{
    T value;
    uint32_t nodeIndex;
    hash32_t hash;
    bool isNew;
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
    notify_on_array_grow    = 0x01
};

struct render_resource_cache_t
{
    memory_allocator_t*                             pMemoryAllocator;
    dynamic_array_t<gpu_texture_t>                  gpuTextures;
    dynamic_array_t<gpu_buffer_t>                   gpuBuffers;
    dynamic_array_t<render_pass_t>                  renderPasses;
    dynamic_array_t<render_target_t>                renderTargets;
    dynamic_array_t<shader_binary_t>                shaderBinaries;
    dynamic_array_t<texture_sampler_t>                      sampler;

    hash_map_t<graphics_pipeline_t>                 graphicPipelines;
    hash_map_t<vertex_format_t>                     vertexFormats;

    linked_list_node_t<render_pass_t>*              pFirstFreeRenderPass;
    linked_list_node_t<gpu_buffer_t>*               pFirstFreeGpuBuffer;
    linked_list_node_t<gpu_texture_t>*              pFirstFreeGpuTexture;
    linked_list_node_t<render_target_t>*            pFirstFreeRenderTarget;
    linked_list_node_t<shader_binary_t>*            pFirstFreeShaderBinary;
    linked_list_node_t<texture_sampler_t>*                  pFirstFreeSampler;

    flags8_t<render_resource_flags_t>               flags;
};

struct graphics_frame_t
{
    render_state_t                          renderState;
    memory_allocator_t                      tempMemoryAllocator;
    render_resource_cache_t*                pRenderResourceCache;
    shader_compiler_context_t*              pShaderCompilerContext;
    render_target_t*                        pBackBuffer;
    memory_allocator_t*                     pMemoryAllocator;
    render_pass_t*                          pFirstRenderPassToExecute;
    gpu_buffer_t*                           pFirstGpuBufferToFree;
    gpu_texture_t*                          pFirstGpuTextureToFree;
    vertex_format_t*                        pFirstVertexFormatToFree;
    graphics_pipeline_t*                    pFirstGraphicsPipelineToFree;
    texture_sampler_t*                      pFirstSamplerToFree;

    d3d12_descriptor_heap_t*                pShaderVisibleDescriptorHeap;
    d3d12_descriptor_heap_t*                pSamplerDescriptorHeap;

#if USE_VALIDATION
    gpu_buffer_t*                           pFirstMappedGpuBuffer;
#endif

    uint64_t                                frameIndex;
    uint32_t                                openRenderPassCount;
    D3D12DeviceType*                        pDevice;
    ID3D12Fence*                            pFrameFence;
    ID3D12GraphicsCommandList*              pFrameGeneralGraphicsCommandList;
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
    uint32_t                    maxRenderPassCount;
    d3d12_descriptor_heap_t*    pShaderDescriptorHeap;
    d3d12_descriptor_heap_t*    pSamplerDescriptorHeap;
};

struct render_context_t
{
    D3D12DeviceType*            pDevice;
    D3D12DebugType*             pDebugLayer;
    DXGIFactoryType*            pFactory;

    d3d12_descriptor_heap_t     shaderVisibleDescriptorHeap;
    d3d12_descriptor_heap_t     samplerDescriptorHeap;

    shader_compiler_context_t   shaderCompilerContext;
    render_resource_cache_t     renderResourceCache;
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
        ASSERT_DEBUG(pPointer != nullptr);
        return pPointer;
    }

    T& operator*()
    {
        ASSERT_DEBUG(pPointer != nullptr);
        return *pPointer;
    }

    T** operator&()
    {
        return &pPointer;
    }

    T* takeOwnership()
    {
        ASSERT_DEBUG(pPointer != nullptr);

        T* pRetPointer = pPointer;
        pPointer = nullptr;
        return pRetPointer;
    }

    T* pPointer;
};

enum assert_result_t
{
    assert_result_debug,
    assert_result_continue,
    assert_result_exit
};

uint3_t createUint3(uint32_t x, uint32_t y, uint32_t z)
{
    return {x, y, z};
}

bool Validate(const bool expression, const char* pMessage, ...)
{
    UNUSED_PARAMETER(pMessage);
    
#if USE_VALIDATION
    if(!expression)
    {
        printf("Validation error:");
        va_list vaList;
        va_start(vaList, pMessage);
        vprintf(pMessage, vaList);
        va_end(vaList);
    }
#endif

    return expression;
}

bool ValidateNoBreak(const bool expression, const char* pMessage, ...)
{
    UNUSED_PARAMETER(pMessage);
    if(!Validate(expression, pMessage))
    {
#if !FORCE_VALIDATION_BREAKS_OFF
        DebugBreak();
#endif
    }

    return expression;
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

constexpr memory_buffer_t   emptyMemoryBuffer = {nullptr, 0u};

template<typename T>
void clearMemoryWithZeroes(T* pMemory)
{
    ASSERT_DEBUG(pMemory);
    memset(pMemory, 0, sizeof(T));
}

void copyMemoryNonOverlapping(void* pDst, const void* pSrc, const uint64_t sizeInBytes)
{
    memcpy(pDst, pSrc, sizeInBytes);
}

void resetAllocator(memory_allocator_t* pAllocator)
{
    
}

void* allocateFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, alloc_flags_t flags = none)
{
    void* pMemory = pAllocator->allocateFnc(pAllocator, sizeInBytes, defaultAllocationAlignment);
    
#if FORCE_CLEAR_ALLOCATIONS
    const bool clearMemory = true;
#else
    const bool clearMemory = (flags & clear_memory);
#endif

    if(pMemory && clearMemory)
    {
        memset(pMemory, 0, sizeInBytes);
    }

    return pMemory;
}

void* allocateAlignedFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, uint64_t alignmentInBytes, alloc_flags_t flags = none)
{
    void* pMemory = pAllocator->allocateFnc(pAllocator, sizeInBytes, alignmentInBytes);
    
#if FORCE_CLEAR_ALLOCATIONS
    const bool clearMemory = true;
#else
    const bool clearMemory = (flags & clear_memory);
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

void destroyDynamicArray(base_dynamic_array_t* pArray)
{
    ASSERT_DEBUG(pArray != nullptr);
    ASSERT_DEBUG(pArray->pMemoryAllocator != nullptr);
    if(pArray->pData != nullptr)
    {
        freeFromAllocator(pArray->pMemoryAllocator, pArray->pData);
        pArray->pData = nullptr;
    }
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

template<typename T>
bool createDynamicArray(base_dynamic_array_t* pOutArray, memory_allocator_t* pMemoryAllocator, const uint32_t elementCapacity, alloc_flags_t allocationFlags)
{
    void* pArrayMemory = allocateFromAllocator(pMemoryAllocator, elementCapacity * sizeof(T), allocationFlags);
    if(pArrayMemory == nullptr)
    {
        return false;
    }

    pOutArray->pMemoryAllocator     = pMemoryAllocator;
    pOutArray->pData                = (T*)pArrayMemory;
    pOutArray->count                = 0u;
    pOutArray->capacity             = elementCapacity;
    pOutArray->elementSizeInBytes   = sizeof(T);
    return true;
}

template<typename T>
void destroyHashMap(hash_map_t<T>* pHashMap)
{
    ASSERT_DEBUG(pHashMap != nullptr);
    ASSERT_DEBUG(pHashMap->pMemoryAllocator != nullptr);

    if(pHashMap->pFreeNodes != nullptr)
    {
        freeFromAllocator(pHashMap->pMemoryAllocator, pHashMap->pFreeNodes);
        pHashMap->pFreeNodes = nullptr;
    }

    if(pHashMap->ppBaseNodes != nullptr)
    {
        freeFromAllocator(pHashMap->pMemoryAllocator, pHashMap->ppBaseNodes);
        pHashMap->ppBaseNodes = nullptr;
    }
}

template<typename T>
bool createHashMap(hash_map_t<T>* pOutHashMap, memory_allocator_t* pMemoryAllocator, const uint32_t nodeCapacity, alloc_flags_t allocationFlags)
{
    const uint64_t nodeSizeInBytes = sizeof(hash_map_node_t<T>);
    void* pFreeNodesMemory = allocateFromAllocator(pMemoryAllocator, nodeSizeInBytes * nodeCapacity, allocationFlags);
    if(pFreeNodesMemory == nullptr)
    {
        destroyHashMap(pOutHashMap);
        return false;
    }

    void* ppBaseNodesMemory = allocateFromAllocator(pMemoryAllocator, nodeCapacity * sizeof(void*), allocationFlags);
    if(ppBaseNodesMemory == nullptr)
    {
        destroyHashMap(pOutHashMap);
        return false;
    }

    pOutHashMap->ppBaseNodes        = (hash_map_node_t<T>**)ppBaseNodesMemory;
    pOutHashMap->pFreeNodes         = (hash_map_node_t<T>*)pFreeNodesMemory;
    pOutHashMap->pMemoryAllocator   = pMemoryAllocator;
    pOutHashMap->count              = 0u;
    pOutHashMap->capacity           = nodeCapacity;

    return true;
}

void* pushBackFromDynamicArrayDontGrow(base_dynamic_array_t* pArray, const uint32_t count)
{
    const uint32_t newArrayCount = pArray->count + count;
    if(newArrayCount > pArray->capacity)
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

template<typename T>
T* addNodesToLinkedList(linked_list_node_t<T>** ppLinkedListBase, T* pNodes, const uint32_t nodeCount)
{
    ASSERT_DEBUG(pNodes != nullptr);
    ASSERT_DEBUG(nodeCount > 0u);

    linked_list_node_t<T>* pFirstNode   = (linked_list_node_t<T>*)pNodes;
    linked_list_node_t<T>* pCurrentNode = pFirstNode;
    linked_list_node_t<T>* pNextNode    = (linked_list_node_t<T>*)((uint8_t*)pNodes + sizeof(T));
    uint32_t nodeIndex = 0u;
    while(true)
    {
        if((nodeIndex + 1) == nodeCount)
        {
            break;
        }

        pCurrentNode->pNext = (T*)pNextNode;
        pCurrentNode = pNextNode;
        pNextNode = (linked_list_node_t<T>*)((uint8_t*)pNextNode + sizeof(T));
        ++nodeIndex;
    }
    
    pCurrentNode->pNext = (T*)(*ppLinkedListBase == nullptr ? nullptr : (*ppLinkedListBase));
    *ppLinkedListBase = pFirstNode;
    return pNodes;
}

template<typename T>
linked_list_node_t<T>* createLinkedList(linked_list_node_t<T>** ppFirstNode, T* pNodes, const uint32_t nodeCount)
{
    return addNodesToLinkedList<T>(ppFirstNode, (T*)pNodes, nodeCount);
}

D3D12_BLEND_DESC createDefaultBlendDesc()
{
   D3D12_BLEND_DESC defaultBlendDesc = {};
   defaultBlendDesc.AlphaToCoverageEnable                   = FALSE;
   defaultBlendDesc.IndependentBlendEnable                  = FALSE;
   defaultBlendDesc.RenderTarget[0].BlendEnable             = FALSE;
   defaultBlendDesc.RenderTarget[0].LogicOpEnable           = FALSE;
   defaultBlendDesc.RenderTarget[0].SrcBlend                = D3D12_BLEND_ONE;
   defaultBlendDesc.RenderTarget[0].DestBlend               = D3D12_BLEND_ZERO;
   defaultBlendDesc.RenderTarget[0].BlendOp                 = D3D12_BLEND_OP_ADD;
   defaultBlendDesc.RenderTarget[0].SrcBlendAlpha           = D3D12_BLEND_ONE;
   defaultBlendDesc.RenderTarget[0].DestBlendAlpha          = D3D12_BLEND_ZERO;
   defaultBlendDesc.RenderTarget[0].BlendOpAlpha            = D3D12_BLEND_OP_ADD;
   defaultBlendDesc.RenderTarget[0].LogicOp                 = D3D12_LOGIC_OP_NOOP;
   defaultBlendDesc.RenderTarget[0].RenderTargetWriteMask   = D3D12_COLOR_WRITE_ENABLE_ALL;

   return defaultBlendDesc;
}

D3D12_DEPTH_STENCIL_DESC createDefaultDepthStencilDesc()
{
    D3D12_DEPTH_STENCIL_DESC defaultDepthStencilDesc = {};
    defaultDepthStencilDesc.DepthEnable                     = TRUE;
    defaultDepthStencilDesc.DepthWriteMask                  = D3D12_DEPTH_WRITE_MASK_ALL;
    defaultDepthStencilDesc.DepthFunc                       = D3D12_COMPARISON_FUNC_LESS;
    defaultDepthStencilDesc.StencilEnable                   = FALSE;
    defaultDepthStencilDesc.StencilReadMask                 = D3D12_DEFAULT_STENCIL_READ_MASK;
    defaultDepthStencilDesc.StencilWriteMask                = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    defaultDepthStencilDesc.FrontFace.StencilFailOp         = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilDesc.FrontFace.StencilDepthFailOp    = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilDesc.FrontFace.StencilPassOp         = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilDesc.FrontFace.StencilFunc           = D3D12_COMPARISON_FUNC_ALWAYS;
    defaultDepthStencilDesc.BackFace.StencilFailOp          = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilDesc.BackFace.StencilDepthFailOp     = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilDesc.BackFace.StencilPassOp          = D3D12_STENCIL_OP_KEEP;
    defaultDepthStencilDesc.BackFace.StencilFunc            = D3D12_COMPARISON_FUNC_ALWAYS;

    return defaultDepthStencilDesc;
}

D3D12_RASTERIZER_DESC createDefaultRasterizerDesc()
{
   D3D12_RASTERIZER_DESC defaultRasterizerDesc = {};
   defaultRasterizerDesc.FillMode               = D3D12_FILL_MODE_SOLID;
   defaultRasterizerDesc.CullMode               = D3D12_CULL_MODE_BACK;
   defaultRasterizerDesc.DepthBias              = 0;
   defaultRasterizerDesc.DepthBiasClamp         = 0.0f;
   defaultRasterizerDesc.SlopeScaledDepthBias   = 0.0f;
   defaultRasterizerDesc.DepthClipEnable        = TRUE;
   defaultRasterizerDesc.MultisampleEnable      = FALSE;
   defaultRasterizerDesc.AntialiasedLineEnable  = FALSE;
   defaultRasterizerDesc.ForcedSampleCount      = 0;
   defaultRasterizerDesc.FrontCounterClockwise  = FALSE;
   defaultRasterizerDesc.ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

   return defaultRasterizerDesc;
}

template<typename D, typename S>
D rangeCheckCast(S value)
{
    ASSERT_ALWAYS(value <= std::numeric_limits<S>::max());
    ASSERT_ALWAYS(value >= std::numeric_limits<S>::min());
    return (D)value;
}

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

void addPixBeginMarker(ID3D12GraphicsCommandList* pCommandList, const char* pName)
{
    PIXBeginEvent(pCommandList, PIX_COLOR(100, 100, 100), pName);
}

void addPixEndMarker(ID3D12GraphicsCommandList* pCommandList)
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
    wchar_t wideNameBuffer[512] = {};
    mbstowcs(wideNameBuffer, pName, sizeof(wideNameBuffer) / sizeof(wchar_t));
    pObject->SetName(wideNameBuffer);
}

HRESULT logOnHResultError(const HRESULT originalResult, const char* pFunctionCall, const char* pFile, const uint32_t lineNumber)
{
    if(originalResult != S_OK)
    {
        logError("Error during call '%s' in %s:%u\nError:%s\n", pFunctionCall, pFile, lineNumber, getHResultString(originalResult));
    }

    return originalResult;
}

void freeGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer)
{
    ASSERT_DEBUG((pGpuBuffer->flags & gpu_buffer_flag_t::marked_as_free) == 0);

    pGpuBuffer->pNext = pGraphicsFrame->pFirstGpuBufferToFree;
    pGraphicsFrame->pFirstGpuBufferToFree = pGpuBuffer;
    pGpuBuffer->flags |= gpu_buffer_flag_t::marked_as_free;
}

void freeGpuTexture(graphics_frame_t* pGraphicsFrame, gpu_texture_t* pGpuTexture)
{
    ASSERT_DEBUG((pGpuTexture->flags & gpu_texture_flag_t::marked_as_free) == 0);

    pGpuTexture->pNext = pGraphicsFrame->pFirstGpuTextureToFree;
    pGraphicsFrame->pFirstGpuTextureToFree = pGpuTexture;
    pGpuTexture->flags |= gpu_texture_flag_t::marked_as_free;
}

void initializeRenderTarget(render_target_t* pRenderTarget, uint3_t dimensions, ID3D12Resource* pRenderTargetResource, D3D12_CPU_DESCRIPTOR_HANDLE* pColorBufferHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pDepthBufferHandle, D3D12_RESOURCE_STATES state)
{
    ASSERT_DEBUG(pRenderTarget != nullptr);
    ASSERT_DEBUG(pRenderTargetResource != nullptr);
    pRenderTarget->dimensions               = dimensions;
    pRenderTarget->resource.pResource       = pRenderTargetResource;
    pRenderTarget->resource.currentState    = state;
    pRenderTarget->colorBufferHandle        = pColorBufferHandle == nullptr ? cpuDescriptorNullptrHandle : *pColorBufferHandle;
    pRenderTarget->depthBufferHandle        = pDepthBufferHandle == nullptr ? cpuDescriptorNullptrHandle : *pDepthBufferHandle;
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

D3D12_GPU_DESCRIPTOR_HANDLE getNextGPUDescriptorHandle(d3d12_descriptor_heap_t* pDescriptorHeap)
{
    ASSERT_DEBUG(pDescriptorHeap->pGPUCurrent != nullptr);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    gpuHandle.ptr = (size_t)pDescriptorHeap->pGPUCurrent;
    pDescriptorHeap->pGPUCurrent += pDescriptorHeap->incrementSizeInBytes;

    return gpuHandle;
}

void markRenderPassChainAsFree(render_resource_cache_t* pRenderResourceCache, render_pass_t* pFirstRenderPassInChain)
{
    render_pass_t* pCurrentRenderPass = pFirstRenderPassInChain;
    while(pCurrentRenderPass != nullptr)
    {
        render_pass_t* pNextRenderPass = (render_pass_t*)pCurrentRenderPass->pNext;
        pCurrentRenderPass->pNext = (render_pass_t*)pRenderResourceCache->pFirstFreeRenderPass;
        pRenderResourceCache->pFirstFreeRenderPass = pCurrentRenderPass;

        pCurrentRenderPass = pNextRenderPass;
    }
}

void freePendingFrameResources(graphics_frame_t* pGraphicsFrame)
{
    if(pGraphicsFrame->pFirstGpuBufferToFree != nullptr)
    {
        gpu_buffer_t* pGpuBufferToFree = pGraphicsFrame->pFirstGpuBufferToFree;
        int gpuBufferCount = 0u;
        while(pGpuBufferToFree != nullptr)
        {
            gpu_buffer_t* pNextGpuBuffer = (gpu_buffer_t*)pGpuBufferToFree->pNext;
            COM_RELEASE(pGpuBufferToFree->resource.pResource);
            ZeroMemory(pGpuBufferToFree, sizeof(gpu_buffer_t));

            pGpuBufferToFree = pNextGpuBuffer;

            ++gpuBufferCount;
        }

        addNodesToLinkedList(&pGraphicsFrame->pRenderResourceCache->pFirstFreeGpuBuffer, pGraphicsFrame->pFirstGpuBufferToFree, gpuBufferCount);
        pGraphicsFrame->pFirstGpuBufferToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstGpuTextureToFree != nullptr)
    {
        gpu_texture_t* pGpuTextureToFree = pGraphicsFrame->pFirstGpuTextureToFree;
        int gpuTextureCount = 0u;
        while(pGpuTextureToFree != nullptr)
        {
            gpu_texture_t* pNextGpuTexture = (gpu_texture_t*)pGpuTextureToFree->pNext;
            COM_RELEASE(pGpuTextureToFree->resource.pResource);
            ZeroMemory(pGpuTextureToFree, sizeof(gpu_texture_t));

            pGpuTextureToFree = pNextGpuTexture;

            ++gpuTextureCount;
        }

        addNodesToLinkedList(&pGraphicsFrame->pRenderResourceCache->pFirstFreeGpuTexture, pGraphicsFrame->pFirstGpuTextureToFree, gpuTextureCount);
        pGraphicsFrame->pFirstGpuTextureToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstGraphicsPipelineToFree != nullptr)
    {
        graphics_pipeline_t* pGraphicsPipelineStateToFree = pGraphicsFrame->pFirstGraphicsPipelineToFree;
        while(pGraphicsPipelineStateToFree != nullptr)
        {
            graphics_pipeline_t* pNextGraphicsPipelineStateToFree = (graphics_pipeline_t*)pGraphicsPipelineStateToFree->pNext;
            COM_RELEASE(pGraphicsPipelineStateToFree->pPipelineState);
            COM_RELEASE(pGraphicsPipelineStateToFree->pRootSignature);
            ZeroMemory(pGraphicsPipelineStateToFree, sizeof(graphics_pipeline_t));

            pGraphicsPipelineStateToFree = pNextGraphicsPipelineStateToFree;

            //TODO
            //FK: Remove graphics pipeline entry from hash map
        }

        pGraphicsFrame->pFirstGraphicsPipelineToFree = nullptr;
    }
}

void flushFrame(graphics_frame_t* pGraphicsFrame)
{
    const DWORD waitResult = WaitForSingleObject(pGraphicsFrame->pFrameFinishedEvent, INFINITE);
    ASSERT_DEBUG(waitResult == WAIT_OBJECT_0);
}

void resetRenderState(render_state_t* pRenderState)
{
    memset(pRenderState, 0u, sizeof(render_state_t));
}

void resetFrame(graphics_frame_t* pGraphicsFrame)
{
    Validate(pGraphicsFrame->pFirstGpuBufferToFree == nullptr, "There are pending free gpu buffer, don't forget to call 'freePendingFrameResources()'.");
    Validate(pGraphicsFrame->pFirstGraphicsPipelineToFree == nullptr, "There are pending free graphics pipelines, don't forget to call 'freePendingFrameResources()'.");

    COM_CALL(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator->Reset());
    COM_CALL(pGraphicsFrame->pFrameGeneralGraphicsCommandList->Reset(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator, nullptr));

    resetRenderState(&pGraphicsFrame->renderState);
    markRenderPassChainAsFree(pGraphicsFrame->pRenderResourceCache, pGraphicsFrame->pFirstRenderPassToExecute);
    pGraphicsFrame->pFirstRenderPassToExecute = nullptr;

    ResetEvent(pGraphicsFrame->pFrameFinishedEvent);
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

bool createDescriptorHeap(d3d12_descriptor_heap_t* pOutDescriptorHeap, D3D12DeviceType* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, const uint32_t descriptorCount )
{
    ID3D12DescriptorHeap* pDescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.Type             = type;
    descriptorHeapDesc.NumDescriptors   = descriptorCount;
    descriptorHeapDesc.Flags            = flags;
    if(COM_CALL(pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&pDescriptorHeap))) != S_OK)
    {
        return false;
    }

    const uint64_t incrementSizeInBytes = pDevice->GetDescriptorHandleIncrementSize(type);
    const uint64_t cpuDescriptorHeapStartAddress = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;

    pOutDescriptorHeap->incrementSizeInBytes    = incrementSizeInBytes;
    pOutDescriptorHeap->pCPUBaseAddress         = (const uint8_t*)cpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pCPUCurrent             = (uint8_t*)cpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pCPUEndAddress          = (const uint8_t*)cpuDescriptorHeapStartAddress + incrementSizeInBytes * descriptorCount;
    pOutDescriptorHeap->pDescriptorHeap         = pDescriptorHeap;
    pOutDescriptorHeap->pGPUBaseAddress         = nullptr;
    pOutDescriptorHeap->pGPUCurrent             = nullptr;
    pOutDescriptorHeap->pGPUEndAddress          = nullptr;

    if(flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        const uint64_t gpuDescriptorHeapStartAddress = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        pOutDescriptorHeap->pGPUBaseAddress = (const uint8_t*)gpuDescriptorHeapStartAddress;
        pOutDescriptorHeap->pGPUCurrent     = (uint8_t*)gpuDescriptorHeapStartAddress;
        pOutDescriptorHeap->pGPUEndAddress  = (const uint8_t*)gpuDescriptorHeapStartAddress + incrementSizeInBytes * descriptorCount;
    }
    return true;
}

void destroySwapChain(d3d12_swap_chain_t* pSwapChain)
{
    COM_RELEASE(pSwapChain->pSwapChain);
    for(uint32_t bufferIndex = 0u; bufferIndex < pSwapChain->backBufferCount; ++bufferIndex)
    {
        COM_RELEASE(pSwapChain->pBackBufferRenderTargets[bufferIndex].resource.pResource);
    }

    freeFromAllocator(pSwapChain->pMemoryAllocator, pSwapChain->pBackBufferRenderTargets);
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
    
    if(!createDescriptorHeap(&swapChain.backBufferRenderTargetDescriptorHeap, pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, frameBufferCount))
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

    swapChain.pBackBufferRenderTargets = (render_target_t*)allocateFromAllocator(pMemoryAllocator, sizeof(render_target_t) * frameBufferCount);
    if(swapChain.pBackBufferRenderTargets == nullptr)
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
        D3D12_CPU_DESCRIPTOR_HANDLE frameBufferHandle = getNextCPUDescriptorHandle(&swapChain.backBufferRenderTargetDescriptorHeap);
        pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, frameBufferHandle);

        initializeRenderTarget(&swapChain.pBackBufferRenderTargets[bufferIndex], createUint3(windowWidth, windowHeight, 1u), pFrameBuffer, &frameBufferHandle, nullptr, D3D12_RESOURCE_STATE_PRESENT);
    }
    
    swapChain.width = windowWidth;
    swapChain.height = windowHeight;

    *pOutSwapChain = swapChain;
    return true;

    cleanup_and_exit_failure:
        destroySwapChain(&swapChain);
        return false;
}

int mapFormatTypeOffset(const gpu_texture_format_type_t formatType, const flags8_t<gpu_texture_format_type_flag_t> supportedTypeFlags)
{
    gpu_texture_format_type_flag_t formatTypeFlag = (gpu_texture_format_type_flag_t)(1u << ((uint8_t)formatType));
    ASSERT_DEBUG(!supportedTypeFlags.isFlagSet(formatTypeFlag));

    const uint64_t formatTypeMask = (uint64_t)formatTypeFlag - 1ull;
    return rangeCheckCast<int>(PopulationCount64(formatTypeMask) - 1);
}

DXGI_FORMAT mapFormatRange(DXGI_FORMAT baseFormat, const gpu_texture_format_type_t formatType, const flags8_t<gpu_texture_format_type_flag_t> supportedFormatTypes)
{
    return (DXGI_FORMAT)(baseFormat + mapFormatTypeOffset(formatType, supportedFormatTypes));
}

DXGI_FORMAT mapTextureFormatToD3D12Format(gpu_texture_format_t format, gpu_texture_format_type_t formatType)
{
    switch(format)
    {
        case gpu_texture_format_t::R8:
            return mapFormatRange(DXGI_FORMAT_R8_TYPELESS, formatType, gpu_texture_format_type_flag_t::all_except_floating_point);
        case gpu_texture_format_t::R8G8B8A8:
            return mapFormatRange(DXGI_FORMAT_R8G8B8A8_TYPELESS, formatType, gpu_texture_format_type_flag_t::all_except_floating_point);
        case gpu_texture_format_t::R16:
            return mapFormatRange(DXGI_FORMAT_R16_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::floating_point);
        case gpu_texture_format_t::R16G16:
            return mapFormatRange(DXGI_FORMAT_R16G16_TYPELESS, formatType, gpu_texture_format_type_flag_t::all);
        case gpu_texture_format_t::R16G16B16A16:
            return mapFormatRange(DXGI_FORMAT_R16G16B16A16_TYPELESS, formatType, gpu_texture_format_type_flag_t::all);
        case gpu_texture_format_t::R32:
        {
            const DXGI_FORMAT r32Format = (DXGI_FORMAT)(mapFormatRange(DXGI_FORMAT_R32_TYPELESS, formatType, gpu_texture_format_type_flag_t::all) + 1);
            return r32Format == DXGI_FORMAT_D32_FLOAT ? DXGI_FORMAT_R32_FLOAT : r32Format;
        }
        case gpu_texture_format_t::R32G32:
            return mapFormatRange(DXGI_FORMAT_R32G32_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::floating_point | gpu_texture_format_type_flag_t::unsigned_int | gpu_texture_format_type_flag_t::signed_int);
        case gpu_texture_format_t::R32G32B32:
            return mapFormatRange(DXGI_FORMAT_R32G32B32_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::floating_point | gpu_texture_format_type_flag_t::unsigned_int | gpu_texture_format_type_flag_t::signed_int);
        case gpu_texture_format_t::R32G32B32A32:
            return mapFormatRange(DXGI_FORMAT_R32G32B32A32_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::floating_point | gpu_texture_format_type_flag_t::unsigned_int | gpu_texture_format_type_flag_t::signed_int);
        case gpu_texture_format_t::R10G10B10A2:
            return mapFormatRange(DXGI_FORMAT_R10G10B10A2_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::unsigned_int | gpu_texture_format_type_flag_t::normalized_unsigned_int);
        case gpu_texture_format_t::D24S8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case gpu_texture_format_t::D32:
            return mapFormatRange(DXGI_FORMAT_D32_FLOAT, formatType, gpu_texture_format_type_flag_t::floating_point);
        case gpu_texture_format_t::BC1:
            return mapFormatRange(DXGI_FORMAT_BC1_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::normalized_unsigned_int);
        case gpu_texture_format_t::BC2:
            return mapFormatRange(DXGI_FORMAT_BC2_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::normalized_unsigned_int);
        case gpu_texture_format_t::BC3:
            return mapFormatRange(DXGI_FORMAT_BC3_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::normalized_unsigned_int);
        case gpu_texture_format_t::BC4:
            return mapFormatRange(DXGI_FORMAT_BC4_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::normalized_unsigned_int);
        case gpu_texture_format_t::BC5:
            return mapFormatRange(DXGI_FORMAT_BC5_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::normalized_unsigned_int);
        case gpu_texture_format_t::BC7:
            return mapFormatRange(DXGI_FORMAT_BC7_TYPELESS, formatType, gpu_texture_format_type_flag_t::typeless | gpu_texture_format_type_flag_t::normalized_unsigned_int);
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return DXGI_FORMAT_UNKNOWN;
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

hash32_t generateHash(const void* pData, const uint64_t dataSizeInBytes)
{
    const char* pDataBuffer = (const char*)pData;
    hash32_t hash = 5381;
    for(uint64_t i = 0; i < dataSizeInBytes; ++i)
    {
        int c = *pDataBuffer++;
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

template<typename T>
hash_map_node_t<T>* getFreeHashMapNode(hash_map_t<T>* pHashMap)
{
    if(pHashMap->count + 1 == pHashMap->capacity)
    {
        return nullptr;
    }

    return pHashMap->pFreeNodes + pHashMap->count;
}

template<typename T>
uint32_t calculateHashmapEntryIndex(const void* pNode, const hash_map_t<T>* pHashMap)
{
    const ptrdiff_t nodeDistanceFromBase = (ptrdiff_t)pNode - (ptrdiff_t)pHashMap->ppBaseNodes[0];
    const ptrdiff_t nodeIndex = (nodeDistanceFromBase >> 3);
    return rangeCheckCast<uint32_t>(nodeIndex);
}

template<typename T>
hash_map_entry_t<T*> findOrInsertEntryIntoHashMap(hash_map_t<T>* pHashMap, const void* pData, const uint64_t dataSizeInBytes)
{
    const hash32_t hash = generateHash(pData, dataSizeInBytes);
    const uint32_t index = hash % pHashMap->capacity;
    bool foundNode = false;
    hash_map_node_t<T>** ppNode = &pHashMap->ppBaseNodes[index];
    hash_map_node_t<T>* pNode = *ppNode;
    hash_map_node_t<T>* pPrevNode = pHashMap->ppBaseNodes[index];

    while(true)
    {
        if(pNode == nullptr)
        {
            break;
        }

        foundNode = (pNode->hash == hash);
        if(foundNode)
        {
            break;
        }
        
        pPrevNode = pNode;
        ppNode = (hash_map_node_t<T>**)&pNode->pNext;
        pNode = *ppNode;
    }

    if(foundNode)
    {
        hash_map_entry_t<T*> entry;
        entry.isNew     = false;
        entry.value     = &(*ppNode)->value;
        entry.nodeIndex = calculateHashmapEntryIndex(pNode, pHashMap);
        entry.hash      = hash;
        return entry;
    }
    
    hash_map_node_t<T>* pNewNode = getFreeHashMapNode(pHashMap);
    if(pNewNode == nullptr)
    {
        //FK: Note: hashmap doesn't grow yet.
        ASSERT_DEBUG_UNREACHABLE_CODE();
    }

    ++pHashMap->count;
    pNewNode->hash = hash;
    *ppNode = pNewNode;

    hash_map_entry_t<T*> entry;
    entry.isNew     = true;
    entry.value     = &(*ppNode)->value;
    entry.nodeIndex = calculateHashmapEntryIndex(pNode, pHashMap);
    entry.hash      = hash;
    return entry;
}

template<typename T>
void removeEntryFromHashMap(hash_map_t<T>* pHashMap, const hash_map_entry_t<T*>* pEntry)
{
    const uint32_t nodeIndex = pEntry->nodeIndex;
    ASSERT_ALWAYS(pHashMap->ppBaseNodes[nodeIndex] != nullptr);
    ASSERT_ALWAYS(pHashMap->count > 0u);

    hash_map_node_t<T>* pPrevNode = nullptr;
    hash_map_node_t<T>* pNode = pHashMap->ppBaseNodes[nodeIndex];
    while(pNode->hash != pEntry->hash)
    {
        pPrevNode = pNode;
        pNode = (hash_map_node_t<T>*)pNode->pNext;
    }

    hash_map_node_t<T>* pNextNode = (hash_map_node_t<T>*)pNode->pNext;
    if(pPrevNode == nullptr)
    {
        pHashMap->ppBaseNodes[nodeIndex] = pNextNode;
    }
    else
    {
        pPrevNode->pNext = pNextNode;
    }

    const uint32_t freeNodesIndex = pHashMap->capacity - pHashMap->count;
    pHashMap->pFreeNodes[freeNodesIndex] = *pNode;
    pHashMap->count -= 1u;
}

const char* getVertexAttributeSemanticBaseName(const vertex_attribute_t attribute)
{
    static_assert((uint32_t)vertex_attribute_t::count == 4u);

    switch(attribute)
    {
        case vertex_attribute_t::color:
            return "COLOR";
        case vertex_attribute_t::position:
            return "POSITION";
        case vertex_attribute_t::normal:
            return "NORMAL";
        case vertex_attribute_t::texcoord:
            return "TEXCOORD";
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return nullptr;
}

DXGI_FORMAT convertVertexAttributeFormat(const vertex_attribute_entry_t* pVertexAttribute)
{
    static_assert((uint32_t)vertex_attribute_type_t::count == 1u);

    switch(pVertexAttribute->type)
    {
        case vertex_attribute_type_t::float32:
            if(pVertexAttribute->count == 1u) return DXGI_FORMAT_R32_FLOAT;
            if(pVertexAttribute->count == 2u) return DXGI_FORMAT_R32G32_FLOAT;
            if(pVertexAttribute->count == 3u) return DXGI_FORMAT_R32G32B32_FLOAT;
            if(pVertexAttribute->count == 4u) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return DXGI_FORMAT_UNKNOWN;
}

D3D12_INPUT_CLASSIFICATION convertVertexAttributeInputFrequency(const vertex_attribute_frequency_t frequency)
{
    static_assert((uint32_t)vertex_attribute_frequency_t::count == 2u);

    switch(frequency)
    {
        case vertex_attribute_frequency_t::vertex:
            return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        case vertex_attribute_frequency_t::instance:
            return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}

bool isValidGraphicsPipelineParameters(const graphics_pipeline_parameters_t* pPipelineParameters)
{
    if(pPipelineParameters == nullptr)
    {
        return false;
    }

    if(pPipelineParameters->pPixelShader == nullptr)
    {
        return false;
    }

    if(pPipelineParameters->pVertexFormat == nullptr)
    {
        return false;
    }

    if(pPipelineParameters->pVertexShader == nullptr)
    {
        return false;
    }

    if(pPipelineParameters->topology < topology_t::point_list || pPipelineParameters->topology > topology_t::triangle_strip)
    {
        return false;
    }

    return true;
}

D3D12_ROOT_PARAMETER_TYPE mapBindingPointTypeToRootParameterType(const shader_binding_point_t* pShaderBindingPoint)
{
    switch(pShaderBindingPoint->type)
    {
        case bound_resource_type_t::constant_buffer:
            return D3D12_ROOT_PARAMETER_TYPE_CBV;
        case bound_resource_type_t::texture:
        case bound_resource_type_t::sampler:
            return D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    }

    DebugBreak();
    return D3D12_ROOT_PARAMETER_TYPE_CBV;
}

D3D12_DESCRIPTOR_RANGE_TYPE mapBindingPointTypeToDescriptorRangeType(const bound_resource_type_t bindingPointType)
{
    switch(bindingPointType)
    {
        case bound_resource_type_t::constant_buffer:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case bound_resource_type_t::sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case bound_resource_type_t::texture:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
}

bool tryToCreateRootParameter(D3D12_ROOT_PARAMETER* pOutRootParameter, const shader_binding_point_t* pBindingPoint, memory_allocator_t* pMemoryAllocator, D3D12_SHADER_VISIBILITY shaderVisibility)
{
    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = mapBindingPointTypeToRootParameterType(pBindingPoint);
    rootParameter.ShaderVisibility = shaderVisibility;

    if(rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
    {
        D3D12_DESCRIPTOR_RANGE* pDescriptorRange = (D3D12_DESCRIPTOR_RANGE*)allocateFromAllocator(pMemoryAllocator, sizeof(D3D12_DESCRIPTOR_RANGE), alloc_flags_t::clear_memory);
        if(pDescriptorRange == nullptr)
        {
            return false;
        }

        pDescriptorRange->BaseShaderRegister    = pBindingPoint->slot;
        pDescriptorRange->RegisterSpace         = pBindingPoint->space;
        pDescriptorRange->RangeType             = mapBindingPointTypeToDescriptorRangeType(pBindingPoint->type);
        pDescriptorRange->NumDescriptors        = 1;

        rootParameter.DescriptorTable.NumDescriptorRanges   = 1;
        rootParameter.DescriptorTable.pDescriptorRanges     = pDescriptorRange;
    }
    else if(rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
            rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
            rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
    {
        rootParameter.Descriptor.RegisterSpace  = pBindingPoint->space;
        rootParameter.Descriptor.ShaderRegister = pBindingPoint->slot;
    }
    else
    {
        ASSERT_DEBUG_UNREACHABLE_CODE();
    }

    *pOutRootParameter = rootParameter;
    return true;
}

bool fillRootSignature(memory_allocator_t* pMemoryAllocator, D3D12_ROOT_SIGNATURE_DESC* pOutRootSignature, const shader_binding_point_t* pBindingPoints, const uint32_t bindingPointCount)
{
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    dynamic_array_t<D3D12_ROOT_PARAMETER> rootParameters;
    if(!createDynamicArray<D3D12_ROOT_PARAMETER>(&rootParameters, pMemoryAllocator, bindingPointCount, alloc_flags_t::clear_memory))
    {
        return false;
    }

    D3D12_ROOT_PARAMETER* pRootParameters = (D3D12_ROOT_PARAMETER*)pushBackFromDynamicArrayDontGrow(&rootParameters, bindingPointCount);
    for(uint32_t bindingPointIndex = 0u; bindingPointIndex < bindingPointCount; ++bindingPointIndex )
    {
        if(!tryToCreateRootParameter(&pRootParameters[bindingPointIndex], &pBindingPoints[bindingPointIndex], pMemoryAllocator, D3D12_SHADER_VISIBILITY_ALL))
        {
            return false;
        }
    }

    rootSignatureDesc.NumParameters = rootParameters.count;
    rootSignatureDesc.pParameters = (D3D12_ROOT_PARAMETER*)rootParameters.pData;

    *pOutRootSignature = rootSignatureDesc;
    return true;
}

D3D12_PRIMITIVE_TOPOLOGY mapTopologyToD3D12Topology(const topology_t topology)
{
    switch(topology)
    {
        case topology_t::point_list:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case topology_t::line_list:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case topology_t::line_strip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case topology_t::triangle_list:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case topology_t::triangle_strip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE mapTopologyToD3D12TopologyType(const topology_t topology)
{
    switch(topology)
    {
        case topology_t::point_list:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case topology_t::line_list:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case topology_t::line_strip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case topology_t::triangle_list:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case topology_t::triangle_strip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

uint32_t calculateBindingPointCount(const graphics_pipeline_parameters_t* pPipelineParameters)
{
    uint32_t bindingPointCount = 0u;
    if(pPipelineParameters->pPixelShader != nullptr)
    {
        bindingPointCount += pPipelineParameters->pPixelShader->bindingPointCount;
    }
    
    if(pPipelineParameters->pVertexShader != nullptr)
    {
        bindingPointCount += pPipelineParameters->pVertexShader->bindingPointCount;
    }

    return bindingPointCount;
}

void collectGraphicPipelineBindingPointsFromShader(shader_binding_point_t* pBindingPointsToFill, uint32_t* pBindingPointsIndex, const shader_binary_t* pShaderBinary)
{
    if(pShaderBinary->bindingPointCount == 0u)
    {
        return;
    }

    uint32_t bindingPointsIndex = *pBindingPointsIndex;
    *pBindingPointsIndex += pShaderBinary->bindingPointCount;

    memcpy(pBindingPointsToFill + bindingPointsIndex, pShaderBinary->bindingPoints, sizeof(shader_binding_point_t) * pShaderBinary->bindingPointCount);
}

void collectGraphicPipelineBindingPoints(shader_binding_point_t* pBindingPointsToFill, const graphics_pipeline_parameters_t* pPipelineParameters)
{
    uint32_t bindingPointIndex = 0u;
    if(pPipelineParameters->pVertexShader != nullptr)
    {
        collectGraphicPipelineBindingPointsFromShader(pBindingPointsToFill, &bindingPointIndex, pPipelineParameters->pVertexShader);
    }

    if(pPipelineParameters->pPixelShader != nullptr)
    {
        collectGraphicPipelineBindingPointsFromShader(pBindingPointsToFill, &bindingPointIndex, pPipelineParameters->pPixelShader);
    }
}

NO_DISCARD graphics_pipeline_t* createGraphicsPipeline(graphics_frame_t* pGraphicsFrame, const graphics_pipeline_parameters_t* pPipelineParameters)
{
    Validate(isValidGraphicsPipelineParameters(pPipelineParameters), "Graphics pipeline parameters are invalid.");

    hash_map_entry_t<graphics_pipeline_t*> pipelineState = findOrInsertEntryIntoHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, pPipelineParameters, sizeof(pPipelineParameters));
    if(!pipelineState.isNew)
    {
        return pipelineState.value;
    }
    
    graphics_pipeline_t* pPipelineState = pipelineState.value;

    const uint32_t bindingPointCount = calculateBindingPointCount(pPipelineParameters);
    shader_binding_point_t* pShaderBindingPoints = (shader_binding_point_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, sizeof(shader_binding_point_t) * bindingPointCount, alloc_flags_t::clear_memory);
    if(pShaderBindingPoints == nullptr && bindingPointCount > 0u)
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineState);
        return nullptr;
    }

    collectGraphicPipelineBindingPoints(pShaderBindingPoints, pPipelineParameters);

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    if(!fillRootSignature(&pGraphicsFrame->tempMemoryAllocator, &rootSignatureDesc, pShaderBindingPoints, bindingPointCount))
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineState);
        return nullptr;
    }

    ComPtr<ID3DBlob> pRootSignatureBlob = nullptr;
    ComPtr<ID3DBlob> pErrorBlob = nullptr;
    if(COM_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pRootSignatureBlob, &pErrorBlob)) != S_OK)
    {
        if(pErrorBlob != nullptr)
        {
            const char* pError = (const char*)pErrorBlob->GetBufferPointer();
            logError(pError);
        }

        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineState);
        return nullptr;
    }

    ID3D12RootSignature* pRootSignature = nullptr;
    COM_CALL(pGraphicsFrame->pDevice->CreateRootSignature(0u, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc = {};
    graphicsPipelineStateDesc.VS.BytecodeLength     = pPipelineParameters->pVertexShader->shaderBlobSizeInBytes;
    graphicsPipelineStateDesc.VS.pShaderBytecode    = pPipelineParameters->pVertexShader->pShaderBlob;
    graphicsPipelineStateDesc.PS.BytecodeLength     = pPipelineParameters->pPixelShader->shaderBlobSizeInBytes;
    graphicsPipelineStateDesc.PS.pShaderBytecode    = pPipelineParameters->pPixelShader->pShaderBlob;
    graphicsPipelineStateDesc.NumRenderTargets      = 1u;
    graphicsPipelineStateDesc.SampleMask            = 0xFFFFFFFF;
    graphicsPipelineStateDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    graphicsPipelineStateDesc.PrimitiveTopologyType = mapTopologyToD3D12TopologyType(pPipelineParameters->topology);
    graphicsPipelineStateDesc.SampleDesc.Count      = 1u;
    graphicsPipelineStateDesc.SampleDesc.Quality    = 0u;
    graphicsPipelineStateDesc.BlendState            = createDefaultBlendDesc();
    graphicsPipelineStateDesc.DepthStencilState     = createDefaultDepthStencilDesc();
    graphicsPipelineStateDesc.RasterizerState       = createDefaultRasterizerDesc();
    graphicsPipelineStateDesc.pRootSignature        = pRootSignature;
    
    graphicsPipelineStateDesc.InputLayout.NumElements = pPipelineParameters->pVertexFormat->inputElementCount;
    graphicsPipelineStateDesc.InputLayout.pInputElementDescs = pPipelineParameters->pVertexFormat->pInputElementDescs;

    ID3D12PipelineState* pPipelineStateObject = nullptr;
    const HRESULT pipelineStateObjectResult = COM_CALL(pGraphicsFrame->pDevice->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pPipelineStateObject)));
    if(pipelineStateObjectResult != S_OK)
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineState);
        logError("'%s' while trying to create graphics pipeline state '%s'.", getHResultString(pipelineStateObjectResult), pPipelineParameters->pName);
        return nullptr;
    }

    setD3D12ObjectDebugName(pPipelineStateObject, pPipelineParameters->pName);
    
    pPipelineState->pPipelineState          = pPipelineStateObject;
    pPipelineState->pRootSignature          = pRootSignature;
    pPipelineState->topology                = pPipelineParameters->topology;
    pPipelineState->pShaderBindingPoints    = pShaderBindingPoints;
    pPipelineState->shaderBindingPointCount = bindingPointCount;
    return pPipelineState;
}

void destroyGraphicsPipelineState(graphics_pipeline_t* pGraphicsPipelineState)
{
    
}

void destroyGraphicsFrame(graphics_frame_t* pGraphicsFrame)
{
    flushFrame(pGraphicsFrame);
    freePendingFrameResources(pGraphicsFrame);
    if(pGraphicsFrame->pFrameFinishedEvent != nullptr)
    {
        CloseHandle(pGraphicsFrame->pFrameFinishedEvent);
    }

    COM_RELEASE(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator);
    COM_RELEASE(pGraphicsFrame->pFrameGeneralGraphicsCommandList);
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

bool createGraphicsFrame(graphics_frame_t* pOutGraphicFrame, const graphics_frame_parameters_t* pGraphicsFrameParameters, memory_allocator_t* pMemoryAllocator, render_resource_cache_t* pRenderResourceCache, shader_compiler_context_t* pShaderCompilerContext, D3D12DeviceType* pDevice)
{
    ASSERT_DEBUG(pGraphicsFrameParameters != nullptr);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    ASSERT_DEBUG(pDevice != nullptr);

    graphics_frame_t graphicsFrame = {0};
    graphicsFrame.pMemoryAllocator = pMemoryAllocator;
    graphicsFrame.pFrameFinishedEvent = CreateEvent(nullptr, TRUE, TRUE, "");
    graphicsFrame.pShaderCompilerContext = pShaderCompilerContext;
    graphicsFrame.pRenderResourceCache = pRenderResourceCache;
    graphicsFrame.pShaderVisibleDescriptorHeap = pGraphicsFrameParameters->pShaderDescriptorHeap;
    graphicsFrame.pSamplerDescriptorHeap = pGraphicsFrameParameters->pSamplerDescriptorHeap;

    if(graphicsFrame.pFrameFinishedEvent == nullptr)
    {
        return false;
    }

    createDefaultMemoryAllocator(&graphicsFrame.tempMemoryAllocator);

    if(!createCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &graphicsFrame.pFrameGeneralGraphicsCommandAllocator))
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, graphicsFrame.pFrameGeneralGraphicsCommandAllocator, &graphicsFrame.pFrameGeneralGraphicsCommandList))
    {
        goto cleanup_and_exit_failure;
    }

    graphicsFrame.pDevice = pDevice;

    if(!createFence(pDevice, &graphicsFrame.pFrameFence, 0))
    {
        goto cleanup_and_exit_failure;
    }

    setD3D12ObjectDebugName(graphicsFrame.pFrameGeneralGraphicsCommandAllocator, "GraphicsFrame GraphicsCommandAllocator");
    setD3D12ObjectDebugName(graphicsFrame.pFrameGeneralGraphicsCommandList, "GraphicsFrame GraphicsCommandList");

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
        uint32_t                        maxGpuTextureCount;
        uint32_t                        maxGpuBufferCount;
        uint32_t                        maxVertexFormatCount;
        uint32_t                        maxShaderBinaryCount;
        uint32_t                        maxRenderPassCount;
        uint32_t                        maxRenderTargetCount;
        uint32_t                        maxPipelineStateCount;
        uint32_t                        maxSamplerCount;
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

    if(pParameters->limits.maxGpuBufferCount == 0)
    {
        return false;
    }

    if(pParameters->limits.maxGpuTextureCount == 0)
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

    if(COM_CALL(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pShaderCompilerContext->pUtils))) != S_OK)
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

bool initRenderPasses(D3D12DeviceType* pDevice, render_pass_t* pFirstRenderPassInChain)
{   
    uint32_t renderPassIndex = 0u;
    char renderPassDebugNameBuffer[] = "render_pass_graphics_command_allocator_______";
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

        if(!createFence(pDevice, &pCurrentRenderPass->pRenderPassFence, 0u))
        {
            return false;
        }

        sprintf(renderPassDebugNameBuffer, "render_pass_graphics_command_allocator_%u", renderPassIndex);
        setD3D12ObjectDebugName(pCurrentRenderPass->pGraphicsCommandAllocator, renderPassDebugNameBuffer);    

        sprintf(renderPassDebugNameBuffer, "render_pass_graphics_command_list_%u", renderPassIndex);
        setD3D12ObjectDebugName(pCurrentRenderPass->pGraphicsCommandList, renderPassDebugNameBuffer);    

        pCurrentRenderPass = (render_pass_t*)pCurrentRenderPass->pNext;
        ++renderPassIndex;
    }

    return true;
}

bool areAllGpuBuffersReleased(dynamic_array_t<gpu_buffer_t>* pGpuBuffers)
{
    gpu_buffer_t* pBaseGpuBuffer = (gpu_buffer_t*)pGpuBuffers->pData;
    bool foundLeakedResources = false;
    for(uint32_t bufferIndex = 0u; bufferIndex < pGpuBuffers->capacity; ++bufferIndex)
    {
        if(pBaseGpuBuffer[bufferIndex].resource.pResource != nullptr)
        {
            logWarning("GPU buffer '%s' leaked.", pBaseGpuBuffer[bufferIndex].pName);
            foundLeakedResources = true;
        }
    }

    return !foundLeakedResources;
}

void destroyRenderResourceCache(render_resource_cache_t* pRenderResourceCache)
{
    Validate(areAllGpuBuffersReleased(&pRenderResourceCache->gpuBuffers), "Leaked GPU buffers detected.");
    //Validate((areAllGraphicPipelinesReleased(&pRenderResourceCache->graphicPipelines), "Leaked graphics pipeline states.");
    //TODO: More validation

    //FK: Render pass resources where created during createRenderResourceCache
    render_pass_t* pRenderPasses = (render_pass_t*)pRenderResourceCache->renderPasses.pData;
    for(uint32_t i = 0u; i < pRenderResourceCache->renderPasses.capacity; ++i)
    {
        render_pass_t* pRenderPass = &pRenderPasses[i];
        COM_RELEASE(pRenderPass->pGraphicsCommandAllocator);
        COM_RELEASE(pRenderPass->pGraphicsCommandList);
        COM_RELEASE(pRenderPass->pRenderPassFence);
    }

    destroyHashMap(&pRenderResourceCache->vertexFormats);
    destroyHashMap(&pRenderResourceCache->graphicPipelines);

    destroyDynamicArray(&pRenderResourceCache->gpuBuffers);
    destroyDynamicArray(&pRenderResourceCache->shaderBinaries);
    destroyDynamicArray(&pRenderResourceCache->renderTargets);
    destroyDynamicArray(&pRenderResourceCache->renderPasses);
}

bool createRenderResourceCache(D3D12DeviceType* pDevice, render_resource_cache_t* pOutRenderResourceCache, memory_allocator_t* pMemoryAllocator, const render_context_parameters_t::limits_t* pLimits, const bool notifyOnLimitReach)
{
    pOutRenderResourceCache->pMemoryAllocator = pMemoryAllocator;
    pOutRenderResourceCache->flags = 0u;

    if(notifyOnLimitReach)
    {
        pOutRenderResourceCache->flags |= render_resource_flags_t::notify_on_array_grow;
    }

    uint64_t offsetInBytes = 0u;
    bool renderResourceCacheAllocationFailed = false;
    renderResourceCacheAllocationFailed |= !createHashMap<vertex_format_t>(&pOutRenderResourceCache->vertexFormats, pMemoryAllocator, pLimits->maxVertexFormatCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createHashMap<graphics_pipeline_t>(&pOutRenderResourceCache->graphicPipelines, pMemoryAllocator, pLimits->maxPipelineStateCount, clear_memory);

    renderResourceCacheAllocationFailed |= !createDynamicArray<gpu_texture_t>(&pOutRenderResourceCache->gpuTextures, pMemoryAllocator, pLimits->maxGpuTextureCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createDynamicArray<gpu_buffer_t>(&pOutRenderResourceCache->gpuBuffers, pMemoryAllocator, pLimits->maxGpuBufferCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createDynamicArray<shader_binary_t>(&pOutRenderResourceCache->shaderBinaries, pMemoryAllocator, pLimits->maxShaderBinaryCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createDynamicArray<render_target_t>(&pOutRenderResourceCache->renderTargets, pMemoryAllocator, pLimits->maxRenderTargetCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createDynamicArray<render_pass_t>(&pOutRenderResourceCache->renderPasses, pMemoryAllocator, pLimits->maxRenderPassCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createDynamicArray<texture_sampler_t>(&pOutRenderResourceCache->sampler, pMemoryAllocator, pLimits->maxSamplerCount, clear_memory);

    if(renderResourceCacheAllocationFailed)
    {
        destroyRenderResourceCache(pOutRenderResourceCache);
        return false;
    }

    createLinkedList(&pOutRenderResourceCache->pFirstFreeRenderPass, (render_pass_t*)pOutRenderResourceCache->renderPasses.pData, pOutRenderResourceCache->renderPasses.capacity);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeRenderTarget, (render_target_t*)pOutRenderResourceCache->renderTargets.pData, pOutRenderResourceCache->renderTargets.capacity);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeGpuBuffer, (gpu_buffer_t*)pOutRenderResourceCache->gpuBuffers.pData, pOutRenderResourceCache->gpuBuffers.capacity);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeGpuTexture, (gpu_texture_t*)pOutRenderResourceCache->gpuTextures.pData, pOutRenderResourceCache->gpuTextures.capacity);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeShaderBinary, (shader_binary_t*)pOutRenderResourceCache->shaderBinaries.pData, pOutRenderResourceCache->shaderBinaries.capacity);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeSampler, (texture_sampler_t*)pOutRenderResourceCache->sampler.pData, pOutRenderResourceCache->sampler.capacity);

    if(!initRenderPasses(pDevice, (render_pass_t*)pOutRenderResourceCache->pFirstFreeRenderPass))
    {
        destroyRenderResourceCache(pOutRenderResourceCache);
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

    //FK: Assume every GpuBuffer is accessed by compute
    const uint32_t numDescriptors = pParameters->limits.maxGpuTextureCount + pParameters->limits.maxRenderTargetCount + pParameters->limits.maxGpuBufferCount;
    if(!createDescriptorHeap(&pRenderContext->shaderVisibleDescriptorHeap, pRenderContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, numDescriptors))
    {
        return false;
    }

    if(!createDescriptorHeap(&pRenderContext->samplerDescriptorHeap, pRenderContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pParameters->limits.maxSamplerCount))
    {
        return false;
    }

    graphics_frame_parameters_t graphicsFrameParameters = {};
    graphicsFrameParameters.maxRenderPassCount      = pParameters->limits.maxRenderPassCount;
    graphicsFrameParameters.pShaderDescriptorHeap   = &pRenderContext->shaderVisibleDescriptorHeap;
    graphicsFrameParameters.pSamplerDescriptorHeap  = &pRenderContext->samplerDescriptorHeap;
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

    setD3D12ObjectDebugName(pRenderContext->pDefaultDirectCommandQueue, "Default Direct Command Queue");
    setD3D12ObjectDebugName(pRenderContext->pDefaultCopyCommandQueue, "Default Copy Command Queue");

    pRenderContext->frameIndex = 1u;

    return true;
}

void validateMappedBuffers(const graphics_frame_t* pGraphicsFrame)
{
#if USE_VALIDATION
    gpu_buffer_t* pMappedGpuBuffer = pGraphicsFrame->pFirstMappedGpuBuffer;
    while(pMappedGpuBuffer)
    {
        logWarning("Gpu buffer '%s' has not been unmapped.", pMappedGpuBuffer->pName);
    }
#endif
}

NO_DISCARD graphics_frame_t* beginNextFrame(render_context_t* pRenderContext)
{
    ASSERT_DEBUG(pRenderContext != nullptr);
    ASSERT_DEBUG(pRenderContext->pCurrentGraphicsFrame == nullptr);

    const uint64_t frameIndex = pRenderContext->frameIndex % pRenderContext->graphicsFramesCollection.frameCount;
    graphics_frame_t* pGraphicsFrame = getGraphicsFrameFromGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, frameIndex);
    
    flushFrame(pGraphicsFrame);
    freePendingFrameResources(pGraphicsFrame);
    resetFrame(pGraphicsFrame);

    const uint32_t currentBackBufferIndex = pRenderContext->swapChain.pSwapChain->GetCurrentBackBufferIndex();
    pRenderContext->pCurrentGraphicsFrame = pGraphicsFrame;
    pGraphicsFrame->frameIndex = pRenderContext->frameIndex;
    pGraphicsFrame->pBackBuffer = pRenderContext->swapChain.pBackBufferRenderTargets + currentBackBufferIndex;
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

    validateMappedBuffers(pGraphicsFrame);

    pGraphicsFrame->pFrameGeneralGraphicsCommandList->Close();
    pRenderContext->pDefaultDirectCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pGraphicsFrame->pFrameGeneralGraphicsCommandList);

    render_pass_t* pRenderPass = pGraphicsFrame->pFirstRenderPassToExecute;
    while(pRenderPass)
    {
        pRenderContext->pDefaultDirectCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pRenderPass->pGraphicsCommandList);
        pRenderContext->pDefaultDirectCommandQueue->Signal(pRenderPass->pRenderPassFence, pGraphicsFrame->frameIndex);
        pRenderPass = (render_pass_t*)pRenderPass->pNext;
    }

    COM_CALL(pRenderContext->swapChain.pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

    COM_CALL(pRenderContext->pDefaultDirectCommandQueue->Signal(pGraphicsFrame->pFrameFence, pGraphicsFrame->frameIndex));
    COM_CALL(pGraphicsFrame->pFrameFence->SetEventOnCompletion(pGraphicsFrame->frameIndex, pGraphicsFrame->pFrameFinishedEvent));

    pRenderContext->pCurrentGraphicsFrame = nullptr;
}

template<typename T>
T* getFreeObjectFromFreeList(linked_list_node_t<T>** ppFreeList, dynamic_array_t<T>* pBackupStorage, const flags8_t<render_resource_flags_t> flags, const char* pObjectName)
{
    if((*ppFreeList) == nullptr)
    {
        if(flags & render_resource_flags_t::notify_on_array_grow)
        {
            logWarning("free list for '%s' ran out of space, growing backing storage.\n", pObjectName);

            const uint32_t oldCapacity = pBackupStorage->capacity;
            T* pNodesToAdd = (T*)pBackupStorage->pData + pBackupStorage->capacity;
            if(!tryToGrowDynamicArray(pBackupStorage))
            {
                logError("could not grow backing storage for '%s'.\n", pObjectName);
                return nullptr;
            }

            const uint32_t newCapacity = pBackupStorage->capacity;
            const uint32_t nodesToAdd = newCapacity - oldCapacity;
            *ppFreeList = addNodesToLinkedList(ppFreeList, pNodesToAdd, nodesToAdd);
        }
    }

    linked_list_node_t<T>* pFreeObject = *ppFreeList;
    *ppFreeList = (linked_list_node_t<T>*)pFreeObject->pNext;
    return (T*)pFreeObject;
}

shader_binary_t* getFreeShaderBinary(render_resource_cache_t* pRenderResourceCache)
{
    return getFreeObjectFromFreeList(&pRenderResourceCache->pFirstFreeShaderBinary, &pRenderResourceCache->shaderBinaries, pRenderResourceCache->flags, "Shader Binaries");
}

render_pass_t* getFreeRenderPass(render_resource_cache_t* pRenderResourceCache)
{
    return getFreeObjectFromFreeList(&pRenderResourceCache->pFirstFreeRenderPass, &pRenderResourceCache->renderPasses, pRenderResourceCache->flags, "Render Passes");
}

gpu_texture_t* getFreeGpuTexture(render_resource_cache_t* pRenderResourceCache)
{
    gpu_texture_t* pGpuTexture = getFreeObjectFromFreeList(&pRenderResourceCache->pFirstFreeGpuTexture, &pRenderResourceCache->gpuTextures, pRenderResourceCache->flags, "Gpu Textures");
    ASSERT_DEBUG(pGpuTexture != nullptr);
    ASSERT_DEBUG((pGpuTexture->flags & gpu_texture_flag_t::marked_as_free) == 0);
    return pGpuTexture;
}

texture_sampler_t* getFreeSampler(render_resource_cache_t* pRenderResourceCache)
{
    return getFreeObjectFromFreeList(&pRenderResourceCache->pFirstFreeSampler, &pRenderResourceCache->sampler, pRenderResourceCache->flags, "Samplers");
}

gpu_buffer_t* getFreeGpuBuffer(render_resource_cache_t* pRenderResourceCache)
{
    gpu_buffer_t* pGpuBuffer = getFreeObjectFromFreeList(&pRenderResourceCache->pFirstFreeGpuBuffer, &pRenderResourceCache->gpuBuffers, pRenderResourceCache->flags, "Gpu Buffers");
    ASSERT_DEBUG(pGpuBuffer != nullptr);
    ASSERT_DEBUG((pGpuBuffer->flags & gpu_buffer_flag_t::marked_as_free) == 0);
    return pGpuBuffer;
}

NO_DISCARD render_pass_t* startRenderPass(graphics_frame_t* pGraphicsFrame, const char* pRenderPassName, render_target_t* pRenderTarget)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);

    render_pass_t* pRenderPass = getFreeRenderPass(pGraphicsFrame->pRenderResourceCache);
    if(pRenderPass == nullptr)
    {
        return nullptr;
    }

    ++pGraphicsFrame->openRenderPassCount;

    resetRenderState(&pRenderPass->state);
    COM_CALL(pRenderPass->pGraphicsCommandAllocator->Reset());
    COM_CALL(pRenderPass->pGraphicsCommandList->Reset(pRenderPass->pGraphicsCommandAllocator, nullptr));

    pRenderPass->isOpen = true;
    pRenderPass->pName = pRenderPassName;
    pRenderPass->pGraphicsFrame = pGraphicsFrame;
    pRenderPass->state.pRenderTarget = pRenderTarget;
    pRenderPass->state.viewport.height = pRenderTarget->dimensions.y;
    pRenderPass->state.viewport.width = pRenderTarget->dimensions.x;
    pRenderPass->state.viewport.minDepth = 1000.0f;
    pRenderPass->state.scissor.width = pRenderTarget->dimensions.x;
    pRenderPass->state.scissor.height = pRenderTarget->dimensions.y;

    transitionResource(pRenderPass->pGraphicsCommandList, &pRenderTarget->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);

    setD3D12ObjectDebugName(pRenderPass->pGraphicsCommandList, pRenderPassName);    
    addPixBeginMarker(pRenderPass->pGraphicsCommandList, pRenderPassName);
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

    transitionResource(pRenderPass->pGraphicsCommandList, &pRenderPass->state.pRenderTarget->resource, D3D12_RESOURCE_STATE_COMMON);

    addPixEndMarker(pRenderPass->pGraphicsCommandList);
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

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return 0u;
}

uint32_t getDXGIFormatSizeInBytes(const DXGI_FORMAT format) 
{
    switch (format) 
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 16;
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 12;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return 8;
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return 8;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return 4;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return 4;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        return 2;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        return 4;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return 4;
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        return 2;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return 0u;
}

uint32_t calculateVertexStrideSizeInBytes(const vertex_format_t* pVertexFormat)
{
    uint32_t strideSizeInBytes = 0u;
    for(uint32_t attributeIndex = 0u; attributeIndex < pVertexFormat->inputElementCount; ++attributeIndex)
    {
        strideSizeInBytes += getDXGIFormatSizeInBytes(pVertexFormat->pInputElementDescs[attributeIndex].Format);
    }

    return strideSizeInBytes;
}

void bindVertexBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pVertexBuffer, const vertex_format_t* pVertexFormat, uint32_t slotIndex)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pVertexBuffer != nullptr);
    if(!Validate(pVertexBuffer->bufferUsage == gpu_buffer_usage_t::vertex_buffer, "Gpu buffer '%s' is not a vertex buffer.", pVertexBuffer->pName))
    {
        return;
    }

    transitionResource(pRenderPass->pGraphicsCommandList, &pVertexBuffer->resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = pVertexBuffer->resource.pResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes    = pVertexBuffer->sizeInBytes;
    vertexBufferView.StrideInBytes  = calculateVertexStrideSizeInBytes(pVertexFormat);
    pRenderPass->pGraphicsCommandList->IASetVertexBuffers(slotIndex, 1u, &vertexBufferView);
}

void bindIndexBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pIndexBuffer, const index_format_t indexFormat)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pIndexBuffer != nullptr);
    if(!Validate(pIndexBuffer->bufferUsage == gpu_buffer_usage_t::index_buffer, "Gpu buffer '%s' is not an index buffer.", pIndexBuffer->pName))
    {
        return;
    }

    transitionResource(pRenderPass->pGraphicsCommandList, &pIndexBuffer->resource, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    const DXGI_FORMAT dxgiIndexFormat = (indexFormat == index_format_t::unsigned_int_16bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation  = pIndexBuffer->resource.pResource->GetGPUVirtualAddress();
    indexBufferView.Format          = dxgiIndexFormat;
    indexBufferView.SizeInBytes     = pIndexBuffer->sizeInBytes;
    pRenderPass->pGraphicsCommandList->IASetIndexBuffer(&indexBufferView);
}

void bindConstantBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pConstantBuffer, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pConstantBuffer != nullptr);
    if(!Validate(pConstantBuffer->bufferUsage == gpu_buffer_usage_t::constant_buffer, "Gpu buffer '%s' is not a constant buffer.", pConstantBuffer->pName))
    {
        return;
    }

    ASSERT_DEBUG(pRenderPass->state.boundResourceCount < sizeof(pRenderPass->state.boundResources) / sizeof(pRenderPass->state.boundResources[0]));

    const uint32_t boundResourceIndex = pRenderPass->state.boundResourceCount++;
    pRenderPass->state.boundResources[boundResourceIndex].type              = bound_resource_type_t::constant_buffer;
    pRenderPass->state.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->state.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->state.boundResources[boundResourceIndex].pResource         = &pConstantBuffer->resource;
}

void bindTexture(render_pass_t* pRenderPass, gpu_texture_t* pTexture, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pTexture != nullptr);

    ASSERT_DEBUG(pRenderPass->state.boundResourceCount < sizeof(pRenderPass->state.boundResources) / sizeof(pRenderPass->state.boundResources[0]));

    const uint32_t boundResourceIndex = pRenderPass->state.boundResourceCount++;
    pRenderPass->state.boundResources[boundResourceIndex].type              = bound_resource_type_t::texture;
    pRenderPass->state.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->state.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->state.boundResources[boundResourceIndex].pResource         = &pTexture->resource;
}

void bindTextureSampler(render_pass_t* pRenderPass, texture_sampler_t* pSampler, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pSampler != nullptr);

    ASSERT_DEBUG(pRenderPass->state.boundResourceCount < sizeof(pRenderPass->state.boundResources) / sizeof(pRenderPass->state.boundResources[0]));

    const uint32_t boundResourceIndex = pRenderPass->state.boundResourceCount++;
    pRenderPass->state.boundResources[boundResourceIndex].type              = bound_resource_type_t::sampler;
    pRenderPass->state.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->state.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->state.boundResources[boundResourceIndex].descriptorHandle  = pSampler->descriptorHandle;
}

void bindGraphicsPipeline(render_pass_t* pRenderPass, graphics_pipeline_t* pGraphicsPipeline)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pGraphicsPipeline != nullptr);
    pRenderPass->state.pGraphicsPipeline = pGraphicsPipeline;
}

void applyGraphicsPipeline(ID3D12GraphicsCommandList* pGraphicsCommandList, render_state_t* pRenderState, const graphics_pipeline_t* pGraphicsPipeline)
{
    if(pRenderState->pGraphicsPipeline == pGraphicsPipeline)
    {
        return;
    }
    
    pRenderState->pGraphicsPipeline = pGraphicsPipeline;

    pGraphicsCommandList->SetPipelineState(pGraphicsPipeline->pPipelineState);
	pGraphicsCommandList->SetGraphicsRootSignature(pGraphicsPipeline->pRootSignature);
	pGraphicsCommandList->IASetPrimitiveTopology(mapTopologyToD3D12Topology(pGraphicsPipeline->topology));
}

void applyRenderTarget(ID3D12GraphicsCommandList* pGraphicsCommandList, render_state_t* pRenderState, render_target_t* pRenderTarget)
{
    if(pRenderState->pRenderTarget == pRenderTarget)
    {
        return;
    }

    pRenderState->pRenderTarget = pRenderTarget;

    transitionResource(pGraphicsCommandList, &pRenderTarget->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
    D3D12_CPU_DESCRIPTOR_HANDLE* pColorDescriptorHandle = pRenderTarget->colorBufferHandle.ptr == 0u ? nullptr : &pRenderTarget->colorBufferHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE* pDepthDescriptorHandle = pRenderTarget->depthBufferHandle.ptr == 0u ? nullptr : &pRenderTarget->depthBufferHandle;
    pGraphicsCommandList->OMSetRenderTargets(1u, pColorDescriptorHandle, FALSE, pDepthDescriptorHandle);
}

void applyViewport(ID3D12GraphicsCommandList* pGraphicsCommandList, render_state_t* pRenderState, const viewport_t* pViewport)
{
    if(memcmp(&pRenderState->viewport, pViewport, sizeof(viewport_t)) == 0u)
    {
        return;
    }

    pRenderState->viewport = *pViewport;

    D3D12_VIEWPORT viewport = {};
    viewport.Height     = (float)pViewport->height;
    viewport.Width      = (float)pViewport->width;
    viewport.TopLeftX   = (float)pViewport->x;
    viewport.TopLeftY   = (float)pViewport->y;
    viewport.MinDepth   = (float)pViewport->minDepth;
    viewport.MaxDepth   = (float)pViewport->maxDepth;
    pGraphicsCommandList->RSSetViewports(1u, &viewport);
}

void applyScissor(ID3D12GraphicsCommandList* pGraphicsCommandList, render_state_t* pRenderState, const scissor_t* pScissor)
{
    if(memcmp(&pRenderState->scissor, pScissor, sizeof(scissor_t)) == 0u)
    {
        return;
    }

    pRenderState->scissor = *pScissor;

    D3D12_RECT scissorRect = {};
    scissorRect.left    = pScissor->x;
    scissorRect.top     = pScissor->y;
    scissorRect.right   = pScissor->x + pScissor->width;
    scissorRect.bottom  = pScissor->y + pScissor->height;
    pGraphicsCommandList->RSSetScissorRects(1u, &scissorRect);
}

bool hasMatchingBindingPoint(const shader_binding_point_t* pShaderBindingPoints, const uint32_t shaderBindingPointCount, const bound_resource_t* pBoundResource)
{
    for(uint32_t bindingPointIndex = 0u; bindingPointIndex < shaderBindingPointCount; ++bindingPointIndex)
    {
        const shader_binding_point_t* pShaderBindingPoint = &pShaderBindingPoints[bindingPointIndex];
        if(pShaderBindingPoint->type == pBoundResource->type && pShaderBindingPoint->slot == pBoundResource->registerIndex && pShaderBindingPoint->space == pBoundResource->registerSpace)
        {
            return true;
        }
    }

    return false;
}

void applyBoundBufferResources(ID3D12GraphicsCommandList* pGraphicsCommandList, const graphics_pipeline_t* pGraphicsPipeline, render_state_t* pRenderPassState, const bound_resource_t* pBoundResources, const uint32_t boundResourceCount)
{
    if(memcmp(pRenderPassState->boundResources, pBoundResources, sizeof(bound_resource_t) * boundResourceCount) == 0) 
    {
        return;
    }

    pRenderPassState->boundResourceCount = boundResourceCount;

    for(uint32_t boundResourceIndex = 0u; boundResourceIndex < boundResourceCount; ++boundResourceIndex)
    {
        const bound_resource_t* pBoundResource = &pBoundResources[boundResourceIndex];
        const bound_resource_t* pAlreadyBoundResource = &pRenderPassState->boundResources[boundResourceIndex];
        if(memcmp(pAlreadyBoundResource, pBoundResource, sizeof(bound_resource_t)) == 0)
        {
            continue;
        }

        if(!hasMatchingBindingPoint(pGraphicsPipeline->pShaderBindingPoints, pGraphicsPipeline->shaderBindingPointCount, pBoundResource))
        {
            continue;
        }

        if(pBoundResource->type == bound_resource_type_t::constant_buffer)
        {
            transitionResource(pGraphicsCommandList, pBoundResource->pResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = (D3D12_GPU_VIRTUAL_ADDRESS)pBoundResource->pResource->descriptorHandle.gpuDescriptorHandle.ptr;
            pGraphicsCommandList->SetGraphicsRootConstantBufferView(boundResourceIndex, gpuAddress);
            continue;
        }
        else if(pBoundResource->type == bound_resource_type_t::texture)
        {
            transitionResource(pGraphicsCommandList, pBoundResource->pResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            pGraphicsCommandList->SetGraphicsRootDescriptorTable(boundResourceIndex, pBoundResource->pResource->descriptorHandle.gpuDescriptorHandle);
            continue;
        }
        else if(pBoundResource->type == bound_resource_type_t::sampler)
        {
            pGraphicsCommandList->SetGraphicsRootDescriptorTable(boundResourceIndex, pBoundResource->descriptorHandle.gpuDescriptorHandle);
            continue;
        }
        else
        {
            ASSERT_DEBUG_UNREACHABLE_CODE();
        }
    }

    memcpy(pRenderPassState->boundResources, pBoundResources, sizeof(bound_resource_t) * boundResourceCount);
}

void applyRenderState(ID3D12GraphicsCommandList* pGraphicsCommandList, graphics_frame_t* pGraphicsFrame, render_state_t* pRenderPassState)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderPassState != nullptr);
    ASSERT_DEBUG(pGraphicsCommandList != nullptr);

    ID3D12DescriptorHeap* descriptorHeaps[] = {
        pGraphicsFrame->pSamplerDescriptorHeap->pDescriptorHeap,
        pGraphicsFrame->pShaderVisibleDescriptorHeap->pDescriptorHeap
    };
    pGraphicsCommandList->SetDescriptorHeaps(2u, descriptorHeaps);

    applyViewport(pGraphicsCommandList, &pGraphicsFrame->renderState, &pRenderPassState->viewport);
    applyScissor(pGraphicsCommandList, &pGraphicsFrame->renderState, &pRenderPassState->scissor);
    applyGraphicsPipeline(pGraphicsCommandList, &pGraphicsFrame->renderState, pRenderPassState->pGraphicsPipeline);
    applyRenderTarget(pGraphicsCommandList, &pGraphicsFrame->renderState, pRenderPassState->pRenderTarget);
    applyBoundBufferResources(pGraphicsCommandList, pRenderPassState->pGraphicsPipeline, &pGraphicsFrame->renderState, pRenderPassState->boundResources, pRenderPassState->boundResourceCount);
}

void draw(render_pass_t* pRenderPass, const uint32_t vertexOffset, const uint32_t vertexCount)
{
    applyRenderState(pRenderPass->pGraphicsCommandList, pRenderPass->pGraphicsFrame, &pRenderPass->state);
	pRenderPass->pGraphicsCommandList->DrawInstanced(vertexCount, 1u, vertexOffset, 0u);
}

void drawIndexed(render_pass_t* pRenderPass, const uint32_t indexOffset, const uint32_t indexCount)
{
    applyRenderState(pRenderPass->pGraphicsCommandList, pRenderPass->pGraphicsFrame, &pRenderPass->state);
    pRenderPass->pGraphicsCommandList->DrawIndexedInstanced(indexCount, 1u, indexOffset, 0u, 0u);
}

void clearColorRenderTarget(render_pass_t* pRenderPass, render_target_t* pRenderTarget, const float r, const float g, const float b, const float a)
{
    ASSERT_DEBUG(pRenderTarget != nullptr);
    ASSERT_DEBUG(isColorRenderTarget(pRenderTarget));
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderPass->isOpen);

    const FLOAT colorValues[4] = {r, g, b, a};

    pRenderPass->pGraphicsCommandList->ClearRenderTargetView(pRenderTarget->colorBufferHandle, colorValues, 0, nullptr);
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
}

D3D12_RESOURCE_STATES mapGpuUsageToResourceState(const gpu_buffer_usage_t gpuBufferUsage)
{
    switch(gpuBufferUsage)
    {
        case gpu_buffer_usage_t::vertex_buffer:
        case gpu_buffer_usage_t::constant_buffer:
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case gpu_buffer_usage_t::index_buffer:
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case gpu_buffer_usage_t::storage_buffer:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_HEAP_TYPE mapGpuMemoryHintToHeapType(const gpu_memory_usage_hint_t memoryUsageHint)
{
    switch(memoryUsageHint)
    {
        case gpu_memory_usage_hint_t::cpuReadAccess:
            return D3D12_HEAP_TYPE_READBACK;
        case gpu_memory_usage_hint_t::cpuWriteGpuReadAccess:
            return D3D12_HEAP_TYPE_UPLOAD;
        case gpu_memory_usage_hint_t::gpuExclusiveAccess:
            return D3D12_HEAP_TYPE_DEFAULT;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_CPU_PAGE_PROPERTY mapGpuMemoryHintToCpuPageProperty(const gpu_memory_usage_hint_t memoryUsageHint)
{
    switch(memoryUsageHint)
    {
        case gpu_memory_usage_hint_t::cpuReadAccess:
            return D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        case gpu_memory_usage_hint_t::cpuWriteGpuReadAccess:
            return D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
        case gpu_memory_usage_hint_t::gpuExclusiveAccess:
            return D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
}

D3D12_MEMORY_POOL mapGpuBufferMemoryHintToMemoryPool(const gpu_memory_usage_hint_t memoryUsageHint)
{
    switch(memoryUsageHint)
    {
        case gpu_memory_usage_hint_t::cpuReadAccess:
            return D3D12_MEMORY_POOL_L0;
        case gpu_memory_usage_hint_t::cpuWriteGpuReadAccess:
            return D3D12_MEMORY_POOL_L0;
        case gpu_memory_usage_hint_t::gpuExclusiveAccess:
            return D3D12_MEMORY_POOL_L1;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_MEMORY_POOL_UNKNOWN;
}

bound_resource_type_t mapShaderInputType(const D3D_SHADER_INPUT_TYPE shaderInputType)
{
    switch(shaderInputType)
    {
        case D3D_SIT_CBUFFER:
            return bound_resource_type_t::constant_buffer;
        case D3D_SIT_TEXTURE:
            return bound_resource_type_t::texture;
        case D3D_SIT_SAMPLER:
            return bound_resource_type_t::sampler;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return bound_resource_type_t::constant_buffer;
}

bool isBlockTextureFormat(const gpu_texture_format_t format)
{
    return format == gpu_texture_format_t::BC1 ||
        format == gpu_texture_format_t::BC2 ||
        format == gpu_texture_format_t::BC3 ||
        format == gpu_texture_format_t::BC4 ||
        format == gpu_texture_format_t::BC5 ||
        format == gpu_texture_format_t::BC7;
}

uint32_t getBlockSizeInBytes(const gpu_texture_format_t format)
{
    ASSERT_DEBUG(format >= gpu_texture_format_t::BC1 && format <= gpu_texture_format_t::BC7);
    switch(format)
    {
        case gpu_texture_format_t::BC1:
            return 8u;
        case gpu_texture_format_t::BC2:
        case gpu_texture_format_t::BC3:
            return 16u;
        case gpu_texture_format_t::BC4:
            return 8u;
        case gpu_texture_format_t::BC5:
        case gpu_texture_format_t::BC7:
            return 16u;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return 0u;
}

uint32_t getSizeInBytesOfTextureFormat(const gpu_texture_format_t format)
{
    switch(format)
    {
        case gpu_texture_format_t::R8:
            return 1u;
        case gpu_texture_format_t::R16:
            return 2u;
        case gpu_texture_format_t::R8G8B8A8:
        case gpu_texture_format_t::R16G16:
        case gpu_texture_format_t::R32:
        case gpu_texture_format_t::R10G10B10A2:
        case gpu_texture_format_t::D24S8:
        case gpu_texture_format_t::D32:
            return 4u;
        case gpu_texture_format_t::R16G16B16A16:
        case gpu_texture_format_t::R32G32:
            return 8u;
        case gpu_texture_format_t::R32G32B32:
            return 12u;
        case gpu_texture_format_t::R32G32B32A32:
            return 16u;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return 0u;
}

uint32_t calculateTextureDataSizeInBytes(uint3_t dimensions, const gpu_texture_format_t format)
{
    const bool isBlockFormat = isBlockTextureFormat(format);
    if(!isBlockFormat)
    {
        const uint32_t numPixels = dimensions.x * dimensions.y * dimensions.z;
        return numPixels * getSizeInBytesOfTextureFormat(format);
    }

    ASSERT_DEBUG(dimensions.z == 1);
    ASSERT_DEBUG(dimensions.y > 4u && PopulationCount64(dimensions.y) == 1u);
    ASSERT_DEBUG(dimensions.x > 4u && PopulationCount64(dimensions.x) == 1u);
    const uint32_t numPixels = dimensions.x * dimensions.y;
    const uint32_t blockCount = numPixels >> 4u;
    const uint32_t blockSizeInBytes = getBlockSizeInBytes(format);
    return blockCount * blockSizeInBytes;
}

uint32_t calculateTextureDataSizeInBytesRecursive(uint32_t sizeAccumulator, uint3_t dimensions, const gpu_texture_format_t format, const uint32_t mipMapLevels)
{
    if(mipMapLevels == 0u)
    {
        return sizeAccumulator;
    }

    sizeAccumulator += calculateTextureDataSizeInBytes(dimensions, format);
    ASSERT_DEBUG(dimensions.x > 1 && dimensions.y > 1);
    dimensions.x >>= 1u;
    dimensions.y >>= 1u;
    
    return calculateTextureDataSizeInBytesRecursive(sizeAccumulator, dimensions, format, mipMapLevels - 1);
}

void copyGpuTexture(graphics_frame_t* pGraphicsFrame, gpu_texture_t* pDestinationTexture, gpu_texture_t* pSourceTexture)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pDestinationTexture != nullptr);
    ASSERT_DEBUG(pSourceTexture != nullptr);

    if(!Validate(pDestinationTexture->sizeInBytes == pSourceTexture->sizeInBytes, "Trying to copy source texture '%s' into destination texture '%s' but they don't have the same size.", pSourceTexture->pName, pDestinationTexture->pName))
    {
        return;
    }

    if(!Validate(pDestinationTexture->format == pSourceTexture->format, "Trying to copy source texture '%s' into destination texture '%s' but they don't have the same format.", pSourceTexture->pName, pDestinationTexture->pName))
    {
        return;
    }

    const uint32_t bufferSizeInBytes = pSourceTexture->sizeInBytes;
    //transitionResource(pGraphicsFrame->pFrameGeneralGraphicsCommandList, &pDestinationBuffer->bufferResource, D3D12_RESOURCE_STATE_COPY_DEST);
    //transitionResource(pGraphicsFrame->pFrameGeneralGraphicsCommandList, &pSourceBuffer->bufferResource, D3D12_RESOURCE_STATE_COPY_SOURCE);

    D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation = {};
    sourceCopyLocation.pResource = pSourceTexture->resource.pResource;
    sourceCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    sourceCopyLocation.SubresourceIndex = 0u;

    D3D12_TEXTURE_COPY_LOCATION destinationCopyLocation = {};
    destinationCopyLocation.pResource = pDestinationTexture->resource.pResource;
    destinationCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destinationCopyLocation.SubresourceIndex = 0u;

    pGraphicsFrame->pFrameGeneralGraphicsCommandList->CopyTextureRegion(&destinationCopyLocation, 0u, 0u, 0u, &sourceCopyLocation, nullptr);
}

void copyGpuTextureFromBuffer(graphics_frame_t* pGraphicsFrame, gpu_texture_t* pDestinationTexture, gpu_buffer_t* pSourceBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pDestinationTexture != nullptr);
    ASSERT_DEBUG(pSourceBuffer != nullptr);

    if(!Validate(pDestinationTexture->sizeInBytes == pSourceBuffer->sizeInBytes, "Trying to copy source buffer '%s' into destination texture '%s' but they don't have the same size.", pSourceBuffer->pName, pDestinationTexture->pName))
    {
        return;
    }

    const uint32_t bufferSizeInBytes = pSourceBuffer->sizeInBytes;

    D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation = {};
    sourceCopyLocation.pResource = pSourceBuffer->resource.pResource;
    sourceCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    sourceCopyLocation.PlacedFootprint.Footprint.Depth      = pDestinationTexture->dimensions.z;
    sourceCopyLocation.PlacedFootprint.Footprint.Height     = pDestinationTexture->dimensions.y;
    sourceCopyLocation.PlacedFootprint.Footprint.Width      = pDestinationTexture->dimensions.x;
    sourceCopyLocation.PlacedFootprint.Footprint.RowPitch   = getSizeInBytesOfTextureFormat(pDestinationTexture->format) * pDestinationTexture->dimensions.x;
    sourceCopyLocation.PlacedFootprint.Footprint.Format     = mapTextureFormatToD3D12Format(pDestinationTexture->format, pDestinationTexture->formatType);

    D3D12_TEXTURE_COPY_LOCATION destinationCopyLocation = {};
    destinationCopyLocation.pResource = pDestinationTexture->resource.pResource;
    destinationCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    transitionResource(pGraphicsFrame->pFrameGeneralGraphicsCommandList, &pDestinationTexture->resource, D3D12_RESOURCE_STATE_COPY_DEST);
    transitionResource(pGraphicsFrame->pFrameGeneralGraphicsCommandList, &pSourceBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
    pGraphicsFrame->pFrameGeneralGraphicsCommandList->CopyTextureRegion(&destinationCopyLocation, 0u, 0u, 0u, &sourceCopyLocation, nullptr);
}

void copyGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pDestinationBuffer, gpu_buffer_t* pSourceBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pDestinationBuffer != nullptr);
    ASSERT_DEBUG(pSourceBuffer != nullptr);

    if(!Validate(pSourceBuffer->sizeInBytes == pDestinationBuffer->sizeInBytes, "Trying to copy source buffer '%s' into destination buffer '%s' but they don't have the same size.", pSourceBuffer->pName, pDestinationBuffer->pName))
    {
        return;
    }

    const uint32_t bufferSizeInBytes = pSourceBuffer->sizeInBytes;
    pGraphicsFrame->pFrameGeneralGraphicsCommandList->CopyBufferRegion(pDestinationBuffer->resource.pResource, 0u, pSourceBuffer->resource.pResource, 0u, bufferSizeInBytes);
}

D3D12_FILTER_TYPE mapSamplerFilterTypeToD3D12SamplerFilterType(const texture_sampler_filter_type_t filterType)
{
    switch(filterType)
    {
        case texture_sampler_filter_type_t::point:
            return D3D12_FILTER_TYPE_POINT;
        case texture_sampler_filter_type_t::linear:
            return D3D12_FILTER_TYPE_LINEAR;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_FILTER_TYPE_POINT;
}

D3D12_TEXTURE_ADDRESS_MODE mapTextureAddressModeToD3D12TextureAddressMode(const texture_sampler_address_mode_type_t addressModeType)
{
    switch(addressModeType)
    {
        case texture_sampler_address_mode_type_t::border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case texture_sampler_address_mode_type_t::clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case texture_sampler_address_mode_type_t::mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case texture_sampler_address_mode_type_t::wrap:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
}

D3D12_FILTER mapSamplerFilterToD3D12SamplerFilter(const texture_sampler_filter_type_t minifactionFilter, const texture_sampler_filter_type_t magnificationFilter, const texture_sampler_filter_type_t mipmapFilter)
{
    const D3D12_FILTER_TYPE minFilter = mapSamplerFilterTypeToD3D12SamplerFilterType(minifactionFilter);
    const D3D12_FILTER_TYPE magFilter = mapSamplerFilterTypeToD3D12SamplerFilterType(magnificationFilter);
    const D3D12_FILTER_TYPE mipFilter = mapSamplerFilterTypeToD3D12SamplerFilterType(mipmapFilter);
    return D3D12_ENCODE_BASIC_FILTER(minFilter, magFilter, mipFilter, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
}

NO_DISCARD texture_sampler_t* createTextureSampler(graphics_frame_t* pGraphicsFrame, const texture_sampler_parameter_t* pParameter)
{
    texture_sampler_t* pSampler = getFreeSampler(pGraphicsFrame->pRenderResourceCache);
    if(pSampler == nullptr)
    {
        return nullptr;
    }

    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = mapSamplerFilterToD3D12SamplerFilter(pParameter->minifactionFilter, pParameter->magnificationFilter, pParameter->mipmapFilter);
    samplerDesc.AddressU = mapTextureAddressModeToD3D12TextureAddressMode(pParameter->addressModeU);
    samplerDesc.AddressV = mapTextureAddressModeToD3D12TextureAddressMode(pParameter->addressModeV);
    samplerDesc.AddressW = mapTextureAddressModeToD3D12TextureAddressMode(pParameter->addressModeW);
    samplerDesc.MaxAnisotropy = 1;
    //samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    //samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_CPU_DESCRIPTOR_HANDLE samplerDescriptorHandle = getNextCPUDescriptorHandle(pGraphicsFrame->pSamplerDescriptorHeap);
    pGraphicsFrame->pDevice->CreateSampler(&samplerDesc, samplerDescriptorHandle);

    pSampler->descriptorHandle.cpuDescriptorHandle  = samplerDescriptorHandle;
    pSampler->descriptorHandle.gpuDescriptorHandle  = getNextGPUDescriptorHandle(pGraphicsFrame->pSamplerDescriptorHeap);
    pSampler->minifactionFilter                     = pParameter->minifactionFilter;
    pSampler->magnificationFilter                   = pParameter->magnificationFilter;
    pSampler->mipmapFilter                          = pParameter->mipmapFilter;
    pSampler->addressModeU                          = pParameter->addressModeU;
    pSampler->addressModeV                          = pParameter->addressModeV;
    pSampler->addressModeW                          = pParameter->addressModeW;

    return pSampler;
}

NO_DISCARD gpu_buffer_t* createGpuBuffer(graphics_frame_t* pGraphicsFrame, uint32_t sizeInBytes, const void* pInitialData, gpu_buffer_usage_t bufferUsage, gpu_memory_usage_hint_t memoryUsageHint, const char* pName = "GpuBuffer")
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(sizeInBytes != 0);

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
    heapProperties.Type                 = mapGpuMemoryHintToHeapType(memoryUsageHint);
    heapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    com_auto_release_t<ID3D12Resource> pGpuBufferResource = nullptr;
    if(COM_CALL(pGraphicsFrame->pDevice->CreateCommittedResource1(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, nullptr, IID_PPV_ARGS(&pGpuBufferResource))) != S_OK)
    {
        return nullptr;
    }

    gpu_buffer_t* pGpuBuffer = getFreeGpuBuffer(pGraphicsFrame->pRenderResourceCache);
    if(pGpuBuffer == nullptr)
    {
        return nullptr;
    }

    #if 0
    if(usageFlags & gpu_buffer_usage_flag_t::shader_accessible)
    {
        pGpuBuffer->resource.cpuDescriptorHandle  = getNextCPUDescriptorHandle(pGraphicsFrame->pShaderVisibleDescriptorHeap);
        pGpuBuffer->resource.gpuDescriptorHandle  = getNextGPUDescriptorHandle(pGraphicsFrame->pShaderVisibleDescriptorHeap);
    }
    #endif

    pGpuBuffer->resource.descriptorHandle.gpuDescriptorHandle.ptr = pGpuBufferResource->GetGPUVirtualAddress();

    pGpuBuffer->resource.pResource            = pGpuBufferResource.pPointer;
    pGpuBuffer->resource.currentState         = D3D12_RESOURCE_STATE_COMMON;
    pGpuBuffer->sizeInBytes                   = sizeInBytes;
    pGpuBuffer->bufferUsage                   = bufferUsage;
    pGpuBuffer->memoryUsageHint               = memoryUsageHint;
    //pGpuBuffer->usageFlags                    = usageFlags;
    pGpuBuffer->pName                         = pName;

    if(pInitialData != nullptr)
    {
        bool useStagingBuffer = ( memoryUsageHint == gpu_memory_usage_hint_t::gpuExclusiveAccess );
        if(memoryUsageHint != gpu_memory_usage_hint_t::gpuExclusiveAccess)
        {
            void* pGpuBufferData = nullptr;
            if(COM_CALL(pGpuBufferResource->Map(0u, nullptr, &pGpuBufferData)) != S_OK)
            {
                useStagingBuffer = true;
            }
            else
            {
                memcpy(pGpuBufferData, pInitialData, sizeInBytes);
                pGpuBufferResource->Unmap(0u, nullptr);
            }
        }

        if(useStagingBuffer)
        {
            gpu_buffer_t* pStagingBuffer = createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, bufferUsage, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Staging Buffer");
            if(pStagingBuffer == nullptr)
            {
                return nullptr;
            }

            copyGpuBuffer(pGraphicsFrame, pGpuBuffer, pStagingBuffer);
            freeGpuBuffer(pGraphicsFrame, pStagingBuffer);
        }   
    }

    setD3D12ObjectDebugName(pGpuBuffer->resource.pResource, pGpuBuffer->pName);
    pGpuBufferResource.takeOwnership();
    return pGpuBuffer;
}

NO_DISCARD gpu_buffer_t* createIndexBuffer(graphics_frame_t* pGraphicsFrame, const uint32_t sizeInBytes, const void* pInitialData = nullptr)
{
    return createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, gpu_buffer_usage_t::index_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess);
}

NO_DISCARD gpu_buffer_t* createVertexBuffer(graphics_frame_t* pGraphicsFrame, const uint32_t sizeInBytes, const void* pInitialData = nullptr)
{
    return createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, gpu_buffer_usage_t::vertex_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess);
}

NO_DISCARD gpu_buffer_t* createConstantBuffer(graphics_frame_t* pGraphicsFrame, const uint32_t sizeInBytes, const void* pInitialData = nullptr)
{
    return createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, gpu_buffer_usage_t::constant_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess);
}

NO_DISCARD gpu_texture_t* createGpuTexture(graphics_frame_t* pGraphicsFrame, uint3_t dimensions, const void* pInitialData, gpu_texture_format_t format, gpu_texture_format_type_t formatType, gpu_memory_usage_hint_t memoryUsageHint, flags8_t<gpu_texture_flag_t> flags, const uint8_t mipMapLevels, const char* pName = "GpuTexture")
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(!(dimensions.x == 0 && dimensions.y == 0 && dimensions.z == 0));

    dimensions.y = dimensions.y == 0u ? 1u : dimensions.y;
    dimensions.z = dimensions.z == 0u ? 1u : dimensions.z;

    D3D12_RESOURCE_DIMENSION resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    if(dimensions.y > 1u && dimensions.z == 1u)
    {
        resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    }
    if(dimensions.y > 1u && dimensions.z > 1u)
    {
        resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }

    DXGI_FORMAT dxgiFormat = mapTextureFormatToD3D12Format(format, formatType);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = resourceDimension;
    desc.Alignment          = 0u;
    desc.Format             = dxgiFormat;
    desc.DepthOrArraySize   = dimensions.z;
    desc.Height             = dimensions.y;
    desc.Width              = dimensions.x;
    desc.MipLevels          = mipMapLevels == 0u ? 1u : mipMapLevels;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.SampleDesc.Count   = 1u;
    desc.SampleDesc.Quality = 0u;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                 = mapGpuMemoryHintToHeapType(memoryUsageHint);
    heapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    com_auto_release_t<ID3D12Resource> pGpuTextureResource = nullptr;
    if(COM_CALL(pGraphicsFrame->pDevice->CreateCommittedResource1(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, nullptr, IID_PPV_ARGS(&pGpuTextureResource))) != S_OK)
    {
        return nullptr;
    }

    gpu_texture_t* pGpuTexture = getFreeGpuTexture(pGraphicsFrame->pRenderResourceCache);
    if(pGpuTexture == nullptr)
    {
        return nullptr;
    }

    const uint32_t sizeInBytes = calculateTextureDataSizeInBytesRecursive(0u, dimensions, format, mipMapLevels);

    pGpuTexture->resource.descriptorHandle.cpuDescriptorHandle  = getNextCPUDescriptorHandle(pGraphicsFrame->pShaderVisibleDescriptorHeap);
    pGpuTexture->resource.descriptorHandle.gpuDescriptorHandle  = getNextGPUDescriptorHandle(pGraphicsFrame->pShaderVisibleDescriptorHeap);
    pGpuTexture->resource.pResource            = pGpuTextureResource.pPointer;
    pGpuTexture->resource.currentState         = D3D12_RESOURCE_STATE_COMMON;
    pGpuTexture->dimensions                    = dimensions;
    pGpuTexture->format                        = format;
    pGpuTexture->formatType                    = formatType;
    pGpuTexture->flags                         = flags;
    pGpuTexture->sizeInBytes                   = sizeInBytes;
    pGpuTexture->memoryUsageHint               = memoryUsageHint;
    pGpuTexture->pName                         = pName;

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format = dxgiFormat;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = 1u;
    pGraphicsFrame->pDevice->CreateShaderResourceView(pGpuTextureResource.pPointer, &shaderResourceViewDesc, pGpuTexture->resource.descriptorHandle.cpuDescriptorHandle);

    if(pInitialData != nullptr)
    {
        bool useStagingBuffer = ( memoryUsageHint == gpu_memory_usage_hint_t::gpuExclusiveAccess );
        if(memoryUsageHint != gpu_memory_usage_hint_t::gpuExclusiveAccess)
        {
            void* pGpuTextureData = nullptr;
            if(COM_CALL(pGpuTextureResource->Map(0u, nullptr, &pGpuTextureData)) != S_OK)
            {
                useStagingBuffer = true;
            }
            else
            {
                memcpy(pGpuTextureData, pInitialData, sizeInBytes);
                pGpuTextureResource->Unmap(0u, nullptr);
            }
        }

        if(useStagingBuffer)
        {
            gpu_buffer_t* pStagingBuffer = createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, gpu_buffer_usage_t::storage_buffer, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Staging Texture Buffer");
            if(pStagingBuffer == nullptr)
            {
                return nullptr;
            }
            
            copyGpuTextureFromBuffer(pGraphicsFrame, pGpuTexture, pStagingBuffer);
            freeGpuBuffer(pGraphicsFrame, pStagingBuffer);
        }   
    }

    setD3D12ObjectDebugName(pGpuTexture->resource.pResource, pGpuTexture->pName);
    pGpuTextureResource.takeOwnership();
    return pGpuTexture;
}

void trackMappedGpuBufferForValidation(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer)
{
#if USE_VALIDATION
    pGpuBuffer->pNext = pGraphicsFrame->pFirstMappedGpuBuffer;
    pGraphicsFrame->pFirstMappedGpuBuffer = pGpuBuffer;
#else
    UNUSED_PARAMETER(pGraphicsFrame);
    UNUSED_PARAMETER(pGpuBuffer);
#endif
}

void untrackMappedGpuBufferForValidation(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer)
{
#if USE_VALIDATION
    gpu_buffer_t* pPreviousGpuBuffer = nullptr;
    gpu_buffer_t* pCurrentGpuBuffer = pGraphicsFrame->pFirstMappedGpuBuffer;
    while(true)
    {
        if(pCurrentGpuBuffer == pGpuBuffer)
        {
            if(pPreviousGpuBuffer == nullptr)
            {
                pGraphicsFrame->pFirstMappedGpuBuffer = pGpuBuffer->pNext;
            }
            else
            {
                pPreviousGpuBuffer->pNext = pGpuBuffer->pNext;
            }
        }

        pPreviousGpuBuffer = pCurrentGpuBuffer;
        pCurrentGpuBuffer = pCurrentGpuBuffer->pNext;
    }
#else
    UNUSED_PARAMETER(pGraphicsFrame);
    UNUSED_PARAMETER(pGpuBuffer);
#endif
}

NO_DISCARD void* mapGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer, const uint32_t mapRangeStartOffsetInBytes = 0u, const uint32_t mapRangeEndOffsetInBytes = 0u)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGpuBuffer != nullptr);
    ASSERT_DEBUG((pGpuBuffer->flags & gpu_buffer_flag_t::is_mapped) == 0);
    ASSERT_DEBUG(pGpuBuffer->memoryUsageHint != gpu_memory_usage_hint_t::gpuExclusiveAccess);
    
    if(!Validate(mapRangeStartOffsetInBytes + mapRangeEndOffsetInBytes < pGpuBuffer->sizeInBytes, "Trying to map gpu buffer '%s' out of bounds."))
    {
        return nullptr;
    }

    trackMappedGpuBufferForValidation(pGraphicsFrame, pGpuBuffer);

    D3D12_RANGE mapRange = {};
    mapRange.Begin = mapRangeStartOffsetInBytes;
    mapRange.End = mapRangeEndOffsetInBytes == 0u ? pGpuBuffer->sizeInBytes : mapRangeEndOffsetInBytes;

    void* pMappedData = nullptr;
    if(COM_CALL(pGpuBuffer->resource.pResource->Map(0u, &mapRange, &pMappedData)) != S_OK)
    {
        return nullptr;
    }

    pGpuBuffer->flags |= gpu_buffer_flag_t::is_mapped;
    return pMappedData;
}

void unmapGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGpuBuffer != nullptr);
    ASSERT_DEBUG(pGpuBuffer->flags & gpu_buffer_flag_t::is_mapped);

    untrackMappedGpuBufferForValidation(pGraphicsFrame, pGpuBuffer);

    pGpuBuffer->resource.pResource->Unmap(0u, nullptr);
    pGpuBuffer->flags.clearFlag(gpu_buffer_flag_t::is_mapped);
}

void releaseGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGpuBuffer != nullptr);

    gpu_buffer_t* pPrevGpuBuffer = pGraphicsFrame->pFirstGpuBufferToFree;
    pGpuBuffer->pNext = pPrevGpuBuffer;
    pGraphicsFrame->pFirstGpuBufferToFree = pGpuBuffer;
}

void releaseVertexFormat(graphics_frame_t* pGraphicsFrame, vertex_format_t* pVertexFormat)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pVertexFormat != nullptr);

    vertex_format_t* pPrevVertexFormat = pGraphicsFrame->pFirstVertexFormatToFree;
    pVertexFormat->pNext = pPrevVertexFormat;
    pGraphicsFrame->pFirstVertexFormatToFree = pVertexFormat;
}

void releaseGraphicsPipeline(graphics_frame_t* pGraphicsFrame, graphics_pipeline_t* pGraphicsPipeline)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGraphicsPipeline != nullptr);

    graphics_pipeline_t* pPrevGraphicsPipeline = pGraphicsFrame->pFirstGraphicsPipelineToFree;
    pGraphicsPipeline->pNext = pPrevGraphicsPipeline;
    pGraphicsFrame->pFirstGraphicsPipelineToFree = pGraphicsPipeline;
}

void initializeVertexFormat(graphics_frame_t* pGraphicsFrame, vertex_format_t* pVertexFormat, const vertex_attribute_entry_t* pVertexAttributes, const uint32_t vertexAttributeCount)
{
    //FK: TODO:
    //Validate((checkDoubleVertexAttributes(pVertexAttributes, vertexAttributeCount));
    const uint32_t clampedVertexAttributeCount = GET_MIN(vertexAttributeCount, maxVertexAttributeCount);
    for(uint32_t vertexAttributeIndex = 0u; vertexAttributeIndex < clampedVertexAttributeCount; ++vertexAttributeIndex)
    {
        const uint32_t attributeOffset = pVertexAttributes[vertexAttributeIndex].offsetInBytes;
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].SemanticName         = getVertexAttributeSemanticBaseName(pVertexAttributes[vertexAttributeIndex].attribute);
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].Format               = convertVertexAttributeFormat(&pVertexAttributes[vertexAttributeIndex]);
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].SemanticIndex        = 0u;
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].InputSlot            = 0u;
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].InputSlotClass       = convertVertexAttributeInputFrequency(pVertexAttributes[vertexAttributeIndex].frequency);
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].AlignedByteOffset    = attributeOffset == 0u && vertexAttributeIndex > 0 ? D3D12_APPEND_ALIGNED_ELEMENT : attributeOffset;
        pVertexFormat->pInputElementDescs[vertexAttributeIndex].InstanceDataStepRate = 0u;

        ++pVertexFormat->inputElementCount;
    }
}

NO_DISCARD vertex_format_t* createVertexFormat(graphics_frame_t* pGraphicsFrame, const vertex_attribute_entry_t* pVertexAttributes, const uint32_t vertexAttributeCount)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pVertexAttributes != nullptr);
    ASSERT_DEBUG(vertexAttributeCount > 0u);
    
    hash_map_entry_t<vertex_format_t*> vertexFormatEntry = findOrInsertEntryIntoHashMap(&pGraphicsFrame->pRenderResourceCache->vertexFormats, pVertexAttributes, sizeof(vertex_attribute_entry_t) * vertexAttributeCount);
    if(!vertexFormatEntry.isNew)
    {
        return vertexFormatEntry.value;
    }

    initializeVertexFormat(pGraphicsFrame, vertexFormatEntry.value, pVertexAttributes, vertexAttributeCount);
    return vertexFormatEntry.value;
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

    wchar_t* pTextBuffer = (wchar_t*)allocateFromAllocator(pAllocator, charactersPrinted * sizeof(wchar_t), clear_memory);
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
    uint32_t argumentCount = 6u; //entry point + shader profile
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
    ppArguments[2] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-Zi");
    ppArguments[3] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-Fo shader.pdb");
    ppArguments[4] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-Qembed_debug");
    ppArguments[5] = formatWideStringIntoNewBuffer(pMemoryAllocator, L"-WX");

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
        
        for(uint32_t argumentIndex = 6u; argumentIndex < argumentCount; ++argumentIndex)
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
    addNodesToLinkedList(&pGraphicsFrame->pRenderResourceCache->pFirstFreeShaderBinary, pShaderBinary, 1u);
}

NO_DISCARD shader_binary_t* loadAndCompileShaderCodeFromFile(graphics_frame_t* pGraphicsFrame, const shader_compilation_parameters_t* pParameters)
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

    ComPtr<IDxcResult> pCompileResult = nullptr;

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
        ComPtr<IDxcBlobEncoding> pErrorBufferEncoding = nullptr;
        const HRESULT getErrorBufferResult = COM_CALL(pCompileResult->GetErrorBuffer(&pErrorBufferEncoding));
        if(getErrorBufferResult != S_OK)
        {
            logError("Shader compilation of shader '%s' failed but the error couldn't get retrieved - error: %s.", pParameters->pFilePath, getHResultString(getErrorBufferResult));
            return nullptr;
        }

        if(pErrorBufferEncoding != nullptr && pErrorBufferEncoding->GetBufferSize() > 0)
        {
            logError("Shader compilation of shader '%s' failed because: %s\n", pParameters->pFilePath, (char*)pErrorBufferEncoding->GetBufferPointer());
            return nullptr;
        }
    }

    ComPtr<IDxcBlob> pCompileShaderBlob = nullptr;
    const HRESULT getBlobOutputResult = COM_CALL(pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pCompileShaderBlob), nullptr));
    if(getBlobOutputResult != S_OK)
    {
        logError("Shader compilation of shader '%s' was successful but there's no shader blob. GetOutput() error: %s", pParameters->pFilePath, getHResultString(getBlobOutputResult));
        return nullptr;
    }

    DxcBuffer compiledShaderBuffer = {};
    compiledShaderBuffer.Ptr = pCompileShaderBlob->GetBufferPointer();
    compiledShaderBuffer.Size = pCompileShaderBlob->GetBufferSize();

    ComPtr<ID3D12ShaderReflection> pShaderReflection = nullptr;
    const HRESULT shaderReflectionResult = COM_CALL(pShaderCompilerContext->pUtils->CreateReflection(&compiledShaderBuffer, IID_PPV_ARGS(&pShaderReflection)));
    if(shaderReflectionResult != S_OK)
    {
        logError("Could not reflect shader '%s' - %s", pParameters->pFilePath, getHResultString(shaderReflectionResult));
        return nullptr;
    }

    D3D12_SHADER_DESC shaderDesc = {};
    pShaderReflection->GetDesc(&shaderDesc);

    shader_binding_point_t shaderBindingPoints[maxShaderBindingPoints] = {};

    uint32_t shaderBoundResourceCount = shaderDesc.BoundResources;
    if(shaderBoundResourceCount > maxShaderBindingPoints)
    {
        logWarning("Shader '%s' has more bound resources than %u, some won't be accessible.", pParameters->pFilePath, maxShaderBindingPoints);
        shaderBoundResourceCount = maxShaderBindingPoints;
    }

    for(uint32_t boundResourceIndex = 0u; boundResourceIndex < shaderBoundResourceCount; ++boundResourceIndex)
    {
        D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc = {};
        const HRESULT resourceBindDescResult = COM_CALL(pShaderReflection->GetResourceBindingDesc(boundResourceIndex, &shaderInputBindDesc));
        if(resourceBindDescResult != S_OK)
        {
            logError("Could not reflect shader '%s' bound resource '%u' - %s", pParameters->pFilePath, getHResultString(resourceBindDescResult));
            return nullptr;
        }

        strncpy(shaderBindingPoints[boundResourceIndex].name, shaderInputBindDesc.Name, maxShaderBindingPointNameLength);
        shaderBindingPoints[boundResourceIndex].slot    = rangeCheckCast<uint16_t>(shaderInputBindDesc.BindPoint);
        shaderBindingPoints[boundResourceIndex].space   = rangeCheckCast<uint16_t>(shaderInputBindDesc.Space);
        shaderBindingPoints[boundResourceIndex].type    = mapShaderInputType(shaderInputBindDesc.Type);
    }

    const uint32_t shaderBlobSizeInBytes = rangeCheckCast<uint32_t>(pCompileShaderBlob->GetBufferSize());
    uint8_t* pShaderBlobCopy = (uint8_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, pCompileShaderBlob->GetBufferSize());
    if(pShaderBlobCopy == nullptr)
    {
        logError("Shader compilation of shader '%s' was successful but we ran out of memory trying to copy the shader blob.", pParameters->pFilePath);
        return nullptr;
    }

    memcpy(pShaderBlobCopy, pCompileShaderBlob->GetBufferPointer(), pCompileShaderBlob->GetBufferSize());

    shader_binary_t* pShaderBinary = getFreeShaderBinary(pGraphicsFrame->pRenderResourceCache);
    if(pShaderBinary == nullptr)
    {
        freeFromAllocator(pGraphicsFrame->pMemoryAllocator, pShaderBlobCopy);
        return nullptr;
    }

    memcpy(pShaderBinary->bindingPoints, shaderBindingPoints, sizeof(shaderBindingPoints));
    pShaderBinary->bindingPointCount        = shaderBoundResourceCount;
    pShaderBinary->pShaderBlob              = pShaderBlobCopy;
    pShaderBinary->shaderBlobSizeInBytes    = shaderBlobSizeInBytes;

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
    destroyRenderResourceCache(&pRenderContext->renderResourceCache);
    destroyDescriptorHeap(&pRenderContext->shaderVisibleDescriptorHeap);
    destroyDescriptorHeap(&pRenderContext->samplerDescriptorHeap);

    const bool debugEnabled = (pRenderContext->pDebugLayer != nullptr);
    ID3D12DebugDevice* pDebugDevice = nullptr;
    pRenderContext->pDevice->QueryInterface(IID_PPV_ARGS(&pDebugDevice));

    COM_RELEASE(pRenderContext->pDefaultDirectCommandQueue);
    COM_RELEASE(pRenderContext->pDefaultCopyCommandQueue);
    COM_RELEASE(pRenderContext->pFactory);
    COM_RELEASE(pRenderContext->pDebugLayer);
    COM_RELEASE(pRenderContext->pDevice);

    if(debugEnabled)
    {
        pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
    } 

    COM_RELEASE(pDebugDevice);
    clearMemoryWithZeroes(pRenderContext);
}

void resizeBackBuffer(render_context_t* pRenderContext, const uint32_t width, const uint32_t height)
{
    flushAllFrames(pRenderContext);

    for(uint32_t bufferIndex = 0u; bufferIndex < pRenderContext->swapChain.backBufferCount; ++bufferIndex)
    {
        COM_RELEASE(pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex].resource.pResource);
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

        initializeRenderTarget(&pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex], createUint3(width, height, 0), pFrameBuffer, &descriptorHandle, nullptr, D3D12_RESOURCE_STATE_PRESENT);
    }
}

NO_DISCARD render_context_parameters_t createDefaultRenderContextParameters(HWND pWindowHandle, const uint32_t frameBufferCount, const uint32_t windowWidth, const uint32_t windowHeight, const bool useDebugLayer)
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
    parameters.limits.maxGpuTextureCount                = 32u;
    parameters.limits.maxGpuBufferCount                 = 32u;
    parameters.limits.maxRenderPassCount                = 32u;
    parameters.limits.maxPipelineStateCount             = 32u;
    parameters.limits.maxRenderTargetCount              = 32u;
    parameters.limits.maxShaderBinaryCount              = 32u;
    parameters.limits.maxVertexFormatCount              = 32u;
    parameters.limits.maxSamplerCount                   = 32u;

    return parameters;
}