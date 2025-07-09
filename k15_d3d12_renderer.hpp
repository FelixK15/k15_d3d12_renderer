#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

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

#define KiloByte(x) ((x)*1024u)
#define MegaByte(x) ((x)*1024u*1024u)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct memory_allocator_t;
struct render_context_t;
struct graphics_frame_t;

typedef void*(*allocate_from_memory_allocator_fnc)(memory_allocator_t*, uint64_t, uint64_t);
typedef void(*free_from_memory_allocator_fnc)(memory_allocator_t*, void*);

typedef ID3D12Device10              D3D12DeviceType;
typedef ID3D12Debug6                D3D12DebugType;
typedef IDXGIFactory7               DXGIFactoryType;
typedef IDXGISwapChain4             DXGISwapChainType;
typedef ID3D12GraphicsCommandList7  D3D12GraphicsCommandListType;

typedef uint32_t hash32_t;

constexpr uint64_t  maxVertexAttributeCount         = 16u;
constexpr uint64_t  maxShaderBindingPoints          = 32u;
constexpr uint64_t  maxShaderBindingPointNameLength = 32u;
constexpr uint64_t  maxPipelineBindingPoints        = 32u;
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
    compilation_error,
    reflection_error
};

template<typename T, typename BASE_TYPE>
struct flags_t
{
    flags_t<T, BASE_TYPE>()
    {
        value = (BASE_TYPE)0u;
    }

    flags_t<T, BASE_TYPE>(T flag)
    {
        value = (BASE_TYPE)flag;
    }

    flags_t<T, BASE_TYPE>(int flags)
    {
        value = (BASE_TYPE)flags;
    }

    flags_t<T, BASE_TYPE>(const flags_t<T, BASE_TYPE>& other)
    {
        value = other.value;
    }

    flags_t<T, BASE_TYPE>& operator=(const BASE_TYPE flagsValue)
    {
        value = flagsValue;
        return *this;
    }

    flags_t<T, BASE_TYPE>& operator=(const flags_t<T, BASE_TYPE>& other)
    {
        value = other.value;
        return *this;
    }

    bool isFlagSet(const T flag) const
    {
        return (value & (BASE_TYPE)flag) > 0;
    }

    bool isFlagClear(const T flag) const
    {
        return (value & (BASE_TYPE)flag) == 0;
    }

    void clearFlags()
    {
        value = 0;
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
flags_t<T, BASE_TYPE>& operator|=(flags_t<T, BASE_TYPE>& a, const flags_t<T, BASE_TYPE>& b)
{
    a.value |= b.value;
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

template<typename T, typename BASE_TYPE>
BASE_TYPE operator&(const flags_t<T, BASE_TYPE>& flagsA, const flags_t<T, BASE_TYPE>& flagsB)
{
    return flagsA.value & flagsB.value;
}

template<typename T, typename BASE_TYPE>
bool operator!=(const flags_t<T, BASE_TYPE>& flagsA, const flags_t<T, BASE_TYPE>& flagsB)
{
    return flagsA.value != flagsB.value;
}

template<typename T, typename BASE_TYPE>
bool operator==(const flags_t<T, BASE_TYPE>& flagsA, const flags_t<T, BASE_TYPE>& flagsB)
{
    return flagsA.value == flagsB.value;
}

template<typename T>
using flags8_t = flags_t<T, uint8_t>;

template<typename T>
using flags16_t = flags_t<T, uint16_t>;

template<typename T>
using flags32_t = flags_t<T, uint32_t>;

struct stack_allocator_user_data_t
{
    uint8_t* pStartAddress;
    uint8_t* pCurrentAddress;
    uint8_t* pEndAddress;
};

enum class memory_allocator_type_t : uint8_t
{
    default_allocator,  //backed my malloc & free
    stack_allocator
};

struct memory_allocator_t
{
    memory_allocator_type_t             type;
    allocate_from_memory_allocator_fnc  allocateFnc;
    free_from_memory_allocator_fnc      freeFnc;
    void*                               pUserData;
};

template<typename T>
struct linked_list_node_t
{
    T* pNext;
};

struct descriptor_handle_t
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
};

enum class gpu_resource_state_flag_t : uint32_t
{
    common                  = 0x000001,
    vertex_buffer           = 0x000002,
    constant_buffer         = 0x000004,
    index_buffer            = 0x000008,
    render_target           = 0x000010,
    unordered_access        = 0x000020,
    depth_write             = 0x000040,
    depth_read              = 0x000080,
    stream_out              = 0x000100,
    indirect_argument       = 0x000200,
    transfer_dest           = 0x000400,
    transfer_src            = 0x000800,
    resolve_dest            = 0x001000,
    resolve_src             = 0x002000,
    acceleration_structure  = 0x004000,
    generic_read            = 0x008000,
    present                 = 0x010000,
    video_decode_read       = 0x020000,
    video_decode_write      = 0x040000,
    video_process_read      = 0x080000,
    video_process_write     = 0x100000,
    video_encode_read       = 0x200000,
    video_encode_write      = 0x400000
};

enum class shader_type_t : uint8_t
{
    vertex_shader   = 0x01,
    pixel_shader    = 0x02,
    compute_shader  = 0x04
};

enum class shader_model_t : uint8_t
{
    model_6_0
};

enum class gpu_resource_flag_t : uint8_t
{
    used_in_copy_op = 0x01,
    marked_as_free  = 0x02
};

struct gpu_resource_t
{
    ID3D12Resource*                         pResource;
    flags32_t<gpu_resource_state_flag_t>    currentStateMask;
    flags32_t<gpu_resource_state_flag_t>    futureStateMask;
    flags8_t<shader_type_t>                 currentShaderAccessMask;
    flags8_t<shader_type_t>                 futureShaderAccessMask;
    flags8_t<gpu_resource_flag_t>           flags;
};

struct gpu_resource_barrier_t
{
    gpu_resource_t*                         pGpuResource;
    flags32_t<gpu_resource_state_flag_t>    previousStateMask;
    flags32_t<gpu_resource_state_flag_t>    nextStateMask;
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
    uint3_t                         dimensions;
    gpu_resource_t                  colorTextureResource;
    gpu_resource_t                  depthTextureResource;
    descriptor_handle_t             colorBufferDescriptorHandle;
    descriptor_handle_t             depthBufferDescriptorHandle;
    flags8_t<gpu_resource_flag_t>   resourceFlags;
};

struct shader_compiler_context_t
{
    IDxcUtils*          pUtils;
    IDxcCompiler3*      pShaderCompiler;
    IDxcIncludeHandler* pIncludeHandler;
};

enum class gpu_buffer_flag_t : uint8_t
{
    is_mapped       = 0x01,
};

enum gpu_buffer_usage_flag_t : uint8_t
{
    vertex_buffer   = 0x01,
    index_buffer    = 0x02,
    constant_buffer = 0x04,
    storage_buffer  = 0x08,
    transfer_src    = 0x10,
    transfer_dst    = 0x20
};

enum class gpu_memory_usage_hint_t : uint8_t
{
    cpuReadAccess,              //CPU read is optimized
    cpuWriteGpuReadAccess,      //CPU write + GPU read is optimized
    gpuExclusiveAccess,         //CPU can't read or write
};

enum gpu_texture_usage_flag_t : uint8_t
{
    color_render_target     = 0x01,
    depth_render_target     = 0x02,
    shader_resource_view    = 0x04
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

struct gpu_buffer_t : public linked_list_node_t<gpu_buffer_t>
{
    flags8_t<gpu_buffer_flag_t>         flags;
    flags8_t<gpu_resource_flag_t>       resourceFlags;
    flags8_t<gpu_buffer_usage_flag_t>   bufferUsageMask;
    gpu_memory_usage_hint_t             memoryUsageHint;
    gpu_resource_t                      resource;
    uint32_t                            sizeInBytes;
    const char*                         pName;
};

struct gpu_texture_view_t : public linked_list_node_t<gpu_texture_view_t>
{
    descriptor_handle_t                 shaderResourceView;
    descriptor_handle_t                 renderTargetColorView;
    descriptor_handle_t                 renderTargetDepthView;
    gpu_texture_format_t                format;
    gpu_texture_format_type_t           formatType;
    flags8_t<gpu_texture_usage_flag_t>  usageFlags;
    flags8_t<gpu_resource_flag_t>       resourceFlags;
};

struct gpu_texture_t : public linked_list_node_t<gpu_texture_t>
{
    const char*                         pName;
    gpu_texture_view_t*                 pView;
    gpu_resource_t                      resource;
    uint3_t                             dimensions;
    uint32_t                            sizeInBytes;
    gpu_texture_format_t                format;
    gpu_texture_format_type_t           formatType;
    flags8_t<gpu_resource_flag_t>       resourceFlags;
    flags8_t<gpu_texture_flag_t>        flags;
    flags8_t<gpu_texture_usage_flag_t>  usageFlags;
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
    descriptor_handle_t                 descriptorHandle;
    texture_sampler_filter_type_t       mipmapFilter;
    texture_sampler_filter_type_t       minifactionFilter;
    texture_sampler_filter_type_t       magnificationFilter;
    texture_sampler_address_mode_type_t addressModeU;
    texture_sampler_address_mode_type_t addressModeV;
    texture_sampler_address_mode_type_t addressModeW;
};

struct descriptor_heap_t
{
    const uint8_t* pCPUBaseAddress;
    const uint8_t* pGPUBaseAddress;
    const uint8_t* pCPUEndAddress;
    const uint8_t* pGPUEndAddress;

    descriptor_handle_t* pFreeDescriptorHandles;
    ID3D12DescriptorHeap* pDescriptorHeap;
    uint64_t incrementSizeInBytes;
    uint32_t freeDescriptorCount;
};

struct swap_chain_t
{
    descriptor_heap_t               backBufferRenderTargetDescriptorHeap;
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

enum class resource_binding_type_t : uint8_t
{
    constant_buffer,
    texture,
    sampler,
    structured_buffer
};

struct shader_binding_point_t
{
    char                       name[maxShaderBindingPointNameLength];
    uint16_t                   slot;
    uint16_t                   space;
    resource_binding_type_t    type;
    flags8_t<shader_type_t>    shaderAccessMask;      
};

struct graphics_pipeline_t : public linked_list_node_t<graphics_pipeline_t>
{
    ID3D12PipelineState*            pPipelineState;
    ID3D12RootSignature*            pRootSignature;
    const char*                     pName;
    topology_t                      topology;
    shader_binding_point_t*         pShaderBindingPoints;
    uint32_t                        shaderBindingPointCount;
    uint32_t                        nodeIndex;
    hash32_t                        hash;
    flags8_t<gpu_resource_flag_t>   resourceFlags;
};

struct compute_pipeline_t : public linked_list_node_t<compute_pipeline_t>
{
    ID3D12PipelineState*            pPipelineState;
    ID3D12RootSignature*            pRootSignature;
    const char*                     pName;
    shader_binding_point_t*         pShaderBindingPoints;
    uint32_t                        shaderBindingPointCount;
    uint32_t                        nodeIndex;
    hash32_t                        hash;
    flags8_t<gpu_resource_flag_t>   resourceFlags;
};

struct resource_binding_t
{
    gpu_resource_t*             pResource;
    descriptor_handle_t         descriptorHandle;
    resource_binding_type_t       type;
    uint32_t                    registerIndex;
    uint32_t                    registerSpace;
};

enum pipeline_type_t : uint8_t
{
    graphics_pipeline = 0,
    compute_pipeline,
    count
};

struct pipeline_state_t
{
    bool                       resourcesAreDependingOnCopyPass;
    uint8_t                    resourceBindingCount[pipeline_type_t::count];
    resource_binding_t           boundResources[maxPipelineBindingPoints * pipeline_type_t::count];
    viewport_t                 viewport;
    scissor_t                  scissor;
    render_target_t*           pRenderTarget;
    const graphics_pipeline_t* pGraphicsPipeline;
    const compute_pipeline_t*  pComputePipeline;
};

enum class gpu_pass_type_t : uint8_t
{
    render,
    compute,
    copy,

    count
};

struct gpu_command_buffer_t : public linked_list_node_t<gpu_command_buffer_t>
{
    D3D12GraphicsCommandListType*   pCommandList;
    gpu_pass_type_t                 forPassType;
    bool                            isOpen;
};

struct gpu_command_allocator_t : public linked_list_node_t<gpu_command_allocator_t>
{
    ID3D12CommandAllocator*     pCommandAllocator;
    const gpu_command_buffer_t* pCommandBufferUsingThisAllocator;
    gpu_pass_type_t             forPassType;
    bool                        isInUse;
};

struct render_pass_t : public linked_list_node_t<render_pass_t>
{
    gpu_command_buffer_t*       pGpuCommandBuffer;
    gpu_command_allocator_t*    pGpuCommandAllocator;
    const char*                 pName;
    bool                        isOpen;
    graphics_frame_t*           pGraphicsFrame;
    render_pass_t*              pFirstRenderPassDependency;
    ID3D12Fence*                pEndPassFence;
    gpu_pass_type_t             type;
    pipeline_state_t            currentPipelineState;
    pipeline_state_t            cachedPipelineState;
};

enum class render_pass_execution_order_t : uint8_t
{
    push_back,
    push_front
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
    D3D12_INPUT_ELEMENT_DESC        pInputElementDescs[maxVertexAttributeCount];
    uint32_t                        inputElementCount;
};

struct shader_binary_t : public linked_list_node_t<shader_binary_t>
{
    shader_binding_point_t          bindingPoints[maxShaderBindingPoints];
    const uint8_t*                  pShaderBlob;
    const char*                     pName;
    uint32_t                        shaderBlobSizeInBytes;
    uint32_t                        bindingPointCount;
    flags8_t<gpu_resource_flag_t>   resourceFlags;
};

struct shader_compilation_result_t
{
    shader_binary_t*    pShaderBinary;
    const char*         pErrorMessage;
    const char*         pWarningMessage;
    result_status_t     result;
};
       
struct shader_compilation_parameters_t
{
    const char*     pShaderProfile;
    const char*     pEntryPoint;
    const char*     pDefines;
    const char*     pShaderName;
    shader_type_t   shaderType;
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

enum class gpu_copy_operation_type_t : uint8_t
{
    copy_buffer_to_buffer,
    copy_texture_to_buffer,
    copy_buffer_to_texture,
    copy_texture_to_texture
};

struct gpu_copy_operation_t
{
    gpu_copy_operation_type_t type;

    union
    {
        gpu_buffer_t* pBuffer;
        gpu_texture_t* pTexture;
    } destination;

    union
    {
        gpu_buffer_t* pBuffer;
        gpu_texture_t* pTexture;
    } source;
};

enum render_resource_flags_t : uint8_t
{
    notify_on_array_grow    = 0x01
};

template<typename T>
struct page_allocator_page_entry_t
{
    T*                              pMemory;
    page_allocator_page_entry_t<T>* pNext;
};

template<typename T>
struct page_allocator_t
{
    memory_allocator_t*             pAllocator;
    page_allocator_page_entry_t<T>* pFirstPage;
    uint32_t                        elementCountPerPage;
};

struct render_resource_cache_t
{
    D3D12DeviceType*                                pDevice;
    memory_allocator_t*                             pMemoryAllocator;

    page_allocator_t<gpu_texture_t>                 gpuTextureAllocator;
    page_allocator_t<gpu_texture_view_t>            gpuTextureViewAllocator;
    page_allocator_t<gpu_buffer_t>                  gpuBufferAllocator;
    page_allocator_t<render_pass_t>                 renderPassAllocator;
    page_allocator_t<render_target_t>               renderTargetAllocator;
    page_allocator_t<shader_binary_t>               shaderBinaryAllocator;
    page_allocator_t<texture_sampler_t>             samplerAllocator;
    page_allocator_t<gpu_command_allocator_t>       gpuCommandAllocatorAllocator;
    page_allocator_t<gpu_command_buffer_t>          gpuCommandBufferAllocator;

    hash_map_t<graphics_pipeline_t>                 graphicPipelines;
    hash_map_t<compute_pipeline_t>                  computePipelines;
    hash_map_t<vertex_format_t>                     vertexFormats;

    linked_list_node_t<render_pass_t>*              pFirstFreeRenderPass;
    linked_list_node_t<gpu_buffer_t>*               pFirstFreeGpuBuffer;
    linked_list_node_t<gpu_texture_t>*              pFirstFreeGpuTexture;
    linked_list_node_t<gpu_texture_view_t>*         pFirstFreeGpuTextureView;
    linked_list_node_t<render_target_t>*            pFirstFreeRenderTarget;
    linked_list_node_t<shader_binary_t>*            pFirstFreeShaderBinary;
    linked_list_node_t<texture_sampler_t>*          pFirstFreeSampler;
    linked_list_node_t<gpu_command_allocator_t>**   ppFirstFreeCommandAllocatorPerQueueType;
    linked_list_node_t<gpu_command_buffer_t>**      ppFirstFreeCommandBufferPerQueueType;

    flags8_t<render_resource_flags_t>               flags;
};

struct graphics_frame_t
{
    dynamic_array_t<gpu_copy_operation_t>   gpuCopyOperations;

    render_resource_cache_t*                pRenderResourceCache;
    shader_compiler_context_t*              pShaderCompilerContext;
    render_target_t*                        pBackBuffer;
    memory_allocator_t*                     pMemoryAllocator;
    memory_allocator_t*                     pFrameAllocator;
    render_pass_t*                          pFirstRenderPassToExecute;
    render_pass_t*                          pLastRenderPassAdded;
    gpu_buffer_t*                           pFirstGpuBufferToFree;
    gpu_texture_t*                          pFirstGpuTextureToFree;
    gpu_texture_view_t*                     pFirstGpuTextureViewToFree;
    vertex_format_t*                        pFirstVertexFormatToFree;
    graphics_pipeline_t*                    pFirstGraphicsPipelineToFree;
    compute_pipeline_t*                     pFirstComputePipelineToFree;
    texture_sampler_t*                      pFirstSamplerToFree;
    render_target_t*                        pFirstRenderTargetToFree;
    shader_binary_t*                        pFirstShaderBinaryToFree;
    gpu_command_allocator_t*                pFirstCommandAllocatorToReset;

    descriptor_heap_t*                      pShaderVisibleDescriptorHeap;
    descriptor_heap_t*                      pSamplerDescriptorHeap;
    descriptor_heap_t*                      pRenderTargetColorViewDescriptorHeap;
    descriptor_heap_t*                      pRenderTargetDepthViewDescriptorHeap;

#if USE_VALIDATION
    gpu_buffer_t*                           pFirstMappedGpuBuffer;
#endif

    render_pass_t*                          pFrameCopyPass;

    ID3D12Fence**                           ppFrameFences;
    D3D12DeviceType*                        pDevice;
    ID3D12CommandQueue**                    ppCommandQueues;

    HANDLE                                  pFrameFinishedEvent;
    uint64_t                                frameIndex;
    uint32_t                                openGpuPassCount;
};

struct graphics_frame_collection_t
{
    memory_allocator_t* pMemoryAllocator;
    graphics_frame_t*   pGraphicsFrames;
    uint8_t             frameCount;
};

struct graphics_frame_parameters_t
{
    D3D12DeviceType*            pDevice;
    ID3D12CommandQueue**        ppCommandQueues;
    render_resource_cache_t*    pRenderResourceCache;
    shader_compiler_context_t*  pShaderCompilerContext;
    descriptor_heap_t*          pShaderDescriptorHeap;
    descriptor_heap_t*          pSamplerDescriptorHeap;
    descriptor_heap_t*          pRenderTargetColorViewDescriptorHeap;
    descriptor_heap_t*          pRenderTargetDepthViewDescriptorHeap;
};

struct render_context_t
{
    D3D12DeviceType*            pDevice;
    D3D12DebugType*             pDebugLayer;
    DXGIFactoryType*            pFactory;

    descriptor_heap_t           shaderVisibleDescriptorHeap;
    descriptor_heap_t           samplerDescriptorHeap;
    descriptor_heap_t           renderTargetColorViewDescriptorHeap;
    descriptor_heap_t           renderTargetDepthViewDescriptorHeap;

    shader_compiler_context_t   shaderCompilerContext;
    render_resource_cache_t     renderResourceCache;
    memory_allocator_t          defaultAllocator;
    graphics_frame_collection_t graphicsFramesCollection;
    const graphics_frame_t*     pCurrentGraphicsFrame;

    swap_chain_t                swapChain;
    ID3D12CommandQueue**        ppCommandQueues;

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

    bool operator!=(T* pOtherPointer)
    {
        return pPointer != pOtherPointer;
    }

    bool operator==(T* pOtherPointer)
    {
        return pPointer == pOtherPointer;
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

uint64_t getStringLength(const char* pString)
{
    return strlen(pString);
}

void resetStackAllocator(memory_allocator_t* pAllocator)
{
    ASSERT_DEBUG(pAllocator != nullptr);
    ASSERT_DEBUG(pAllocator->type == memory_allocator_type_t::stack_allocator);

    stack_allocator_user_data_t* pStackAllocatorUserData = (stack_allocator_user_data_t*)pAllocator->pUserData;
    pStackAllocatorUserData->pCurrentAddress = pStackAllocatorUserData->pStartAddress;
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

void* allocateFromStackAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, uint64_t alignmentInBytes)
{
    ASSERT_DEBUG(pAllocator != nullptr);
    ASSERT_DEBUG(pAllocator->type == memory_allocator_type_t::stack_allocator);

    stack_allocator_user_data_t* pAllocatorUserData = (stack_allocator_user_data_t*)pAllocator->pUserData;
    if(pAllocatorUserData->pCurrentAddress + sizeInBytes >= pAllocatorUserData->pEndAddress)
    {
        return nullptr;
    }

    void* pMemory = pAllocatorUserData->pCurrentAddress;
    const uintptr_t alignmentOffBy = (uintptr_t)pMemory % alignmentInBytes;
    if(alignmentOffBy != 0ull)
    {
        const uintptr_t offsetToAdd = alignmentInBytes - alignmentOffBy;
        if(pAllocatorUserData->pCurrentAddress + sizeInBytes + offsetToAdd >= pAllocatorUserData->pEndAddress)
        {
            return nullptr;
        }

        pAllocatorUserData->pCurrentAddress += offsetToAdd;
        pMemory = pAllocatorUserData->pCurrentAddress;
        ASSERT_ALWAYS((uintptr_t)pMemory % alignmentInBytes == 0);
    }

    pAllocatorUserData->pCurrentAddress += sizeInBytes;

    return pMemory;
}

void freeFromStackAllocator(memory_allocator_t* pAllocator, void* pMemory)
{
    //Memory is not free'd individually from the stack allocator
}

void createDefaultMemoryAllocator(memory_allocator_t* pAllocator)
{
    pAllocator->type        = memory_allocator_type_t::default_allocator;
    pAllocator->allocateFnc = allocateFromDefaultAllocator;
    pAllocator->freeFnc     = freeFromDefaultAllocator;
}

void createStackMemoryAllocator(memory_allocator_t* pAllocator, const uint32_t allocatorStackSizeInBytes, void* pStackMemoryBuffer)
{
    stack_allocator_user_data_t* pAllocatorUserData = (stack_allocator_user_data_t*)pStackMemoryBuffer;
    pAllocatorUserData->pStartAddress   = (uint8_t*)pStackMemoryBuffer + sizeof(stack_allocator_user_data_t);
    pAllocatorUserData->pCurrentAddress = pAllocatorUserData->pStartAddress;
    pAllocatorUserData->pEndAddress     = pAllocatorUserData->pStartAddress + allocatorStackSizeInBytes - sizeof(stack_allocator_user_data_t); 

    pAllocator->type        = memory_allocator_type_t::stack_allocator;
    pAllocator->allocateFnc = allocateFromStackAllocator;
    pAllocator->freeFnc     = freeFromStackAllocator;
    pAllocator->pUserData   = pAllocatorUserData;
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

NO_DISCARD void* pushBackFromDynamicArrayDontGrow(base_dynamic_array_t* pArray, const uint32_t count)
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

void addPixBeginMarker(D3D12GraphicsCommandListType* pCommandList, const char* pName)
{
    PIXBeginEvent(pCommandList, PIX_COLOR(100, 100, 100), pName);
}

void addPixEndMarker(D3D12GraphicsCommandListType* pCommandList)
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

void releaseComputePipeline(graphics_frame_t* pGraphicsFrame, compute_pipeline_t* pComputePipeline)
{
    #if 0
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pComputePipeline != nullptr);
    ASSERT_DEBUG(pComputePipeline->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    compute_pipeline_t* pPrevComputePipeline = pGraphicsFrame->pFirstComputePipelineToFree;
    pComputePipeline->pNext = pPrevComputePipeline;
    pGraphicsFrame->pFirstComputePipelineToFree = pComputePipeline;
    pComputePipeline->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
    #endif
}

void releaseGraphicsPipeline(graphics_frame_t* pGraphicsFrame, graphics_pipeline_t* pGraphicsPipeline)
{
    #if 0
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGraphicsPipeline != nullptr);
    ASSERT_DEBUG(pGraphicsPipeline->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    graphics_pipeline_t* pPrevGraphicsPipeline = pGraphicsFrame->pFirstGraphicsPipelineToFree;
    pGraphicsPipeline->pNext = pPrevGraphicsPipeline;
    pGraphicsFrame->pFirstGraphicsPipelineToFree = pGraphicsPipeline;
    pGraphicsPipeline->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
    #endif
}

void releaseShaderBinary(graphics_frame_t* pGraphicsFrame, shader_binary_t* pShaderBinary)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pShaderBinary != nullptr);
    ASSERT_DEBUG(pShaderBinary->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    pShaderBinary->pNext = pGraphicsFrame->pFirstShaderBinaryToFree;
    pGraphicsFrame->pFirstShaderBinaryToFree = pShaderBinary;
    pShaderBinary->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
}

void releaseGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pGpuBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGpuBuffer != nullptr);
    ASSERT_DEBUG(pGpuBuffer->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    pGpuBuffer->pNext = pGraphicsFrame->pFirstGpuBufferToFree;
    pGraphicsFrame->pFirstGpuBufferToFree = pGpuBuffer;
    pGpuBuffer->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
}

void releaseGpuTexture(graphics_frame_t* pGraphicsFrame, gpu_texture_t* pGpuTexture)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGpuTexture != nullptr);
    ASSERT_DEBUG(pGpuTexture->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    pGpuTexture->pNext = pGraphicsFrame->pFirstGpuTextureToFree;
    pGraphicsFrame->pFirstGpuTextureToFree = pGpuTexture;
    pGpuTexture->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
}

void releaseGpuTextureView(graphics_frame_t* pGraphicsFrame, gpu_texture_view_t* pGpuTextureView)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGpuTextureView != nullptr);
    ASSERT_DEBUG(pGpuTextureView->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    pGpuTextureView->pNext = pGraphicsFrame->pFirstGpuTextureViewToFree;
    pGraphicsFrame->pFirstGpuTextureViewToFree = pGpuTextureView;
    pGpuTextureView->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
}

void releaseRenderTarget(graphics_frame_t* pGraphicsFrame, render_target_t* pRenderTarget)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderTarget != nullptr);
    ASSERT_DEBUG(pRenderTarget->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    pRenderTarget->pNext = pGraphicsFrame->pFirstRenderTargetToFree;
    pGraphicsFrame->pFirstRenderTargetToFree = pRenderTarget;
    pRenderTarget->resourceFlags.setFlag(gpu_resource_flag_t::marked_as_free);
}

void resetDescriptorHeap(descriptor_heap_t* pDescriptorHeap)
{
    const uint32_t descriptorCount = (uint32_t)((uint64_t)(pDescriptorHeap->pCPUEndAddress - pDescriptorHeap->pCPUBaseAddress) / pDescriptorHeap->incrementSizeInBytes);
    for(uint32_t descriptorIndex = 0u; descriptorIndex < descriptorCount; ++descriptorIndex)
    {
        pDescriptorHeap->pFreeDescriptorHandles[descriptorIndex].cpuDescriptorHandle.ptr = (size_t)(pDescriptorHeap->pCPUBaseAddress + pDescriptorHeap->incrementSizeInBytes * descriptorIndex);
        if(pDescriptorHeap->pGPUBaseAddress != nullptr)
        {
            pDescriptorHeap->pFreeDescriptorHandles[descriptorIndex].cpuDescriptorHandle.ptr = (UINT64)(pDescriptorHeap->pGPUBaseAddress + pDescriptorHeap->incrementSizeInBytes * descriptorIndex);
        }
    }
    pDescriptorHeap->freeDescriptorCount = descriptorCount;
}

template<typename T>
void mergeLinkedLists(linked_list_node_t<T>** ppLinkedListDestination, linked_list_node_t<T>* pLinkedListSource)
{
    linked_list_node_t<T>* pCurrentNode = pLinkedListSource;
    while(pCurrentNode)
    {
        linked_list_node_t<T>* pNextNode = pCurrentNode->pNext;
        pCurrentNode->pNext = (T*)(*ppLinkedListDestination);
        (*ppLinkedListDestination) = pCurrentNode;

        pCurrentNode = pNextNode;
    }
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

void destroyDescriptorHeap(descriptor_heap_t* pDescriptorHeap)
{
    COM_RELEASE(pDescriptorHeap->pDescriptorHeap);
    clearMemoryWithZeroes(pDescriptorHeap);
}

bool createDescriptorHeap(descriptor_heap_t* pOutDescriptorHeap, memory_allocator_t* pMemoryAllocator, D3D12DeviceType* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, const uint32_t descriptorCount)
{
    descriptor_handle_t* pFreeDescriptorHandles = (descriptor_handle_t*)allocateFromAllocator(pMemoryAllocator, sizeof(descriptor_handle_t) * descriptorCount, alloc_flags_t::clear_memory);
    if(pFreeDescriptorHandles == nullptr)
    {
        return false;
    }

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
    pOutDescriptorHeap->pCPUEndAddress          = (const uint8_t*)cpuDescriptorHeapStartAddress + incrementSizeInBytes * descriptorCount;
    pOutDescriptorHeap->pDescriptorHeap         = pDescriptorHeap;
    pOutDescriptorHeap->pGPUBaseAddress         = nullptr;
    pOutDescriptorHeap->pGPUEndAddress          = nullptr;

    if(flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        const uint64_t gpuDescriptorHeapStartAddress = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        pOutDescriptorHeap->pGPUBaseAddress = (const uint8_t*)gpuDescriptorHeapStartAddress;
        pOutDescriptorHeap->pGPUEndAddress  = (const uint8_t*)gpuDescriptorHeapStartAddress + incrementSizeInBytes * descriptorCount;
    }

    for(uint32_t descriptorIndex = 0u; descriptorIndex < descriptorCount; ++descriptorIndex)
    {
        pFreeDescriptorHandles[descriptorIndex].cpuDescriptorHandle.ptr = (size_t)(pOutDescriptorHeap->pCPUBaseAddress + incrementSizeInBytes * descriptorIndex);
        if(flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            pFreeDescriptorHandles[descriptorIndex].gpuDescriptorHandle.ptr = (UINT64)(pOutDescriptorHeap->pGPUBaseAddress + incrementSizeInBytes * descriptorIndex);
        }
    }

    pOutDescriptorHeap->pFreeDescriptorHandles = pFreeDescriptorHandles;
    pOutDescriptorHeap->freeDescriptorCount = descriptorCount;
    return true;
}

bool allocateDescriptor(descriptor_handle_t* pOutDescriptorHandle, descriptor_heap_t* pDescriptorHeap)
{
    ASSERT_DEBUG(pOutDescriptorHandle != nullptr);
    ASSERT_DEBUG(pDescriptorHeap != nullptr);
    ASSERT_DEBUG(pDescriptorHeap->freeDescriptorCount > 0u);

    const uint32_t descriptorIndex = --pDescriptorHeap->freeDescriptorCount;
    *pOutDescriptorHandle = pDescriptorHeap->pFreeDescriptorHandles[descriptorIndex];
    return true;
}

void freeDescriptor(const descriptor_handle_t* pDescriptorHandle, descriptor_heap_t* pDescriptorHeap)
{
    ASSERT_DEBUG(pDescriptorHandle != nullptr);
    ASSERT_DEBUG(pDescriptorHeap != nullptr);
    
    if(pDescriptorHandle->cpuDescriptorHandle.ptr > 0)
    {
        ASSERT_DEBUG(pDescriptorHandle->cpuDescriptorHandle.ptr >= (size_t)pDescriptorHeap->pCPUBaseAddress && pDescriptorHandle->cpuDescriptorHandle.ptr < (size_t)pDescriptorHeap->pCPUEndAddress);
    }

    if(pDescriptorHandle->gpuDescriptorHandle.ptr > 0)
    {
        ASSERT_DEBUG(pDescriptorHandle->gpuDescriptorHandle.ptr >= (UINT64)pDescriptorHeap->pGPUBaseAddress && pDescriptorHandle->gpuDescriptorHandle.ptr < (UINT64)pDescriptorHeap->pGPUEndAddress);
    }

    const uint32_t freeDescriptorIndex = pDescriptorHeap->freeDescriptorCount++;
    pDescriptorHeap->pFreeDescriptorHandles[freeDescriptorIndex] = *pDescriptorHandle;
}

void freeGpuBufferInternally(gpu_buffer_t* pGpuBuffer)
{
    ASSERT_DEBUG(pGpuBuffer->resourceFlags.isFlagSet(gpu_resource_flag_t::marked_as_free));
    COM_RELEASE(pGpuBuffer->resource.pResource);
    pGpuBuffer->resourceFlags.clearFlag(gpu_resource_flag_t::marked_as_free);
}

void freeGpuTextureInternally(gpu_texture_t* pGpuTexture, graphics_frame_t* pGraphicsFrame)
{
    ASSERT_DEBUG(pGpuTexture->resourceFlags.isFlagSet(gpu_resource_flag_t::marked_as_free));

    COM_RELEASE(pGpuTexture->resource.pResource);
    pGpuTexture->resourceFlags.clearFlag(gpu_resource_flag_t::marked_as_free);

    if(pGpuTexture->pView != nullptr)
    {
        releaseGpuTextureView(pGraphicsFrame, pGpuTexture->pView);
    }
}

void freeGpuTextureViewInternally(gpu_texture_view_t* pGpuTextureView, graphics_frame_t* pGraphicsFrame)
{
    ASSERT_DEBUG(pGpuTextureView->resourceFlags.isFlagSet(gpu_resource_flag_t::marked_as_free));

    if(pGpuTextureView->renderTargetColorView.cpuDescriptorHandle.ptr != 0)
    {
        freeDescriptor(&pGpuTextureView->renderTargetColorView, pGraphicsFrame->pRenderTargetColorViewDescriptorHeap);
    }

    if(pGpuTextureView->renderTargetDepthView.cpuDescriptorHandle.ptr != 0)
    {
        freeDescriptor(&pGpuTextureView->renderTargetDepthView, pGraphicsFrame->pRenderTargetDepthViewDescriptorHeap);
    }

    if(pGpuTextureView->shaderResourceView.gpuDescriptorHandle.ptr != 0)
    {
        freeDescriptor(&pGpuTextureView->shaderResourceView, pGraphicsFrame->pShaderVisibleDescriptorHeap);
    }

    pGpuTextureView->resourceFlags.clearFlag(gpu_resource_flag_t::marked_as_free);
}

void freeGraphicsPipelineInternally(graphics_pipeline_t* pGraphicsPipeline, graphics_frame_t* pGraphicsFrame)
{
    //TODO: Fix pipeline free - don't hash?
    #if 0
    COM_RELEASE(pGraphicsPipeline->pPipelineState);
    COM_RELEASE(pGraphicsPipeline->pRootSignature);
    ZeroMemory(pGraphicsPipeline, sizeof(graphics_pipeline_t));

    hash_map_entry_t<graphics_pipeline_t*> pipelineHashEntry = {};
    pipelineHashEntry.hash = pGraphicsPipeline->hash;
    pipelineHashEntry.nodeIndex = pGraphicsPipeline->nodeIndex;

    removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineHashEntry);
    #endif
}

void freeComputePipelineInternally(compute_pipeline_t* pComputePipelineStateToFree, graphics_frame_t* pGraphicsFrame)
{
    //TODO: Fix pipeline free - don't hash?
    #if 0
    COM_RELEASE(pComputePipelineStateToFree->pPipelineState);
    COM_RELEASE(pComputePipelineStateToFree->pRootSignature);
    ZeroMemory(pComputePipelineStateToFree, sizeof(compute_pipeline_t));

    hash_map_entry_t<compute_pipeline_t*> pipelineHashEntry = {};
    pipelineHashEntry.hash = pComputePipelineStateToFree->hash;
    pipelineHashEntry.nodeIndex = pComputePipelineStateToFree->nodeIndex;

    removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->computePipelines, &pipelineHashEntry);
    #endif
}

void freeRenderTargetInternally(render_target_t* pRenderTarget, graphics_frame_t* pGraphicsFrame)
{
    ASSERT_DEBUG(pRenderTarget->resourceFlags.isFlagSet(gpu_resource_flag_t::marked_as_free));

    if(pRenderTarget->colorBufferDescriptorHandle.cpuDescriptorHandle.ptr != 0)
    {
        freeDescriptor(&pRenderTarget->colorBufferDescriptorHandle, pGraphicsFrame->pRenderTargetColorViewDescriptorHeap);
    }

    if(pRenderTarget->depthBufferDescriptorHandle.cpuDescriptorHandle.ptr != 0)
    {
        freeDescriptor(&pRenderTarget->depthBufferDescriptorHandle, pGraphicsFrame->pRenderTargetDepthViewDescriptorHeap);
    }

    pRenderTarget->resourceFlags.clearFlag(gpu_resource_flag_t::marked_as_free);
}

void freeShaderBinaryInternally(shader_binary_t* pShaderBinary, graphics_frame_t* pGraphicsFrame)
{
    ASSERT_DEBUG(pShaderBinary->resourceFlags.isFlagSet(gpu_resource_flag_t::marked_as_free));

    if(pShaderBinary->pShaderBlob)
    {
        freeFromAllocator(pGraphicsFrame->pMemoryAllocator, (void*)pShaderBinary->pShaderBlob);
        pShaderBinary->pShaderBlob = nullptr;
        pShaderBinary->shaderBlobSizeInBytes = 0u;
    }

    pShaderBinary->resourceFlags.clearFlag(gpu_resource_flag_t::marked_as_free);

}

void freePendingFrameResources(graphics_frame_t* pGraphicsFrame)
{
    if(pGraphicsFrame->pFirstGpuBufferToFree != nullptr)
    {
        gpu_buffer_t* pGpuBufferToFree = pGraphicsFrame->pFirstGpuBufferToFree;
        while(pGpuBufferToFree != nullptr)
        {
            gpu_buffer_t* pNextGpuBuffer = pGpuBufferToFree->pNext;
            freeGpuBufferInternally(pGpuBufferToFree);
            pGpuBufferToFree = pNextGpuBuffer;
        }

        mergeLinkedLists(&pGraphicsFrame->pRenderResourceCache->pFirstFreeGpuBuffer, pGraphicsFrame->pFirstGpuBufferToFree);
        pGraphicsFrame->pFirstGpuBufferToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstGpuTextureToFree != nullptr)
    {
        gpu_texture_t* pGpuTextureToFree = pGraphicsFrame->pFirstGpuTextureToFree;
        while(pGpuTextureToFree != nullptr)
        {
            gpu_texture_t* pNextGpuTexture = (gpu_texture_t*)pGpuTextureToFree->pNext;
            freeGpuTextureInternally(pGpuTextureToFree, pGraphicsFrame);
            pGpuTextureToFree = pNextGpuTexture;
        }

        mergeLinkedLists(&pGraphicsFrame->pRenderResourceCache->pFirstFreeGpuTexture, pGraphicsFrame->pFirstGpuTextureToFree);
        pGraphicsFrame->pFirstGpuTextureToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstGpuTextureViewToFree != nullptr)
    {
        gpu_texture_view_t* pGpuTextureViewToFree = pGraphicsFrame->pFirstGpuTextureViewToFree;
        while(pGpuTextureViewToFree != nullptr)
        {
            gpu_texture_view_t* pNextGpuTextureView = (gpu_texture_view_t*)pGpuTextureViewToFree->pNext;
            freeGpuTextureViewInternally(pGpuTextureViewToFree, pGraphicsFrame);
            pGpuTextureViewToFree = pNextGpuTextureView;
        }

        mergeLinkedLists(&pGraphicsFrame->pRenderResourceCache->pFirstFreeGpuTextureView, pGraphicsFrame->pFirstGpuTextureViewToFree);
        pGraphicsFrame->pFirstGpuTextureViewToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstGraphicsPipelineToFree != nullptr)
    {
        graphics_pipeline_t* pGraphicsPipelineStateToFree = pGraphicsFrame->pFirstGraphicsPipelineToFree;
        while(pGraphicsPipelineStateToFree != nullptr)
        {
            graphics_pipeline_t* pNextGraphicsPipelineStateToFree = (graphics_pipeline_t*)pGraphicsPipelineStateToFree->pNext;
            freeGraphicsPipelineInternally(pGraphicsPipelineStateToFree, pGraphicsFrame);
            pGraphicsPipelineStateToFree = pNextGraphicsPipelineStateToFree;
        }

        pGraphicsFrame->pFirstGraphicsPipelineToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstComputePipelineToFree != nullptr)
    {
        compute_pipeline_t* pComputePipelineStateToFree = pGraphicsFrame->pFirstComputePipelineToFree;
        while(pComputePipelineStateToFree != nullptr)
        {
            compute_pipeline_t* pNextComputePipelineStateToFree = (compute_pipeline_t*)pComputePipelineStateToFree->pNext;
            freeComputePipelineInternally(pComputePipelineStateToFree, pGraphicsFrame);
            pComputePipelineStateToFree = pNextComputePipelineStateToFree;
        }

        pGraphicsFrame->pFirstComputePipelineToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstRenderTargetToFree != nullptr)
    {
        render_target_t* pRenderTargetToFree = pGraphicsFrame->pFirstRenderTargetToFree;
        while(pRenderTargetToFree != nullptr)
        {
            render_target_t* pNextRenderTargetToFree = pRenderTargetToFree->pNext;
            freeRenderTargetInternally(pRenderTargetToFree, pGraphicsFrame);
            pRenderTargetToFree = pNextRenderTargetToFree;
        }

        mergeLinkedLists(&pGraphicsFrame->pRenderResourceCache->pFirstFreeRenderTarget, pGraphicsFrame->pFirstRenderTargetToFree);
        pGraphicsFrame->pFirstRenderTargetToFree = nullptr;
    }

    if(pGraphicsFrame->pFirstShaderBinaryToFree != nullptr)
    {
        shader_binary_t* pShaderBinaryToFree = pGraphicsFrame->pFirstShaderBinaryToFree;
        while(pShaderBinaryToFree != nullptr)
        {
            shader_binary_t* pNextShaderBinaryToFree = pShaderBinaryToFree->pNext;
            freeShaderBinaryInternally(pShaderBinaryToFree, pGraphicsFrame);
            pShaderBinaryToFree = pNextShaderBinaryToFree;
        }

        mergeLinkedLists(&pGraphicsFrame->pRenderResourceCache->pFirstFreeShaderBinary, pGraphicsFrame->pFirstShaderBinaryToFree);
        pGraphicsFrame->pFirstShaderBinaryToFree = nullptr;
    }
}

void flushFrame(graphics_frame_t* pGraphicsFrame)
{
    const DWORD waitResult = WaitForSingleObject(pGraphicsFrame->pFrameFinishedEvent, INFINITE);
    ASSERT_DEBUG(waitResult == WAIT_OBJECT_0);
}

void resetPipelineState(pipeline_state_t* pRenderState)
{
    memset(pRenderState, 0u, sizeof(pipeline_state_t));
}

void resetCommandAllocatorList(gpu_command_allocator_t* pFirstCommandAllocator)
{
    gpu_command_allocator_t* pCurrentCommandAllocator = pFirstCommandAllocator;
    while(pCurrentCommandAllocator != nullptr)
    {
        COM_CALL(pCurrentCommandAllocator->pCommandAllocator->Reset());
        pCurrentCommandAllocator = pCurrentCommandAllocator->pNext;
    }
}

void freeCommandAllocatorList(render_resource_cache_t* pRenderResourceCache, gpu_command_allocator_t* pFirstCommandAllocator)
{
    gpu_command_allocator_t* pCurrentCommandAllocator = pFirstCommandAllocator;
    gpu_command_allocator_t* pNextCommandAllocator = nullptr;
    while(pCurrentCommandAllocator != nullptr)
    {
        pNextCommandAllocator = pCurrentCommandAllocator->pNext;

        const uint32_t queueIndex = (uint32_t)pCurrentCommandAllocator->forPassType;
        addNodesToLinkedList(&pRenderResourceCache->ppFirstFreeCommandAllocatorPerQueueType[queueIndex], pCurrentCommandAllocator, 1u);
        pCurrentCommandAllocator = pNextCommandAllocator;
    }
}

void resetFrame(graphics_frame_t* pGraphicsFrame)
{
    Validate(pGraphicsFrame->pFirstGpuBufferToFree == nullptr, "There are pending free gpu buffer, don't forget to call 'freePendingFrameResources()'.");
    Validate(pGraphicsFrame->pFirstGraphicsPipelineToFree == nullptr, "There are pending free graphics pipelines, don't forget to call 'freePendingFrameResources()'.");

    render_pass_t* pRenderPass = pGraphicsFrame->pFirstRenderPassToExecute;
    while(pRenderPass != nullptr)
    {
        const uint32_t queueIndex = (uint32_t)pRenderPass->pGpuCommandBuffer->forPassType;
        addNodesToLinkedList(&pGraphicsFrame->pRenderResourceCache->ppFirstFreeCommandBufferPerQueueType[queueIndex], pRenderPass->pGpuCommandBuffer, 1u);
        pRenderPass = pRenderPass->pNext;
    }

    linked_list_node_t<render_pass_t>* pFirstRenderPassToFree = (linked_list_node_t<render_pass_t>*)pGraphicsFrame->pFirstRenderPassToExecute;
    mergeLinkedLists(&pGraphicsFrame->pRenderResourceCache->pFirstFreeRenderPass, pFirstRenderPassToFree);
    pGraphicsFrame->pFirstRenderPassToExecute = nullptr;
    pGraphicsFrame->pLastRenderPassAdded = nullptr;

    resetCommandAllocatorList(pGraphicsFrame->pFirstCommandAllocatorToReset);
    freeCommandAllocatorList(pGraphicsFrame->pRenderResourceCache, pGraphicsFrame->pFirstCommandAllocatorToReset);
    pGraphicsFrame->pFirstCommandAllocatorToReset = nullptr;

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

void destroySwapChain(swap_chain_t* pSwapChain)
{
    COM_RELEASE(pSwapChain->pSwapChain);
    for(uint32_t bufferIndex = 0u; bufferIndex < pSwapChain->backBufferCount; ++bufferIndex)
    {
        COM_RELEASE(pSwapChain->pBackBufferRenderTargets[bufferIndex].colorTextureResource.pResource);
    }

    freeFromAllocator(pSwapChain->pMemoryAllocator, pSwapChain->pBackBufferRenderTargets);
    destroyDescriptorHeap(&pSwapChain->backBufferRenderTargetDescriptorHeap);
}

bool createSwapChain(swap_chain_t* pOutSwapChain, memory_allocator_t* pMemoryAllocator, IDXGIFactory6* pFactory, D3D12DeviceType* pDevice, ID3D12CommandQueue* pCommandQueue, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, const uint32_t frameBufferCount)
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
    
    swap_chain_t swapChain = {};
    swapChain.backBufferCount = frameBufferCount;
    swapChain.pMemoryAllocator = pMemoryAllocator;

    com_auto_release_t<IDXGISwapChain1> pTempSwapChain = nullptr;
    
    if(!createDescriptorHeap(&swapChain.backBufferRenderTargetDescriptorHeap, pMemoryAllocator, pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, frameBufferCount))
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
        descriptor_handle_t frameBufferHandle = {};
        allocateDescriptor(&frameBufferHandle, &swapChain.backBufferRenderTargetDescriptorHeap);

        pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, frameBufferHandle.cpuDescriptorHandle);

        clearMemoryWithZeroes(&swapChain.pBackBufferRenderTargets[bufferIndex]);
        swapChain.pBackBufferRenderTargets[bufferIndex].colorTextureResource.pResource = pFrameBuffer;
        swapChain.pBackBufferRenderTargets[bufferIndex].colorTextureResource.currentStateMask = gpu_resource_state_flag_t::present;
        swapChain.pBackBufferRenderTargets[bufferIndex].colorBufferDescriptorHandle = frameBufferHandle;
        swapChain.pBackBufferRenderTargets[bufferIndex].dimensions.x = windowWidth;
        swapChain.pBackBufferRenderTargets[bufferIndex].dimensions.y = windowHeight;
        swapChain.pBackBufferRenderTargets[bufferIndex].dimensions.z = 1u;
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

bool createFence(D3D12DeviceType* pDevice, ID3D12Fence** pOutFence, const uint32_t initialValue)
{
    if(COM_CALL(pDevice->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(pOutFence))) != S_OK)
    {
        return false;
    }

    return true;
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
        case resource_binding_type_t::constant_buffer:
            return D3D12_ROOT_PARAMETER_TYPE_CBV;
        case resource_binding_type_t::texture:
        case resource_binding_type_t::sampler:
            return D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        case resource_binding_type_t::structured_buffer:
            return D3D12_ROOT_PARAMETER_TYPE_UAV;
    }

    DebugBreak();
    return D3D12_ROOT_PARAMETER_TYPE_CBV;
}

D3D12_DESCRIPTOR_RANGE_TYPE mapBindingPointTypeToDescriptorRangeType(const resource_binding_type_t bindingPointType)
{
    switch(bindingPointType)
    {
        case resource_binding_type_t::constant_buffer:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case resource_binding_type_t::sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case resource_binding_type_t::texture:
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
    for(uint32_t shaderBindingPointIndex = 0u; shaderBindingPointIndex < pShaderBinary->bindingPointCount; ++shaderBindingPointIndex)
    {
        const shader_binding_point_t* pShaderBindingPoint = pShaderBinary->bindingPoints + shaderBindingPointIndex;

        bool alreadyCollected = false;
        for(uint32_t collectedBindingPointIndex = 0; collectedBindingPointIndex < bindingPointsIndex; ++collectedBindingPointIndex)
        {
            shader_binding_point_t* pCollectedBindingPoint = pBindingPointsToFill + collectedBindingPointIndex;
            if(strncmp(pShaderBindingPoint->name, pCollectedBindingPoint->name, sizeof(shader_binding_point_t::name)) != 0)
            {
                continue;
            }

            if(pShaderBindingPoint->slot != pCollectedBindingPoint->slot || pShaderBindingPoint->space != pCollectedBindingPoint->space || pShaderBindingPoint->type != pCollectedBindingPoint->type)
            {
                continue;
            }

            pCollectedBindingPoint->shaderAccessMask |= pShaderBindingPoint->shaderAccessMask;
            alreadyCollected = true;
            break;
        }

        if(alreadyCollected)
        {
            continue;
        }

        memcpy(pBindingPointsToFill + *pBindingPointsIndex, pShaderBindingPoint, sizeof(shader_binding_point_t));
        *pBindingPointsIndex += 1u;
    }
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

NO_DISCARD compute_pipeline_t* createComputePipeline(graphics_frame_t* pGraphicsFrame, const shader_binary_t* pComputeShader, const char* pName)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pComputeShader != nullptr);

    hash_map_entry_t<compute_pipeline_t*> pipelineState = findOrInsertEntryIntoHashMap(&pGraphicsFrame->pRenderResourceCache->computePipelines, pComputeShader, sizeof(pComputeShader));
    if(!pipelineState.isNew)
    {
        return pipelineState.value;
    }

    compute_pipeline_t* pPipelineState = pipelineState.value;

    shader_binding_point_t* pShaderBindingPoints = nullptr;
    if( pComputeShader->bindingPointCount > 0u )
    {
        pShaderBindingPoints = (shader_binding_point_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, sizeof(shader_binding_point_t) * pComputeShader->bindingPointCount);
        if(pShaderBindingPoints == nullptr)
        {
            removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->computePipelines, &pipelineState);
            return nullptr;
        }

        memcpy(pShaderBindingPoints, pComputeShader->bindingPoints, sizeof(shader_binding_point_t) * pComputeShader->bindingPointCount);
    }

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    if(!fillRootSignature(pGraphicsFrame->pFrameAllocator, &rootSignatureDesc, pShaderBindingPoints, pComputeShader->bindingPointCount))
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->computePipelines, &pipelineState);
        return nullptr;
    }

    com_auto_release_t<ID3DBlob> pRootSignatureBlob = nullptr;
    com_auto_release_t<ID3DBlob> pErrorBlob = nullptr;
    if(COM_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pRootSignatureBlob, &pErrorBlob)) != S_OK)
    {
        if(pErrorBlob != nullptr)
        {
            const char* pError = (const char*)pErrorBlob->GetBufferPointer();
            logError(pError);
        }

        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->computePipelines, &pipelineState);
        return nullptr;
    }

    ID3D12RootSignature* pRootSignature = nullptr;
    COM_CALL(pGraphicsFrame->pDevice->CreateRootSignature(0u, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
    computeDesc.CS.pShaderBytecode = pComputeShader->pShaderBlob;
    computeDesc.CS.BytecodeLength = pComputeShader->shaderBlobSizeInBytes;
    computeDesc.pRootSignature = pRootSignature;

    com_auto_release_t<ID3D12PipelineState> pComputePipeline = nullptr;
    if(COM_CALL(pGraphicsFrame->pDevice->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&pComputePipeline))) != S_OK)
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->computePipelines, &pipelineState);
        logError("Error while trying to create compute pipeline state '%s'.", pName);
        return nullptr;
    }

    setD3D12ObjectDebugName(pComputePipeline.pPointer, pName);
    
    pPipelineState->pPipelineState          = pComputePipeline.pPointer;
    pPipelineState->pRootSignature          = pRootSignature;
    pPipelineState->pShaderBindingPoints    = pShaderBindingPoints;
    pPipelineState->shaderBindingPointCount = pComputeShader->bindingPointCount;
    pComputePipeline.takeOwnership();

    return pPipelineState;
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
    if(!fillRootSignature(pGraphicsFrame->pFrameAllocator, &rootSignatureDesc, pShaderBindingPoints, bindingPointCount))
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineState);
        return nullptr;
    }

    com_auto_release_t<ID3DBlob> pRootSignatureBlob = nullptr;
    com_auto_release_t<ID3DBlob> pErrorBlob = nullptr;
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

    com_auto_release_t<ID3D12PipelineState> pPipelineStateObject = nullptr;
    if(COM_CALL(pGraphicsFrame->pDevice->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pPipelineStateObject))) != S_OK)
    {
        removeEntryFromHashMap(&pGraphicsFrame->pRenderResourceCache->graphicPipelines, &pipelineState);
        logError("Error while trying to create graphics pipeline state '%s'.",  pPipelineParameters->pName);
        return nullptr;
    }

    setD3D12ObjectDebugName(pPipelineStateObject.pPointer, pPipelineParameters->pName);
    
    pPipelineState->pPipelineState          = pPipelineStateObject.pPointer;
    pPipelineState->pRootSignature          = pRootSignature;
    pPipelineState->topology                = pPipelineParameters->topology;
    pPipelineState->pShaderBindingPoints    = pShaderBindingPoints;
    pPipelineState->shaderBindingPointCount = bindingPointCount;
    pPipelineState->nodeIndex               = pipelineState.nodeIndex;
    pPipelineState->hash                    = pipelineState.hash;
    pPipelineStateObject.takeOwnership();
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

    const uint32_t fenceCount = (uint32_t)gpu_pass_type_t::count;
    for(uint32_t fenceIndex = 0u; fenceIndex < fenceCount; ++fenceIndex)
    {
        COM_RELEASE(pGraphicsFrame->ppFrameFences[fenceIndex]);   
    }

    destroyDynamicArray(&pGraphicsFrame->gpuCopyOperations);
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

bool createGraphicsFrame(graphics_frame_t* pOutGraphicFrame, memory_allocator_t* pMemoryAllocator, const graphics_frame_parameters_t* pGraphicsFrameParameters)
{
    ASSERT_DEBUG(pGraphicsFrameParameters != nullptr);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->pDevice != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->ppCommandQueues != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->pShaderCompilerContext != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->pRenderResourceCache != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->ppCommandQueues[(uint32_t)gpu_pass_type_t::render] != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->ppCommandQueues[(uint32_t)gpu_pass_type_t::compute] != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->ppCommandQueues[(uint32_t)gpu_pass_type_t::copy] != nullptr);

    ID3D12Fence** ppFences = (ID3D12Fence**)allocateFromAllocator(pMemoryAllocator, sizeof(void*));
    if(ppFences == nullptr)
    {
        return false;
    }

    graphics_frame_t graphicsFrame = {0};
    graphicsFrame.ppFrameFences = ppFences;
    graphicsFrame.pMemoryAllocator = pMemoryAllocator;
    graphicsFrame.pFrameFinishedEvent = CreateEvent(nullptr, TRUE, TRUE, "");
    graphicsFrame.pShaderCompilerContext = pGraphicsFrameParameters->pShaderCompilerContext;
    graphicsFrame.pRenderResourceCache = pGraphicsFrameParameters->pRenderResourceCache;
    graphicsFrame.ppCommandQueues = pGraphicsFrameParameters->ppCommandQueues;
    graphicsFrame.pShaderVisibleDescriptorHeap = pGraphicsFrameParameters->pShaderDescriptorHeap;
    graphicsFrame.pSamplerDescriptorHeap = pGraphicsFrameParameters->pSamplerDescriptorHeap;
    graphicsFrame.pRenderTargetColorViewDescriptorHeap = pGraphicsFrameParameters->pRenderTargetColorViewDescriptorHeap;
    graphicsFrame.pRenderTargetDepthViewDescriptorHeap = pGraphicsFrameParameters->pRenderTargetDepthViewDescriptorHeap;
    graphicsFrame.pDevice = pGraphicsFrameParameters->pDevice;

    if(graphicsFrame.pFrameFinishedEvent == nullptr)
    {
        return false;
    }

    graphicsFrame.pFrameAllocator = (memory_allocator_t*)allocateFromAllocator(pMemoryAllocator, sizeof(memory_allocator_t), alloc_flags_t::clear_memory);
    if(graphicsFrame.pFrameAllocator == nullptr)
    {
        return false;
    }

    void* pFrameAllocatorMemory = allocateFromAllocator(pMemoryAllocator, MegaByte(1), alloc_flags_t::clear_memory);
    if(pFrameAllocatorMemory == nullptr)
    {
        return false;
    }

    createStackMemoryAllocator(graphicsFrame.pFrameAllocator, MegaByte(1), pFrameAllocatorMemory);


    const uint32_t queueCount = (uint32_t)gpu_pass_type_t::count;
    for(uint32_t queueIndex = 0u; queueIndex < queueCount; ++queueIndex)
    {
        if(!createFence(graphicsFrame.pDevice, &graphicsFrame.ppFrameFences[queueIndex], 0))
        {
            goto cleanup_and_exit_failure;
        }
    }

    if(!createDynamicArray<gpu_copy_operation_t>(&graphicsFrame.gpuCopyOperations, pMemoryAllocator, 64u, alloc_flags_t::clear_memory))
    {
        goto cleanup_and_exit_failure;
    }

    *pOutGraphicFrame = graphicsFrame;
    return true;

    cleanup_and_exit_failure:
        destroyGraphicsFrame(&graphicsFrame);
        return false;
}

bool createGraphicsFrameCollection(graphics_frame_collection_t* pOutGraphicFrameCollection, memory_allocator_t* pMemoryAllocator, const graphics_frame_parameters_t* pGraphicsFrameParameters,  const uint8_t frameCount)
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
        if(!createGraphicsFrame(&graphicFrameCollection.pGraphicsFrames[frameIndex], pMemoryAllocator, pGraphicsFrameParameters))
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
        uint32_t                        maxGpuTextureViewCount;
        uint32_t                        maxGpuBufferCount;
        uint32_t                        maxVertexFormatCount;
        uint32_t                        maxShaderBinaryCount;
        uint32_t                        maxRenderPassCount;
        uint32_t                        maxComputePassCount;
        uint32_t                        maxCopyPassCount;
        uint32_t                        maxRenderTargetCount;
        uint32_t                        maxGraphicsPipelineCount;
        uint32_t                        maxComputePipelineCount;
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

template<typename T>
page_allocator_page_entry_t<T>* allocatePageEntryForPageAllocator(memory_allocator_t* pAllocator, const uint32_t pageElementCount, const alloc_flags_t flags)
{
    const uint32_t pageSizeInBytes = pageElementCount * sizeof(T) + sizeof(page_allocator_page_entry_t<T>);
    void* pPageMemory = allocateFromAllocator(pAllocator, pageSizeInBytes, flags);
    if(pPageMemory == nullptr)
    {
        return nullptr;
    }

    page_allocator_page_entry_t<T>* pPageEntry = (page_allocator_page_entry_t<T>*)pPageMemory;
    pPageEntry->pMemory = (T*)(pPageEntry + 1);
    return pPageEntry;
}

template<typename T>
bool createPageAllocator(page_allocator_t<T>* pOutAllocator, memory_allocator_t* pAllocator, const uint32_t pageElementCount, const alloc_flags_t flags)
{
    ASSERT_DEBUG(pOutAllocator != nullptr);
    ASSERT_DEBUG(pAllocator != nullptr);
    ASSERT_DEBUG(pageElementCount != 0);

    page_allocator_page_entry_t<T>* pPageEntry = allocatePageEntryForPageAllocator<T>(pAllocator, pageElementCount, flags);
    if(pPageEntry == nullptr)
    {
        return false;
    }

    pOutAllocator->pFirstPage = pPageEntry;
    pOutAllocator->pAllocator = pAllocator;
    pOutAllocator->elementCountPerPage = pageElementCount;
    return true;
}

template<typename T>
void destroyPageAllocator(page_allocator_t<T>* pPageAllocator)
{
    page_allocator_page_entry_t<T>* pAllocatorPage = pPageAllocator->pFirstPage;
    while(pAllocatorPage != nullptr)
    {
        page_allocator_page_entry_t<T>* pNextPage = pAllocatorPage->pNext;
        freeFromDefaultAllocator(pPageAllocator->pAllocator, pAllocatorPage);
        pAllocatorPage = pNextPage;
    }
}

template<typename T>
T* allocateNewPageWorthOfObjects(page_allocator_t<T>* pPageAllocator, const alloc_flags_t allocFlags)
{
    page_allocator_page_entry_t<T>* pPageAllocatorEntry = allocatePageEntryForPageAllocator<T>(pPageAllocator->pAllocator, pPageAllocator->elementCountPerPage, allocFlags);
    if(pPageAllocatorEntry == nullptr)
    {
        return nullptr;
    }

    pPageAllocatorEntry->pNext = pPageAllocator->pFirstPage;
    pPageAllocator->pFirstPage = pPageAllocatorEntry;
    return pPageAllocatorEntry->pMemory;
}

void destroyRenderResourceCache(render_resource_cache_t* pRenderResourceCache)
{
    //Validate(areAllGpuBuffersReleased(&pRenderResourceCache->gpuBufferAllocator.), "Leaked GPU buffers detected.");
    //Validate((areAllGraphicPipelinesReleased(&pRenderResourceCache->graphicPipelines), "Leaked graphics pipeline states.");
    //TODO: More validation

    {
        page_allocator_page_entry_t<gpu_command_buffer_t>* pCommandBufferAllocatorPage = pRenderResourceCache->gpuCommandBufferAllocator.pFirstPage;
        while(pCommandBufferAllocatorPage != nullptr)
        {
            for(uint32_t i = 0u; i < pRenderResourceCache->gpuCommandBufferAllocator.elementCountPerPage; ++i)
            {
                COM_RELEASE(pCommandBufferAllocatorPage->pMemory[i].pCommandList);
            }

            pCommandBufferAllocatorPage = pCommandBufferAllocatorPage->pNext;
        }
    }
    
    {
        page_allocator_page_entry_t<gpu_command_allocator_t>* pCommandAllocatorAllocatorPage = pRenderResourceCache->gpuCommandAllocatorAllocator.pFirstPage;
        while(pCommandAllocatorAllocatorPage != nullptr)
        {
            for(uint32_t i = 0u; i < pRenderResourceCache->gpuCommandAllocatorAllocator.elementCountPerPage; ++i)
            {
                COM_RELEASE(pCommandAllocatorAllocatorPage->pMemory[i].pCommandAllocator);
            }

            pCommandAllocatorAllocatorPage = pCommandAllocatorAllocatorPage->pNext;
        }
    }

    {
        page_allocator_page_entry_t<render_pass_t>* pRenderPassAllocatorPage = pRenderResourceCache->renderPassAllocator.pFirstPage;
        while(pRenderPassAllocatorPage != nullptr)
        {
            for(uint32_t i = 0u; i < pRenderResourceCache->renderPassAllocator.elementCountPerPage; ++i)
            {
                COM_RELEASE(pRenderPassAllocatorPage->pMemory[i].pEndPassFence);
            }

            pRenderPassAllocatorPage = pRenderPassAllocatorPage->pNext;
        }
    }


    destroyHashMap(&pRenderResourceCache->vertexFormats);
    destroyHashMap(&pRenderResourceCache->graphicPipelines);
    destroyHashMap(&pRenderResourceCache->computePipelines);

    destroyPageAllocator(&pRenderResourceCache->gpuBufferAllocator);
    destroyPageAllocator(&pRenderResourceCache->gpuTextureAllocator);
    destroyPageAllocator(&pRenderResourceCache->shaderBinaryAllocator);
    destroyPageAllocator(&pRenderResourceCache->renderTargetAllocator);
    destroyPageAllocator(&pRenderResourceCache->renderPassAllocator);
    destroyPageAllocator(&pRenderResourceCache->samplerAllocator);
    destroyPageAllocator(&pRenderResourceCache->gpuCommandAllocatorAllocator);
    destroyPageAllocator(&pRenderResourceCache->gpuCommandBufferAllocator);
}

D3D12_COMMAND_LIST_TYPE mapGpuPassTypeToD3D12CommandListType(const gpu_pass_type_t gpuPassType)
{
    switch(gpuPassType)
    {
        case gpu_pass_type_t::render:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case gpu_pass_type_t::compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case gpu_pass_type_t::copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return D3D12_COMMAND_LIST_TYPE_DIRECT;
}

flags32_t<gpu_resource_state_flag_t> mapBufferUsageToResourceStates(const flags8_t<gpu_buffer_usage_flag_t> gpuBufferUsage)
{
    flags32_t<gpu_resource_state_flag_t> resourceState;
    if(gpuBufferUsage.isFlagSet(gpu_buffer_usage_flag_t::vertex_buffer))
    {
        resourceState.setFlag(gpu_resource_state_flag_t::vertex_buffer);
    }
    if(gpuBufferUsage.isFlagSet(gpu_buffer_usage_flag_t::constant_buffer))
    {
        resourceState.setFlag(gpu_resource_state_flag_t::constant_buffer);
    }
    if(gpuBufferUsage.isFlagSet(gpu_buffer_usage_flag_t::index_buffer))
    {
        resourceState.setFlag(gpu_resource_state_flag_t::index_buffer);
    }
    if(gpuBufferUsage.isFlagSet(gpu_buffer_usage_flag_t::storage_buffer))
    {
        resourceState.setFlag(gpu_resource_state_flag_t::unordered_access);
    }
    if(gpuBufferUsage.isFlagSet(gpu_buffer_usage_flag_t::transfer_dst))
    {
        resourceState.setFlag(gpu_resource_state_flag_t::transfer_dest);
    }
    if(gpuBufferUsage.isFlagSet(gpu_buffer_usage_flag_t::transfer_src))
    {
        resourceState.setFlag(gpu_resource_state_flag_t::transfer_src);
    }

    ASSERT_DEBUG(resourceState.value != 0);
    return resourceState;
}

D3D12_RESOURCE_STATES mapResourceStateMaskToD3D12ResourceStateMask(const flags32_t<gpu_resource_state_flag_t> resourceStateMask)
{
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::constant_buffer) || resourceStateMask.isFlagSet(gpu_resource_state_flag_t::vertex_buffer))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::index_buffer))
    {
        resourceStates |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::transfer_dest))
    {
        resourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::depth_read))
    {
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_READ;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::depth_write))
    {
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::stream_out))
    {
        resourceStates |= D3D12_RESOURCE_STATE_STREAM_OUT;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::indirect_argument))
    {
        resourceStates |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::resolve_dest))
    {
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::resolve_src))
    {
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::acceleration_structure))
    {
        resourceStates |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::generic_read))
    {
        resourceStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::video_decode_read))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_DECODE_READ;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::video_decode_write))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::video_process_read))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::video_process_write))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::video_encode_read))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::video_encode_write))
    {
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::transfer_src))
    {
        resourceStates |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::unordered_access))
    {
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::render_target))
    {
        resourceStates |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if(resourceStateMask.isFlagSet(gpu_resource_state_flag_t::present))
    {
        resourceStates |= D3D12_RESOURCE_STATE_PRESENT;
    }

    return resourceStates;
}

D3D12_RESOURCE_STATES mapShaderTypeMaskToD3D12ResourceStateMask(const flags8_t<shader_type_t>& shaderTypeMask)
{
    D3D12_RESOURCE_STATES shaderMask = D3D12_RESOURCE_STATE_COMMON;
    if(shaderTypeMask.isFlagSet(shader_type_t::pixel_shader))
    {
        shaderMask |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    if(shaderTypeMask.isFlagSet(shader_type_t::compute_shader) || shaderTypeMask.isFlagSet(shader_type_t::vertex_shader))
    {
        shaderMask |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    return shaderMask;
}

bool initGpuCommandAllocator(D3D12DeviceType* pDevice, const gpu_pass_type_t gpuPassType, linked_list_node_t<gpu_command_allocator_t>* pFirstAllocatorNode)
{
    D3D12_COMMAND_LIST_TYPE commandListType = mapGpuPassTypeToD3D12CommandListType(gpuPassType);
    linked_list_node_t<gpu_command_allocator_t>* pCurrentNode = pFirstAllocatorNode;
    while(pCurrentNode != nullptr)
    {
        gpu_command_allocator_t* pCurrentCommandAllocator = (gpu_command_allocator_t*)pCurrentNode;
        if(COM_CALL(pDevice->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&pCurrentCommandAllocator->pCommandAllocator)) != S_OK))
        {
            return false;
        }

        pCurrentCommandAllocator->isInUse = false;
        pCurrentCommandAllocator->forPassType = gpuPassType;
        pCurrentNode = pCurrentNode->pNext;
    }

    return true;
}

bool initGpuCommandBuffer(D3D12DeviceType* pDevice, const gpu_pass_type_t gpuPassType, linked_list_node_t<gpu_command_buffer_t>* pFirstCommandBufferNode)
{
    D3D12_COMMAND_LIST_TYPE commandListType = mapGpuPassTypeToD3D12CommandListType(gpuPassType);
    linked_list_node_t<gpu_command_buffer_t>* pCurrentNode = pFirstCommandBufferNode;
    while(pCurrentNode != nullptr)
    {
        gpu_command_buffer_t* pCurrentCommandBuffer = (gpu_command_buffer_t*)pCurrentNode;
        if(COM_CALL(pDevice->CreateCommandList1(0u, commandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&pCurrentCommandBuffer->pCommandList)) != S_OK))
        {
            return false;
        }

        pCurrentCommandBuffer->isOpen = false;
        pCurrentCommandBuffer->forPassType = gpuPassType;
        pCurrentNode = pCurrentNode->pNext;
    }

    return true;
}

bool initGpuCommandAllocators(D3D12DeviceType* pDevice, const uint32_t* pGpuPassCountPerQueueType, page_allocator_t<gpu_command_allocator_t>* pCommandAllocatorAllocator, linked_list_node_t<gpu_command_allocator_t>** ppFirstFreeCommandAllocatorPerQueueType)
{
    uint32_t offset = 0u;
    const uint32_t queueCount = (uint32_t)gpu_pass_type_t::count;
    for(uint32_t queueIndex = 0u; queueIndex < queueCount; ++queueIndex)
    {
        const uint32_t countPerQueueType = pGpuPassCountPerQueueType[queueIndex];
        createLinkedList(&ppFirstFreeCommandAllocatorPerQueueType[queueIndex], pCommandAllocatorAllocator->pFirstPage->pMemory + offset, countPerQueueType);
        offset += countPerQueueType;

        if(!initGpuCommandAllocator(pDevice, (gpu_pass_type_t)queueIndex, ppFirstFreeCommandAllocatorPerQueueType[queueIndex]))
        {
            return false;
        }
    }

    return true;
}

bool initGpuCommandBuffers(D3D12DeviceType* pDevice, const uint32_t* pGpuPassCountPerQueueType, page_allocator_t<gpu_command_buffer_t>* pGpuCommandBufferAllocator, linked_list_node_t<gpu_command_buffer_t>** ppFirstFreeCommandBufferPerQueueType)
{
    uint32_t offset = 0u;
    const uint32_t queueCount = (uint32_t)gpu_pass_type_t::count;
    for(uint32_t queueIndex = 0u; queueIndex < queueCount; ++queueIndex)
    {
        const uint32_t countPerQueueType = pGpuPassCountPerQueueType[queueIndex];
        createLinkedList(&ppFirstFreeCommandBufferPerQueueType[queueIndex], pGpuCommandBufferAllocator->pFirstPage->pMemory + offset, countPerQueueType);
        offset += countPerQueueType;

        if(!initGpuCommandBuffer(pDevice, (gpu_pass_type_t)queueIndex, ppFirstFreeCommandBufferPerQueueType[queueIndex]))
        {
            return false;
        }
    }

    return true;
}

bool createRenderPassFences(D3D12DeviceType* pDevice, linked_list_node_t<render_pass_t>* pFirstRenderPassNode)
{
    ASSERT_DEBUG(pDevice != nullptr);
    ASSERT_DEBUG(pFirstRenderPassNode != nullptr);
    render_pass_t* pRenderPass = (render_pass_t*)pFirstRenderPassNode;
    ASSERT_DEBUG(pRenderPass->pEndPassFence == nullptr);

    while(pRenderPass != nullptr)
    {
        if(!createFence(pDevice, &pRenderPass->pEndPassFence, 0u))
        {
            return false;
        }
        pRenderPass = pRenderPass->pNext;
    }

    return true;
}

bool createRenderResourceCache(D3D12DeviceType* pDevice, render_resource_cache_t* pOutRenderResourceCache, memory_allocator_t* pMemoryAllocator, const render_context_parameters_t::limits_t* pLimits, const bool notifyOnLimitReach)
{
    pOutRenderResourceCache->pMemoryAllocator = pMemoryAllocator;
    pOutRenderResourceCache->flags = 0u;
    pOutRenderResourceCache->pDevice = pDevice;

    if(notifyOnLimitReach)
    {
        pOutRenderResourceCache->flags |= render_resource_flags_t::notify_on_array_grow;
    }

    const uint32_t queueCount = (uint32_t)gpu_pass_type_t::count;
    pOutRenderResourceCache->ppFirstFreeCommandAllocatorPerQueueType = (linked_list_node_t<gpu_command_allocator_t>**)allocateFromAllocator(pMemoryAllocator, sizeof(void*) * queueCount, alloc_flags_t::clear_memory);
    pOutRenderResourceCache->ppFirstFreeCommandBufferPerQueueType = (linked_list_node_t<gpu_command_buffer_t>**)allocateFromAllocator(pMemoryAllocator, sizeof(void*) * queueCount, alloc_flags_t::clear_memory);

    if(pOutRenderResourceCache->ppFirstFreeCommandAllocatorPerQueueType == nullptr || pOutRenderResourceCache->ppFirstFreeCommandBufferPerQueueType == nullptr)
    {
        destroyRenderResourceCache(pOutRenderResourceCache);
        return false;
    }

    const uint32_t totalGpuPassCount = pLimits->maxRenderPassCount + pLimits->maxComputePassCount + pLimits->maxCopyPassCount;
    const uint32_t gpuPassCountPerQueueType[] = {
        pLimits->maxRenderPassCount,
        pLimits->maxComputePassCount,
        pLimits->maxCopyPassCount
    };
    static_assert(ARRAY_SIZE(gpuPassCountPerQueueType) == (uint32_t)gpu_pass_type_t::count);

    uint64_t offsetInBytes = 0u;
    bool renderResourceCacheAllocationFailed = false;
    renderResourceCacheAllocationFailed |= !createHashMap<vertex_format_t>(&pOutRenderResourceCache->vertexFormats, pMemoryAllocator, pLimits->maxVertexFormatCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createHashMap<graphics_pipeline_t>(&pOutRenderResourceCache->graphicPipelines, pMemoryAllocator, pLimits->maxGraphicsPipelineCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createHashMap<compute_pipeline_t>(&pOutRenderResourceCache->computePipelines, pMemoryAllocator, pLimits->maxComputePipelineCount, clear_memory);

    renderResourceCacheAllocationFailed |= !createPageAllocator<gpu_texture_t>(&pOutRenderResourceCache->gpuTextureAllocator, pMemoryAllocator, pLimits->maxGpuTextureCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<gpu_texture_view_t>(&pOutRenderResourceCache->gpuTextureViewAllocator, pMemoryAllocator, pLimits->maxGpuTextureViewCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<gpu_buffer_t>(&pOutRenderResourceCache->gpuBufferAllocator, pMemoryAllocator, pLimits->maxGpuBufferCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<shader_binary_t>(&pOutRenderResourceCache->shaderBinaryAllocator, pMemoryAllocator, pLimits->maxShaderBinaryCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<render_target_t>(&pOutRenderResourceCache->renderTargetAllocator, pMemoryAllocator, pLimits->maxRenderTargetCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<render_pass_t>(&pOutRenderResourceCache->renderPassAllocator, pMemoryAllocator, totalGpuPassCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<texture_sampler_t>(&pOutRenderResourceCache->samplerAllocator, pMemoryAllocator, pLimits->maxSamplerCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<gpu_command_allocator_t>(&pOutRenderResourceCache->gpuCommandAllocatorAllocator, pMemoryAllocator, totalGpuPassCount, clear_memory);
    renderResourceCacheAllocationFailed |= !createPageAllocator<gpu_command_buffer_t>(&pOutRenderResourceCache->gpuCommandBufferAllocator, pMemoryAllocator, totalGpuPassCount, clear_memory);

    if(renderResourceCacheAllocationFailed)
    {
        destroyRenderResourceCache(pOutRenderResourceCache);
        return false;
    }

    createLinkedList(&pOutRenderResourceCache->pFirstFreeRenderPass, pOutRenderResourceCache->renderPassAllocator.pFirstPage->pMemory, pOutRenderResourceCache->renderPassAllocator.elementCountPerPage);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeRenderTarget, pOutRenderResourceCache->renderTargetAllocator.pFirstPage->pMemory, pOutRenderResourceCache->renderTargetAllocator.elementCountPerPage);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeGpuBuffer, pOutRenderResourceCache->gpuBufferAllocator.pFirstPage->pMemory, pOutRenderResourceCache->gpuBufferAllocator.elementCountPerPage);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeGpuTexture, pOutRenderResourceCache->gpuTextureAllocator.pFirstPage->pMemory, pOutRenderResourceCache->gpuTextureAllocator.elementCountPerPage);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeGpuTextureView, pOutRenderResourceCache->gpuTextureViewAllocator.pFirstPage->pMemory, pOutRenderResourceCache->gpuTextureAllocator.elementCountPerPage);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeShaderBinary, pOutRenderResourceCache->shaderBinaryAllocator.pFirstPage->pMemory, pOutRenderResourceCache->shaderBinaryAllocator.elementCountPerPage);
    createLinkedList(&pOutRenderResourceCache->pFirstFreeSampler, pOutRenderResourceCache->samplerAllocator.pFirstPage->pMemory, pOutRenderResourceCache->samplerAllocator.elementCountPerPage);

    if(!initGpuCommandAllocators(pDevice, gpuPassCountPerQueueType, &pOutRenderResourceCache->gpuCommandAllocatorAllocator, pOutRenderResourceCache->ppFirstFreeCommandAllocatorPerQueueType))
    {
        destroyRenderResourceCache(pOutRenderResourceCache);
        return false;
    }

    if(!initGpuCommandBuffers(pDevice, gpuPassCountPerQueueType, &pOutRenderResourceCache->gpuCommandBufferAllocator, pOutRenderResourceCache->ppFirstFreeCommandBufferPerQueueType))
    {
        destroyRenderResourceCache(pOutRenderResourceCache);
        return false;
    }

    if(!createRenderPassFences(pDevice, pOutRenderResourceCache->pFirstFreeRenderPass))
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

    const uint32_t queueCount = (uint32_t)gpu_pass_type_t::count;
    pRenderContext->ppCommandQueues = (ID3D12CommandQueue**)allocateFromAllocator(&pRenderContext->defaultAllocator, sizeof(void*) * queueCount);
    if(pRenderContext->ppCommandQueues == nullptr)
    {
        return false;
    }

    for(uint32_t queueIndex = 0u; queueIndex < queueCount; ++queueIndex)
    {
        const D3D12_COMMAND_LIST_TYPE commandListType = mapGpuPassTypeToD3D12CommandListType((gpu_pass_type_t)queueIndex);
        if(!createCommandQueue(pRenderContext->pDevice, &pRenderContext->ppCommandQueues[queueIndex], commandListType))
        {
            return false;
        }
    }

    if(!createSwapChain(&pRenderContext->swapChain, &pRenderContext->defaultAllocator, pRenderContext->pFactory, pRenderContext->pDevice, pRenderContext->ppCommandQueues[(int)gpu_pass_type_t::render], pParameters->pWindowHandle, pParameters->windowWidth, pParameters->windowHeight, pParameters->frameBufferCount))
    {
        return false;
    }

    if(!createShaderCompilerContext(pAllocator, &pRenderContext->shaderCompilerContext))
    {
        return false;
    }

    //FK: Assume every GpuBuffer is accessed by compute
    const uint32_t numDescriptors = pParameters->limits.maxGpuTextureCount + pParameters->limits.maxRenderTargetCount + pParameters->limits.maxGpuBufferCount;
    if(!createDescriptorHeap(&pRenderContext->shaderVisibleDescriptorHeap, &pRenderContext->defaultAllocator, pRenderContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, numDescriptors))
    {
        return false;
    }

    if(!createDescriptorHeap(&pRenderContext->samplerDescriptorHeap, &pRenderContext->defaultAllocator, pRenderContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pParameters->limits.maxSamplerCount))
    {
        return false;
    }

    if(!createDescriptorHeap(&pRenderContext->renderTargetColorViewDescriptorHeap, &pRenderContext->defaultAllocator, pRenderContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pParameters->limits.maxRenderTargetCount))
    {
        return false;
    }

    if(!createDescriptorHeap(&pRenderContext->renderTargetDepthViewDescriptorHeap, &pRenderContext->defaultAllocator, pRenderContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pParameters->limits.maxRenderTargetCount))
    {
        return false;
    }

    graphics_frame_parameters_t graphicsFrameParameters = {};
    graphicsFrameParameters.pDevice                 = pRenderContext->pDevice;
    graphicsFrameParameters.ppCommandQueues         = pRenderContext->ppCommandQueues;
    graphicsFrameParameters.pShaderDescriptorHeap   = &pRenderContext->shaderVisibleDescriptorHeap;
    graphicsFrameParameters.pSamplerDescriptorHeap  = &pRenderContext->samplerDescriptorHeap;
    graphicsFrameParameters.pRenderResourceCache    = &pRenderContext->renderResourceCache;
    graphicsFrameParameters.pShaderCompilerContext  = &pRenderContext->shaderCompilerContext;
    graphicsFrameParameters.pRenderTargetColorViewDescriptorHeap  = &pRenderContext->renderTargetColorViewDescriptorHeap;
    graphicsFrameParameters.pRenderTargetDepthViewDescriptorHeap  = &pRenderContext->renderTargetDepthViewDescriptorHeap;
    if(!createGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, &pRenderContext->defaultAllocator, &graphicsFrameParameters, pParameters->frameBufferCount))
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

    setD3D12ObjectDebugName(pRenderContext->ppCommandQueues[(int)gpu_pass_type_t::render], "Direct Command Queue");
    setD3D12ObjectDebugName(pRenderContext->ppCommandQueues[(int)gpu_pass_type_t::copy], "Copy Command Queue");
    setD3D12ObjectDebugName(pRenderContext->ppCommandQueues[(int)gpu_pass_type_t::compute], "Compute Command Queue");

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

    resetStackAllocator(pGraphicsFrame->pFrameAllocator);

    return pGraphicsFrame;
}

void executeGpuPasses(render_context_t* pRenderContext, render_pass_t* pFirstRenderPass, const uint64_t frameIndex)
{
    render_pass_t* pRenderPass = pFirstRenderPass;
    while(pRenderPass)
    {
        const uint32_t renderPassTypeIndex = (uint32_t)pRenderPass->type;

        render_pass_t* pRenderPassDependency = pRenderPass->pFirstRenderPassDependency;
        while(pRenderPassDependency)
        {
            pRenderContext->ppCommandQueues[renderPassTypeIndex]->Wait(pRenderPassDependency->pEndPassFence, frameIndex);
            pRenderPassDependency = pRenderPassDependency->pNext;
        }

        pRenderContext->ppCommandQueues[renderPassTypeIndex]->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pRenderPass->pGpuCommandBuffer->pCommandList);
        pRenderContext->ppCommandQueues[renderPassTypeIndex]->Signal(pRenderPass->pEndPassFence, frameIndex);
        pRenderPass = (render_pass_t*)pRenderPass->pNext;
    }
}

render_pass_t* startCopyPass(graphics_frame_t*, const char*);
void executeRenderPass(graphics_frame_t*, render_pass_t*, render_pass_execution_order_t);
void endRenderPass(graphics_frame_t*, render_pass_t*);
void copyGpuBuffer(render_pass_t*, gpu_buffer_t*, gpu_buffer_t*);
void copyGpuTexture(render_pass_t*, gpu_texture_t*, gpu_texture_t*);
void copyGpuTextureFromBuffer(render_pass_t*, gpu_texture_t*, gpu_buffer_t*);

void setRenderPassDependency(graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass, const render_pass_t* pRenderPassDependency)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderPassDependency != nullptr);
    ASSERT_DEBUG(pRenderPass->isOpen == false);
    ASSERT_DEBUG(pRenderPassDependency->isOpen == false);

    render_pass_t* pRenderPassDependencyChainEntry = (render_pass_t*)allocateFromAllocator(pGraphicsFrame->pFrameAllocator, sizeof(render_pass_t), alloc_flags_t::clear_memory);
    memcpy(pRenderPassDependencyChainEntry, pRenderPassDependency, sizeof(render_pass_t));

    pRenderPassDependencyChainEntry->pNext = pRenderPass->pFirstRenderPassDependency;
    pRenderPass->pFirstRenderPassDependency = pRenderPassDependencyChainEntry;
}

void executeGpuCopyOperations(graphics_frame_t* pGraphicsFrame)
{
    render_pass_t* pCopyPass = startCopyPass(pGraphicsFrame, "Frame Copy Pass");
    for(uint32_t copyOperationIndex = 0u; copyOperationIndex < pGraphicsFrame->gpuCopyOperations.count; ++copyOperationIndex)
    {
        const gpu_copy_operation_t* pCopyOperation = (const gpu_copy_operation_t*)pGraphicsFrame->gpuCopyOperations.pData + copyOperationIndex;
        switch(pCopyOperation->type)
        {
            case gpu_copy_operation_type_t::copy_buffer_to_buffer:
                copyGpuBuffer(pCopyPass, pCopyOperation->destination.pBuffer, pCopyOperation->source.pBuffer);
                pCopyOperation->destination.pBuffer->resource.flags.clearFlag(gpu_resource_flag_t::used_in_copy_op);
                pCopyOperation->source.pBuffer->resource.flags.clearFlag(gpu_resource_flag_t::used_in_copy_op);
                continue;

            case gpu_copy_operation_type_t::copy_buffer_to_texture:
                copyGpuTextureFromBuffer(pCopyPass, pCopyOperation->destination.pTexture, pCopyOperation->source.pBuffer);
                pCopyOperation->destination.pTexture->resource.flags.clearFlag(gpu_resource_flag_t::used_in_copy_op);
                pCopyOperation->source.pBuffer->resource.flags.clearFlag(gpu_resource_flag_t::used_in_copy_op);
                continue;

            case gpu_copy_operation_type_t::copy_texture_to_texture:
                copyGpuTexture(pCopyPass, pCopyOperation->destination.pTexture, pCopyOperation->source.pTexture);
                pCopyOperation->destination.pTexture->resource.flags.clearFlag(gpu_resource_flag_t::used_in_copy_op);
                pCopyOperation->source.pTexture->resource.flags.clearFlag(gpu_resource_flag_t::used_in_copy_op);
                continue;
        }
    }

    endRenderPass(pGraphicsFrame, pCopyPass);
    executeRenderPass(pGraphicsFrame, pCopyPass, render_pass_execution_order_t::push_front);

    //first pass is always the copy pass, so skip that one
    render_pass_t* pRenderPass = pGraphicsFrame->pFirstRenderPassToExecute->pNext;
    while(pRenderPass)
    {
        if(pRenderPass->currentPipelineState.resourcesAreDependingOnCopyPass)
        {
            setRenderPassDependency(pGraphicsFrame, pRenderPass, pCopyPass);
        }
        pRenderPass = pRenderPass->pNext;
    }
    return;
}

void finishFrame(render_context_t* pRenderContext, graphics_frame_t* pGraphicsFrame)
{
    ASSERT_DEBUG(pRenderContext != nullptr);
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderContext->pCurrentGraphicsFrame == pGraphicsFrame);
    ASSERT_DEBUG(pGraphicsFrame->openGpuPassCount == 0u);

    validateMappedBuffers(pGraphicsFrame);

    const bool hasCopyOperations = pGraphicsFrame->gpuCopyOperations.count > 0;

    if(hasCopyOperations)
    {
        executeGpuCopyOperations(pGraphicsFrame);
        pGraphicsFrame->gpuCopyOperations.count = 0;
    }

    //NOTE: WARNING: There's currently no sync between copy queue and other queues. if hasCopyOperations == true then a copy queue pass will start and run
    executeGpuPasses(pRenderContext, pGraphicsFrame->pFirstRenderPassToExecute, pGraphicsFrame->frameIndex);
    
    COM_CALL(pRenderContext->swapChain.pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    
    COM_CALL(pRenderContext->ppCommandQueues[(uint32_t)gpu_pass_type_t::render]->Signal(pGraphicsFrame->ppFrameFences[(uint32_t)gpu_pass_type_t::render], pGraphicsFrame->frameIndex));
    COM_CALL(pGraphicsFrame->ppFrameFences[(uint32_t)gpu_pass_type_t::render]->SetEventOnCompletion(pGraphicsFrame->frameIndex, pGraphicsFrame->pFrameFinishedEvent));

    pRenderContext->pCurrentGraphicsFrame = nullptr;
}

template<typename T>
T* popObjectFromFreeList(linked_list_node_t<T>** ppFreeList, page_allocator_t<T>* pBackupAllocator, const flags8_t<render_resource_flags_t> flags, const char* pObjectName)
{
    if((*ppFreeList) == nullptr)
    {
        if(flags & render_resource_flags_t::notify_on_array_grow)
        {
            logWarning("free list for '%s' ran out of space, growing page allocator.\n", pObjectName);
            
            T* pNewNodes = (T*)allocateNewPageWorthOfObjects(pBackupAllocator, alloc_flags_t::clear_memory);
            if(pNewNodes == nullptr)
            {
                return nullptr;
            }

            const uint32_t objectPerPage = pBackupAllocator->elementCountPerPage;
            *ppFreeList = addNodesToLinkedList(ppFreeList, pNewNodes, objectPerPage);
        }
    }

    linked_list_node_t<T>* pFreeObject = *ppFreeList;
    *ppFreeList = (linked_list_node_t<T>*)pFreeObject->pNext;
    return (T*)pFreeObject;
}

shader_binary_t* getFreeShaderBinary(render_resource_cache_t* pRenderResourceCache)
{
    return popObjectFromFreeList(&pRenderResourceCache->pFirstFreeShaderBinary, &pRenderResourceCache->shaderBinaryAllocator, pRenderResourceCache->flags, "Shader Binaries");
}

render_target_t* getFreeRenderTarget(render_resource_cache_t* pRenderResourceCache)
{
    return popObjectFromFreeList(&pRenderResourceCache->pFirstFreeRenderTarget, &pRenderResourceCache->renderTargetAllocator, pRenderResourceCache->flags, "Render Targets");
}

render_pass_t* getFreeRenderPass(render_resource_cache_t* pRenderResourceCache)
{
    bool createNewRenderPassFences = ( pRenderResourceCache->pFirstFreeRenderPass == nullptr );
    render_pass_t* pRenderPass = popObjectFromFreeList(&pRenderResourceCache->pFirstFreeRenderPass, &pRenderResourceCache->renderPassAllocator, pRenderResourceCache->flags, "Render Passes");

    if(createNewRenderPassFences && pRenderPass != nullptr)
    {
        createRenderPassFences(pRenderResourceCache->pDevice, pRenderResourceCache->pFirstFreeRenderPass);
    }
    return pRenderPass;
}

gpu_texture_t* getFreeGpuTexture(render_resource_cache_t* pRenderResourceCache)
{
    gpu_texture_t* pGpuTexture = popObjectFromFreeList(&pRenderResourceCache->pFirstFreeGpuTexture, &pRenderResourceCache->gpuTextureAllocator, pRenderResourceCache->flags, "Gpu Textures");
    ASSERT_DEBUG(pGpuTexture != nullptr);
    ASSERT_DEBUG((pGpuTexture->resourceFlags & gpu_resource_flag_t::marked_as_free) == 0);
    return pGpuTexture;
}

gpu_texture_view_t* getFreeGpuTextureView(render_resource_cache_t* pRenderResourceCache)
{
    gpu_texture_view_t* pGpuTextureView = popObjectFromFreeList(&pRenderResourceCache->pFirstFreeGpuTextureView, &pRenderResourceCache->gpuTextureViewAllocator, pRenderResourceCache->flags, "Gpu Texture Views");
    ASSERT_DEBUG(pGpuTextureView != nullptr);
    return pGpuTextureView;
}

texture_sampler_t* getFreeSampler(render_resource_cache_t* pRenderResourceCache)
{
    return popObjectFromFreeList(&pRenderResourceCache->pFirstFreeSampler, &pRenderResourceCache->samplerAllocator, pRenderResourceCache->flags, "Samplers");
}

gpu_buffer_t* getFreeGpuBuffer(render_resource_cache_t* pRenderResourceCache)
{
    gpu_buffer_t* pGpuBuffer = popObjectFromFreeList(&pRenderResourceCache->pFirstFreeGpuBuffer, &pRenderResourceCache->gpuBufferAllocator, pRenderResourceCache->flags, "Gpu Buffers");
    ASSERT_DEBUG(pGpuBuffer != nullptr);
    ASSERT_DEBUG(pGpuBuffer->resourceFlags.isFlagClear(gpu_resource_flag_t::marked_as_free));
    return pGpuBuffer;
}

gpu_command_buffer_t* getFreeGpuCommandBuffer(render_resource_cache_t* pRenderResourceCache, const gpu_pass_type_t gpuPassType)
{
    const uint32_t index = (uint32_t)gpuPassType;
    return popObjectFromFreeList(&pRenderResourceCache->ppFirstFreeCommandBufferPerQueueType[index], &pRenderResourceCache->gpuCommandBufferAllocator, pRenderResourceCache->flags, "GPU Command Buffers");
}

gpu_command_allocator_t* getFreeGpuCommandAllocator(render_resource_cache_t* pRenderResourceCache, const gpu_pass_type_t gpuPassType)
{
    const uint32_t index = (uint32_t)gpuPassType;
    return popObjectFromFreeList(&pRenderResourceCache->ppFirstFreeCommandAllocatorPerQueueType[index], &pRenderResourceCache->gpuCommandAllocatorAllocator, pRenderResourceCache->flags, "GPU Command Buffer Allocators");
}

void openCommandBuffer(gpu_command_buffer_t* pGpuCommandBuffer, gpu_command_allocator_t* pGpuCommandAllocator)
{
    ASSERT_DEBUG(!pGpuCommandBuffer->isOpen);
    ASSERT_DEBUG(!pGpuCommandAllocator->isInUse);

    pGpuCommandBuffer->isOpen = true;
    pGpuCommandAllocator->isInUse = true;
    pGpuCommandAllocator->pCommandBufferUsingThisAllocator = pGpuCommandBuffer;

    D3D12GraphicsCommandListType* pCommandList = pGpuCommandBuffer->pCommandList;
    ID3D12CommandAllocator* pCommandAllocator = pGpuCommandAllocator->pCommandAllocator;
    COM_CALL(pCommandList->Reset(pCommandAllocator, nullptr));
}

void closeCommandBuffer(gpu_command_buffer_t* pGpuCommandBuffer, gpu_command_allocator_t* pGpuCommandAllocator)
{
    ASSERT_DEBUG(pGpuCommandBuffer->isOpen);
    ASSERT_DEBUG(pGpuCommandAllocator->pCommandBufferUsingThisAllocator == pGpuCommandBuffer);

    pGpuCommandBuffer->isOpen = false;
    pGpuCommandAllocator->isInUse = false;
    pGpuCommandAllocator->pCommandBufferUsingThisAllocator = nullptr;

    D3D12GraphicsCommandListType* pCommandList = pGpuCommandBuffer->pCommandList;
    addPixEndMarker(pCommandList);
    COM_CALL(pCommandList->Close());
}

NO_DISCARD render_target_t* createRenderTarget(graphics_frame_t* pGraphicsFrame, uint3_t dimensions, gpu_texture_t* pColorTexture, gpu_texture_t* pDepthTexture)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGraphicsFrame != nullptr || pDepthTexture != nullptr);
    ASSERT_DEBUG(dimensions.x > 0);

    dimensions.y = dimensions.y == 0u ? 1u : dimensions.y;
    dimensions.z = dimensions.z == 0u ? 1u : dimensions.z;

    render_target_t* pRenderTarget = getFreeRenderTarget(pGraphicsFrame->pRenderResourceCache);
    if(pRenderTarget == nullptr)
    {
        return nullptr;
    }

    if(pColorTexture != nullptr)
    {
        descriptor_handle_t colorTargetDescriptor = {};
        if(!allocateDescriptor(&colorTargetDescriptor, pGraphicsFrame->pRenderTargetColorViewDescriptorHeap))
        {
            releaseRenderTarget(pGraphicsFrame, pRenderTarget);
            return nullptr;
        }

        pGraphicsFrame->pDevice->CreateRenderTargetView(pColorTexture->resource.pResource, nullptr, colorTargetDescriptor.cpuDescriptorHandle);
        pRenderTarget->colorBufferDescriptorHandle = colorTargetDescriptor;
        pRenderTarget->colorTextureResource = pColorTexture->resource;
    }

    if(pDepthTexture != nullptr)
    {
        descriptor_handle_t depthTargetDescriptor = {};
        if(!allocateDescriptor(&depthTargetDescriptor, pGraphicsFrame->pRenderTargetDepthViewDescriptorHeap))
        {
            releaseRenderTarget(pGraphicsFrame, pRenderTarget);
            return nullptr;
        }
        pGraphicsFrame->pDevice->CreateRenderTargetView(pDepthTexture->resource.pResource, nullptr, depthTargetDescriptor.cpuDescriptorHandle);
        pRenderTarget->depthBufferDescriptorHandle = depthTargetDescriptor;
        pRenderTarget->depthTextureResource = pDepthTexture->resource;
    }

    pRenderTarget->dimensions = dimensions;
    return pRenderTarget;
}

void updateResourceState(gpu_command_buffer_t* pCommandBuffer, gpu_resource_t* pResource, const flags32_t<gpu_resource_state_flag_t>& gpuResourceStateMask)
{
    if(gpuResourceStateMask != pResource->currentStateMask)
    {
        D3D12_RESOURCE_BARRIER resourceBarrier = {};
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Transition.pResource = pResource->pResource;
        resourceBarrier.Transition.StateAfter = mapResourceStateMaskToD3D12ResourceStateMask(gpuResourceStateMask);
        resourceBarrier.Transition.StateBefore = mapResourceStateMaskToD3D12ResourceStateMask(pResource->currentStateMask);

        pCommandBuffer->pCommandList->ResourceBarrier(1u, &resourceBarrier);

        pResource->currentStateMask = gpuResourceStateMask; 
    }
}

void updateResourceState(render_pass_t* pRenderPass, gpu_resource_t* pResource, const flags32_t<gpu_resource_state_flag_t>& gpuResourceStateMask)
{
    updateResourceState(pRenderPass->pGpuCommandBuffer, pResource, gpuResourceStateMask);
}

render_pass_t* startTypedGpuPass(graphics_frame_t* pGraphicsFrame, const char* pPassName, const gpu_pass_type_t gpuPassType)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);

    render_pass_t* pRenderPass = getFreeRenderPass(pGraphicsFrame->pRenderResourceCache);
    if(pRenderPass == nullptr)
    {
        return nullptr;
    }

    ++pGraphicsFrame->openGpuPassCount;

    resetPipelineState(&pRenderPass->currentPipelineState);

    pRenderPass->pGpuCommandAllocator = getFreeGpuCommandAllocator(pGraphicsFrame->pRenderResourceCache, gpuPassType);
    pRenderPass->pGpuCommandBuffer = getFreeGpuCommandBuffer(pGraphicsFrame->pRenderResourceCache, gpuPassType);

    openCommandBuffer(pRenderPass->pGpuCommandBuffer, pRenderPass->pGpuCommandAllocator);

    pRenderPass->isOpen = true;
    pRenderPass->pName = pPassName;
    pRenderPass->pGraphicsFrame = pGraphicsFrame;
    pRenderPass->type = gpuPassType;
    pRenderPass->pFirstRenderPassDependency = nullptr;

    setD3D12ObjectDebugName(pRenderPass->pGpuCommandBuffer->pCommandList, pPassName);    
    addPixBeginMarker(pRenderPass->pGpuCommandBuffer->pCommandList, pPassName);
    return pRenderPass;
}

NO_DISCARD render_pass_t* startRenderPass(graphics_frame_t* pGraphicsFrame, const char* pRenderPassName, render_target_t* pRenderTarget)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);

    render_pass_t* pRenderPass = startTypedGpuPass(pGraphicsFrame, pRenderPassName, gpu_pass_type_t::render);
    if(pRenderPass == nullptr)
    {
        return nullptr;
    }

    if(pRenderTarget->colorBufferDescriptorHandle.cpuDescriptorHandle.ptr != 0)
    {
        updateResourceState(pRenderPass->pGpuCommandBuffer, &pRenderTarget->colorTextureResource, gpu_resource_state_flag_t::render_target);
    }

    if(pRenderTarget->depthBufferDescriptorHandle.cpuDescriptorHandle.ptr != 0)
    {
        updateResourceState(pRenderPass->pGpuCommandBuffer, &pRenderTarget->depthTextureResource, gpu_resource_state_flag_t::render_target);
    }

    resetPipelineState(&pRenderPass->cachedPipelineState);

    pRenderPass->currentPipelineState.viewport.x = 0;
    pRenderPass->currentPipelineState.viewport.y = 0;
    pRenderPass->currentPipelineState.viewport.width = pRenderTarget->dimensions.x;
    pRenderPass->currentPipelineState.viewport.height = pRenderTarget->dimensions.y;
    pRenderPass->currentPipelineState.scissor.x = 0;
    pRenderPass->currentPipelineState.scissor.y = 0;
    pRenderPass->currentPipelineState.scissor.width = pRenderTarget->dimensions.x;
    pRenderPass->currentPipelineState.scissor.height = pRenderTarget->dimensions.y;
    pRenderPass->currentPipelineState.pRenderTarget = pRenderTarget;
    return pRenderPass;
}

NO_DISCARD render_pass_t* startComputePass(graphics_frame_t* pGraphicsFrame, const char* pRenderPassName)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    return startTypedGpuPass(pGraphicsFrame, pRenderPassName, gpu_pass_type_t::compute);
}

NO_DISCARD render_pass_t* startCopyPass(graphics_frame_t* pGraphicsFrame, const char* pRenderPassName)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    return startTypedGpuPass(pGraphicsFrame, pRenderPassName, gpu_pass_type_t::copy);
}

void addCommandAllocatorsToAllocatorsToReset(graphics_frame_t* pGraphicsFrame, gpu_command_allocator_t* pCommandAllocator)
{
    bool commandAllocatorAlreadyAdded = false;
    gpu_command_allocator_t* pCurrentCommandAllocator = pGraphicsFrame->pFirstCommandAllocatorToReset;
    while(pCurrentCommandAllocator != nullptr)
    {
        if(pCurrentCommandAllocator == pCommandAllocator)
        {
            commandAllocatorAlreadyAdded = true;
            break;
        }

        pCurrentCommandAllocator = pCurrentCommandAllocator->pNext;
    }

    if(commandAllocatorAlreadyAdded)
    {
        return;
    }

    pCommandAllocator->pNext = pGraphicsFrame->pFirstCommandAllocatorToReset;
    pGraphicsFrame->pFirstCommandAllocatorToReset = pCommandAllocator;
}

void endRenderPass(graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGraphicsFrame->openGpuPassCount > 0);
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderPass->isOpen);

    pRenderPass->isOpen = false;
    --pGraphicsFrame->openGpuPassCount;

    if(pRenderPass->currentPipelineState.pRenderTarget == pGraphicsFrame->pBackBuffer)
    {
        updateResourceState(pRenderPass->pGpuCommandBuffer, &pRenderPass->currentPipelineState.pRenderTarget->colorTextureResource, gpu_resource_state_flag_t::present);
    }

    closeCommandBuffer(pRenderPass->pGpuCommandBuffer, pRenderPass->pGpuCommandAllocator);

    const uint32_t queueIndex = (uint32_t)pRenderPass->type;
    //TODO: addNodesToLinkedList(&pGraphicsFrame->pRenderResourceCache->ppFirstFreeCommandAllocatorPerQueueType[queueIndex], pRenderPass->pGpuCommandAllocator, 1u);
    addCommandAllocatorsToAllocatorsToReset(pGraphicsFrame, pRenderPass->pGpuCommandAllocator);
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

void bindComputePipeline(render_pass_t* pRenderPass, compute_pipeline_t* pComputePipeline)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pComputePipeline != nullptr);
    pRenderPass->currentPipelineState.pComputePipeline = pComputePipeline;
}

void bindVertexBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pVertexBuffer, const vertex_format_t* pVertexFormat, uint32_t slotIndex)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pVertexBuffer != nullptr);
    if(!Validate(pVertexBuffer->bufferUsageMask.isFlagSet(gpu_buffer_usage_flag_t::vertex_buffer), "Gpu buffer '%s' is not a vertex buffer.", pVertexBuffer->pName))
    {
        return;
    }

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = pVertexBuffer->resource.pResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes    = pVertexBuffer->sizeInBytes;
    vertexBufferView.StrideInBytes  = calculateVertexStrideSizeInBytes(pVertexFormat);
    pRenderPass->pGpuCommandBuffer->pCommandList->IASetVertexBuffers(slotIndex, 1u, &vertexBufferView);
}

void bindIndexBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pIndexBuffer, const index_format_t indexFormat)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pIndexBuffer != nullptr);
    if(!Validate(pIndexBuffer->bufferUsageMask.isFlagSet(gpu_buffer_usage_flag_t::index_buffer), "Gpu buffer '%s' is not an index buffer.", pIndexBuffer->pName))
    {
        return;
    }

    const DXGI_FORMAT dxgiIndexFormat = (indexFormat == index_format_t::unsigned_int_16bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation  = pIndexBuffer->resource.pResource->GetGPUVirtualAddress();
    indexBufferView.Format          = dxgiIndexFormat;
    indexBufferView.SizeInBytes     = pIndexBuffer->sizeInBytes;
    pRenderPass->pGpuCommandBuffer->pCommandList->IASetIndexBuffer(&indexBufferView);
}

pipeline_type_t mapGpuPassTypeToPipelineType(const gpu_pass_type_t gpuPassType)
{
    switch(gpuPassType)
    {
        case gpu_pass_type_t::render:
            return pipeline_type_t::graphics_pipeline;
        case gpu_pass_type_t::compute:
            return pipeline_type_t::compute_pipeline;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return pipeline_type_t::graphics_pipeline;
}

void bindStructuredBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pStructuredBuffer, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pStructuredBuffer != nullptr);
    if(!Validate(pStructuredBuffer->bufferUsageMask.isFlagSet(gpu_buffer_usage_flag_t::storage_buffer), "Gpu buffer '%s' is not a storage buffer.", pStructuredBuffer->pName))
    {
        return;
    }

    const pipeline_type_t pipelineType = mapGpuPassTypeToPipelineType(pRenderPass->type);
    ASSERT_DEBUG(pRenderPass->currentPipelineState.resourceBindingCount[pipelineType] < maxPipelineBindingPoints);

    const uint32_t boundResourceOffset = pipelineType * maxPipelineBindingPoints;
    const uint32_t boundResourceIndex = boundResourceOffset + pRenderPass->currentPipelineState.resourceBindingCount[pipelineType]++;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].type              = resource_binding_type_t::structured_buffer;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].pResource         = &pStructuredBuffer->resource;
}

void bindConstantBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pConstantBuffer, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pConstantBuffer != nullptr);
    if(!Validate(pConstantBuffer->bufferUsageMask.isFlagSet(gpu_buffer_usage_flag_t::constant_buffer), "Gpu buffer '%s' is not a constant buffer.", pConstantBuffer->pName))
    {
        return;
    }

    const pipeline_type_t pipelineType = mapGpuPassTypeToPipelineType(pRenderPass->type);
    ASSERT_DEBUG(pRenderPass->currentPipelineState.resourceBindingCount[pipelineType] < maxPipelineBindingPoints);

    const uint32_t boundResourceOffset = pipelineType * maxPipelineBindingPoints;
    const uint32_t boundResourceIndex = boundResourceOffset + pRenderPass->currentPipelineState.resourceBindingCount[pipelineType]++;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].type              = resource_binding_type_t::constant_buffer;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].pResource         = &pConstantBuffer->resource;
}

void bindTexture(render_pass_t* pRenderPass, gpu_texture_t* pTexture, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pTexture != nullptr);

    const pipeline_type_t pipelineType = mapGpuPassTypeToPipelineType(pRenderPass->type);
    ASSERT_DEBUG(pRenderPass->currentPipelineState.resourceBindingCount[pipelineType] < maxPipelineBindingPoints);

    const uint32_t boundResourceOffset = pipelineType * maxPipelineBindingPoints;
    const uint32_t boundResourceIndex = boundResourceOffset + pRenderPass->currentPipelineState.resourceBindingCount[pipelineType]++;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].type              = resource_binding_type_t::texture;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].pResource         = &pTexture->resource;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].descriptorHandle  = pTexture->pView->shaderResourceView;
}

void bindTextureSampler(render_pass_t* pRenderPass, texture_sampler_t* pSampler, uint32_t registerIndex, uint32_t registerSpace)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pSampler != nullptr);

    const pipeline_type_t pipelineType = mapGpuPassTypeToPipelineType(pRenderPass->type);
    ASSERT_DEBUG(pRenderPass->currentPipelineState.resourceBindingCount[pipelineType] < maxPipelineBindingPoints);

    const uint32_t boundResourceOffset = pipelineType * maxPipelineBindingPoints;
    const uint32_t boundResourceIndex = boundResourceOffset + pRenderPass->currentPipelineState.resourceBindingCount[pipelineType]++;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].type              = resource_binding_type_t::sampler;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerIndex     = registerIndex;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].registerSpace     = registerSpace;
    pRenderPass->currentPipelineState.boundResources[boundResourceIndex].descriptorHandle  = pSampler->descriptorHandle;
}

void bindGraphicsPipeline(render_pass_t* pRenderPass, graphics_pipeline_t* pGraphicsPipeline)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pGraphicsPipeline != nullptr);
    pRenderPass->currentPipelineState.pGraphicsPipeline = pGraphicsPipeline;
}

void applyGraphicsPipeline(gpu_command_buffer_t* pGpuCommandBuffer, pipeline_state_t* pPipelineState, const graphics_pipeline_t* pGraphicsPipeline)
{
    if(pPipelineState->pGraphicsPipeline == pGraphicsPipeline)
    {
        return;
    }
    
    pPipelineState->pGraphicsPipeline = pGraphicsPipeline;

    pGpuCommandBuffer->pCommandList->SetPipelineState(pGraphicsPipeline->pPipelineState);
	pGpuCommandBuffer->pCommandList->SetGraphicsRootSignature(pGraphicsPipeline->pRootSignature);
	pGpuCommandBuffer->pCommandList->IASetPrimitiveTopology(mapTopologyToD3D12Topology(pGraphicsPipeline->topology));
}

void applyComputePipeline(gpu_command_buffer_t* pGpuCommandBuffer, pipeline_state_t* pPipelineState, const compute_pipeline_t* pComputePipeline)
{
    if(pPipelineState->pComputePipeline == pComputePipeline)
    {
        return;
    }
    
    pPipelineState->pComputePipeline = pComputePipeline;

    pGpuCommandBuffer->pCommandList->SetPipelineState(pComputePipeline->pPipelineState);
	pGpuCommandBuffer->pCommandList->SetComputeRootSignature(pComputePipeline->pRootSignature);
}

void applyRenderTarget(gpu_command_buffer_t* pGpuCommandBuffer, pipeline_state_t* pPipelineState, render_target_t* pRenderTarget)
{
    if(pPipelineState->pRenderTarget == pRenderTarget)
    {
        //return;
    }

    pPipelineState->pRenderTarget = pRenderTarget;

    D3D12_CPU_DESCRIPTOR_HANDLE* pColorDescriptorHandle = pRenderTarget->colorBufferDescriptorHandle.cpuDescriptorHandle.ptr == 0u ? nullptr : &pRenderTarget->colorBufferDescriptorHandle.cpuDescriptorHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE* pDepthDescriptorHandle = pRenderTarget->depthBufferDescriptorHandle.cpuDescriptorHandle.ptr == 0u ? nullptr : &pRenderTarget->depthBufferDescriptorHandle.cpuDescriptorHandle;

    if(pColorDescriptorHandle != nullptr)
    {
        updateResourceState(pGpuCommandBuffer, &pRenderTarget->colorTextureResource, gpu_resource_state_flag_t::render_target);
    }

    if(pDepthDescriptorHandle != nullptr)
    {
        updateResourceState(pGpuCommandBuffer, &pRenderTarget->depthTextureResource, gpu_resource_state_flag_t::render_target);
    }

    pGpuCommandBuffer->pCommandList->OMSetRenderTargets(1u, pColorDescriptorHandle, FALSE, pDepthDescriptorHandle);
}

void applyViewport(gpu_command_buffer_t* pGpuCommandBuffer, pipeline_state_t* pCachedPipelineState, const viewport_t* pViewport)
{
    if(memcmp(&pCachedPipelineState->viewport, pViewport, sizeof(viewport_t)) == 0u)
    {
        return;
    }

    pCachedPipelineState->viewport = *pViewport;

    D3D12_VIEWPORT viewport = {};
    viewport.Height     = (float)pViewport->height;
    viewport.Width      = (float)pViewport->width;
    viewport.TopLeftX   = (float)pViewport->x;
    viewport.TopLeftY   = (float)pViewport->y;
    viewport.MinDepth   = (float)pViewport->minDepth;
    viewport.MaxDepth   = (float)pViewport->maxDepth;
    pGpuCommandBuffer->pCommandList->RSSetViewports(1u, &viewport);
}

void applyScissor(gpu_command_buffer_t* pGpuCommandBuffer, pipeline_state_t* pPipelineState, const scissor_t* pScissor)
{
    if(memcmp(&pPipelineState->scissor, pScissor, sizeof(scissor_t)) == 0u)
    {
        return;
    }

    pPipelineState->scissor = *pScissor;

    D3D12_RECT scissorRect = {};
    scissorRect.left    = pScissor->x;
    scissorRect.top     = pScissor->y;
    scissorRect.right   = pScissor->x + pScissor->width;
    scissorRect.bottom  = pScissor->y + pScissor->height;
    pGpuCommandBuffer->pCommandList->RSSetScissorRects(1u, &scissorRect);
}

uint32_t getMatchingShaderBindingPointIndex(const shader_binding_point_t* pShaderBindingPoints, const uint32_t shaderBindingPointCount, const resource_binding_t* pBoundResource)
{
    for(uint32_t bindingPointIndex = 0u; bindingPointIndex < shaderBindingPointCount; ++bindingPointIndex)
    {
        const shader_binding_point_t* pShaderBindingPoint = &pShaderBindingPoints[bindingPointIndex];
        if(pShaderBindingPoint->type == pBoundResource->type && pShaderBindingPoint->slot == pBoundResource->registerIndex && pShaderBindingPoint->space == pBoundResource->registerSpace)
        {
            return bindingPointIndex;
        }
    }

    return 0xFFFFFFFFu;
}

void applyBoundPipelineResources(gpu_command_buffer_t* pGpuCommandBuffer, const shader_binding_point_t* pPipelineBindingPoints, const uint32_t pipelineBindingPointCount, pipeline_state_t* pGlobalPipelineState, bool* pOutResourcesAreDependingOnCopyPass, const resource_binding_t* pBoundResources, const uint32_t resourceBindingCount, pipeline_type_t pipelineType)
{
    const uint32_t boundResourcesOffset = 32 * pipelineType;
    resource_binding_t* pPipelineBoundResources = pGlobalPipelineState->boundResources + boundResourcesOffset;
    if(memcmp(pPipelineBoundResources, pBoundResources, sizeof(resource_binding_t) * resourceBindingCount) == 0) 
    {
        return;
    }

    pGlobalPipelineState->resourceBindingCount[pipelineType] = 0;

    bool resourcesAreDependingOnCopyPass = false;
    for(uint32_t boundResourceIndex = 0u; boundResourceIndex < resourceBindingCount; ++boundResourceIndex)
    {
        const resource_binding_t* pBoundResource = &pBoundResources[boundResourceIndex];
        const uint32_t bindingPointIndex = getMatchingShaderBindingPointIndex(pPipelineBindingPoints, pipelineBindingPointCount, pBoundResource);
        if(bindingPointIndex == 0xFFFFFFFF)
        {
            continue;
        }

        pGlobalPipelineState->resourceBindingCount[pipelineType]++;

        const resource_binding_t* pAlreadyBoundResource = &pPipelineBoundResources[boundResourceIndex];
        if(memcmp(pAlreadyBoundResource, pBoundResource, sizeof(resource_binding_t)) == 0)
        {
            continue;
        }

        if(pBoundResource->pResource != nullptr)
        {
            resourcesAreDependingOnCopyPass |= pBoundResource->pResource->flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op);
        }

        if(pBoundResource->type == resource_binding_type_t::constant_buffer)
        {
            switch(pipelineType)
            {
                case pipeline_type_t::graphics_pipeline:
                    pGpuCommandBuffer->pCommandList->SetGraphicsRootConstantBufferView(bindingPointIndex, pBoundResource->pResource->pResource->GetGPUVirtualAddress());
                    break;
                case pipeline_type_t::compute_pipeline:
                    pGpuCommandBuffer->pCommandList->SetComputeRootConstantBufferView(bindingPointIndex, pBoundResource->pResource->pResource->GetGPUVirtualAddress());
                    break;
                default:
                    ASSERT_DEBUG_UNREACHABLE_CODE();
                    break;
            }
            continue;
        }
        else if(pBoundResource->type == resource_binding_type_t::texture || pBoundResource->type == resource_binding_type_t::sampler)
        {
            switch(pipelineType)
            {
                case pipeline_type_t::graphics_pipeline:
                    pGpuCommandBuffer->pCommandList->SetGraphicsRootDescriptorTable(bindingPointIndex, pBoundResource->descriptorHandle.gpuDescriptorHandle);
                    break;
                case pipeline_type_t::compute_pipeline:
                    pGpuCommandBuffer->pCommandList->SetComputeRootDescriptorTable(bindingPointIndex, pBoundResource->descriptorHandle.gpuDescriptorHandle);
                    break;
                default:
                    ASSERT_DEBUG_UNREACHABLE_CODE();
                    break;
            }
            continue;
        }
        else if(pBoundResource->type == resource_binding_type_t::structured_buffer)
        {
            switch(pipelineType)
            {
                case pipeline_type_t::graphics_pipeline:
                    pGpuCommandBuffer->pCommandList->SetGraphicsRootUnorderedAccessView(bindingPointIndex, pBoundResource->pResource->pResource->GetGPUVirtualAddress());
                    break;
                case pipeline_type_t::compute_pipeline:
                    pGpuCommandBuffer->pCommandList->SetComputeRootUnorderedAccessView(bindingPointIndex, pBoundResource->pResource->pResource->GetGPUVirtualAddress());
                    break;
                default:
                    ASSERT_DEBUG_UNREACHABLE_CODE();
                    break;
            }
            continue;
        }
        else
        {
            ASSERT_DEBUG_UNREACHABLE_CODE();
        }
    }

    *pOutResourcesAreDependingOnCopyPass = resourcesAreDependingOnCopyPass;
    memcpy(pPipelineBoundResources, pBoundResources, sizeof(resource_binding_t) * pGlobalPipelineState->resourceBindingCount[pipelineType]);
}

void updatePipelineResourceStates(gpu_command_buffer_t* pCommandBuffer, resource_binding_t* pResourceBindings, const uint32_t resourceBindingCount)
{
    if(resourceBindingCount == 0u)
    {
        return;
    }

    UINT32 resourceBarrierCount = 0;
    D3D12_RESOURCE_BARRIER* pResourceBarriers = (D3D12_RESOURCE_BARRIER*)_alloca(resourceBindingCount * sizeof(D3D12_RESOURCE_BARRIER));
    memset(pResourceBarriers, 0, resourceBindingCount * sizeof(D3D12_RESOURCE_BARRIER));

    for(uint32_t boundResourceIndex = 0; boundResourceIndex < resourceBindingCount; ++boundResourceIndex)
    {    
        if(pResourceBindings[boundResourceIndex].pResource == nullptr)
        {
            continue;
        }

        gpu_resource_t* pGpuResource = pResourceBindings[boundResourceIndex].pResource;
        if(pGpuResource->futureStateMask != pGpuResource->currentStateMask || pGpuResource->futureShaderAccessMask != pGpuResource->currentShaderAccessMask)
        {
            const D3D12_RESOURCE_STATES currentShaderAccessStates   = mapShaderTypeMaskToD3D12ResourceStateMask(pGpuResource->currentShaderAccessMask);
            const D3D12_RESOURCE_STATES futureShaderAccessStates    = mapShaderTypeMaskToD3D12ResourceStateMask(pGpuResource->futureShaderAccessMask);
            const D3D12_RESOURCE_STATES currentResourceStates       = mapResourceStateMaskToD3D12ResourceStateMask(pGpuResource->currentStateMask);
            const D3D12_RESOURCE_STATES futureResourceStates        = mapResourceStateMaskToD3D12ResourceStateMask(pGpuResource->futureStateMask);

            pResourceBarriers[resourceBarrierCount].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            pResourceBarriers[resourceBarrierCount].Transition.pResource = pGpuResource->pResource;
            pResourceBarriers[resourceBarrierCount].Transition.StateAfter = futureResourceStates | futureShaderAccessStates;
            pResourceBarriers[resourceBarrierCount].Transition.StateBefore = currentResourceStates | currentShaderAccessStates;
            ++resourceBarrierCount;

            pGpuResource->currentStateMask = pGpuResource->futureStateMask;
            pGpuResource->currentShaderAccessMask = pGpuResource->futureShaderAccessMask;
        }
    }

    if(resourceBarrierCount == 0)
    {
        return;
    }

    pCommandBuffer->pCommandList->ResourceBarrier(resourceBarrierCount, pResourceBarriers);
}

void applyPipelineState(gpu_command_buffer_t* pCommandBuffer, graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass, pipeline_type_t pipelineType)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pCommandBuffer != nullptr);
    ASSERT_DEBUG(pCommandBuffer->pCommandList != nullptr);

    ID3D12DescriptorHeap* descriptorHeaps[] = {
        pGraphicsFrame->pSamplerDescriptorHeap->pDescriptorHeap,
        pGraphicsFrame->pShaderVisibleDescriptorHeap->pDescriptorHeap
    };
    pCommandBuffer->pCommandList->SetDescriptorHeaps(2u, descriptorHeaps);

    const pipeline_state_t* pCurrentRenderPassPipelineState = &pRenderPass->currentPipelineState;
    pipeline_state_t* pCachedRenderPassPipelineState = &pRenderPass->cachedPipelineState;

    const uint32_t boundResourcesOffset = pipelineType * maxPipelineBindingPoints;
    updatePipelineResourceStates(pCommandBuffer, pRenderPass->currentPipelineState.boundResources + boundResourcesOffset, pRenderPass->currentPipelineState.resourceBindingCount[pipelineType]);
    if(pipelineType == pipeline_type_t::graphics_pipeline)
    {
        applyViewport(pCommandBuffer, pCachedRenderPassPipelineState, &pCurrentRenderPassPipelineState->viewport);
        applyScissor(pCommandBuffer, pCachedRenderPassPipelineState, &pCurrentRenderPassPipelineState->scissor);
        applyRenderTarget(pCommandBuffer, pCachedRenderPassPipelineState, pCurrentRenderPassPipelineState->pRenderTarget);
        if(pCurrentRenderPassPipelineState->pGraphicsPipeline != nullptr)
        {
            applyGraphicsPipeline(pCommandBuffer, pCachedRenderPassPipelineState, pCurrentRenderPassPipelineState->pGraphicsPipeline);
            applyBoundPipelineResources(pCommandBuffer, pCachedRenderPassPipelineState->pGraphicsPipeline->pShaderBindingPoints, pCachedRenderPassPipelineState->pGraphicsPipeline->shaderBindingPointCount, pCachedRenderPassPipelineState, &pCachedRenderPassPipelineState->resourcesAreDependingOnCopyPass, pCurrentRenderPassPipelineState->boundResources + boundResourcesOffset, pCurrentRenderPassPipelineState->resourceBindingCount[pipelineType], pipelineType);
        }
    }
    else if(pipelineType == pipeline_type_t::compute_pipeline)
    {
        applyComputePipeline(pCommandBuffer, pCachedRenderPassPipelineState, pCurrentRenderPassPipelineState->pComputePipeline);
        applyBoundPipelineResources(pCommandBuffer, pCachedRenderPassPipelineState->pComputePipeline->pShaderBindingPoints, pCachedRenderPassPipelineState->pComputePipeline->shaderBindingPointCount, pCachedRenderPassPipelineState, &pCachedRenderPassPipelineState->resourcesAreDependingOnCopyPass, pCurrentRenderPassPipelineState->boundResources + boundResourcesOffset, pCurrentRenderPassPipelineState->resourceBindingCount[pipelineType], pipelineType);
    }
    else
    {
        ASSERT_DEBUG_UNREACHABLE_CODE();
    }
}

void draw(render_pass_t* pRenderPass, const uint32_t vertexOffset, const uint32_t vertexCount)
{
    applyPipelineState(pRenderPass->pGpuCommandBuffer, pRenderPass->pGraphicsFrame, pRenderPass, pipeline_type_t::graphics_pipeline);
	pRenderPass->pGpuCommandBuffer->pCommandList->DrawInstanced(vertexCount, 1u, vertexOffset, 0u);
}

void drawIndexed(render_pass_t* pRenderPass, const uint32_t indexOffset, const uint32_t indexCount)
{
    applyPipelineState(pRenderPass->pGpuCommandBuffer, pRenderPass->pGraphicsFrame, pRenderPass, pipeline_type_t::graphics_pipeline);
    pRenderPass->pGpuCommandBuffer->pCommandList->DrawIndexedInstanced(indexCount, 1u, indexOffset, 0u, 0u);
}

void dispatch(render_pass_t* pRenderPass, uint32_t dispatchX, uint32_t dispatchY, uint32_t dispatchZ)
{
	applyPipelineState(pRenderPass->pGpuCommandBuffer, pRenderPass->pGraphicsFrame, pRenderPass, pipeline_type_t::compute_pipeline);
    pRenderPass->pGpuCommandBuffer->pCommandList->Dispatch(dispatchX, dispatchY, dispatchZ);
}

void setViewport(render_pass_t* pRenderPass, const int x, const int y, const int width, const int height, const float minDepth, const float maxDepth)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(width > 0);
    ASSERT_DEBUG(height > 0);
    
    pRenderPass->currentPipelineState.viewport.x = x;
    pRenderPass->currentPipelineState.viewport.y = y;
    pRenderPass->currentPipelineState.viewport.width = width;
    pRenderPass->currentPipelineState.viewport.height = height;
    pRenderPass->currentPipelineState.viewport.minDepth = minDepth;
    pRenderPass->currentPipelineState.viewport.minDepth = maxDepth;
}

void setScissor(render_pass_t* pRenderPass, const int x, const int y, const int width, const int height)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(width > 0);
    ASSERT_DEBUG(height > 0);

    pRenderPass->currentPipelineState.scissor.x = x;
    pRenderPass->currentPipelineState.scissor.y = y;
    pRenderPass->currentPipelineState.scissor.width = width;
    pRenderPass->currentPipelineState.scissor.height = height;
}

void clearColorRenderTarget(render_pass_t* pRenderPass, render_target_t* pRenderTarget, const float r, const float g, const float b, const float a)
{
    ASSERT_DEBUG(pRenderTarget != nullptr);
    ASSERT_DEBUG(isColorRenderTarget(pRenderTarget));
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderPass->isOpen);

    const FLOAT colorValues[4] = {r, g, b, a};

    applyPipelineState(pRenderPass->pGpuCommandBuffer, pRenderPass->pGraphicsFrame, pRenderPass, pipeline_type_t::graphics_pipeline);
    pRenderPass->pGpuCommandBuffer->pCommandList->ClearRenderTargetView(pRenderTarget->colorBufferDescriptorHandle.cpuDescriptorHandle, colorValues, 0, nullptr);
}

void executeRenderPass(graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass, render_pass_execution_order_t executionOrder = render_pass_execution_order_t::push_back)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(!pRenderPass->isOpen);

    if(executionOrder == render_pass_execution_order_t::push_back)
    {
        pRenderPass->pNext = nullptr;
        if(pGraphicsFrame->pLastRenderPassAdded != nullptr)
        {
            pGraphicsFrame->pLastRenderPassAdded->pNext = pRenderPass;
            pGraphicsFrame->pLastRenderPassAdded = pRenderPass;
        }
        else
        {
            pGraphicsFrame->pFirstRenderPassToExecute = pRenderPass;
            pGraphicsFrame->pLastRenderPassAdded = pRenderPass;
        }
    }
    else if(executionOrder == render_pass_execution_order_t::push_front)
    {
        pRenderPass->pNext = pGraphicsFrame->pFirstRenderPassToExecute;
        pGraphicsFrame->pFirstRenderPassToExecute = pRenderPass;
        
        if(pGraphicsFrame->pLastRenderPassAdded == nullptr)
        {
            pGraphicsFrame->pLastRenderPassAdded = pRenderPass;
        }
    }
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

resource_binding_type_t mapShaderInputType(const D3D_SHADER_INPUT_TYPE shaderInputType)
{
    switch(shaderInputType)
    {
        case D3D_SIT_CBUFFER:
            return resource_binding_type_t::constant_buffer;
        case D3D_SIT_TEXTURE:
            return resource_binding_type_t::texture;
        case D3D_SIT_SAMPLER:
            return resource_binding_type_t::sampler;
        case D3D_SIT_UAV_RWSTRUCTURED:
            return resource_binding_type_t::structured_buffer;
    }

    ASSERT_DEBUG_UNREACHABLE_CODE();
    return resource_binding_type_t::constant_buffer;
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

void copyGpuTexture(render_pass_t* pRenderPass, gpu_texture_t* pDestinationTexture, gpu_texture_t* pSourceTexture)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
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

    D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation = {};
    sourceCopyLocation.pResource = pSourceTexture->resource.pResource;
    sourceCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    sourceCopyLocation.SubresourceIndex = 0u;

    D3D12_TEXTURE_COPY_LOCATION destinationCopyLocation = {};
    destinationCopyLocation.pResource = pDestinationTexture->resource.pResource;
    destinationCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destinationCopyLocation.SubresourceIndex = 0u;

    // We have to wait for the copy pass in case the texture was created this frame and is the destination of a copy from a staging buffer
    if(pDestinationTexture->resource.flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op) || pSourceTexture->resource.flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op))
    {
        pRenderPass->currentPipelineState.resourcesAreDependingOnCopyPass = true;
    }

    pRenderPass->pGpuCommandBuffer->pCommandList->CopyTextureRegion(&destinationCopyLocation, 0u, 0u, 0u, &sourceCopyLocation, nullptr);
}

void copyGpuTextureFromBuffer(render_pass_t* pRenderPass, gpu_texture_t* pDestinationTexture, gpu_buffer_t* pSourceBuffer)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pDestinationTexture != nullptr);
    ASSERT_DEBUG(pSourceBuffer != nullptr);

    if(!Validate(pDestinationTexture->sizeInBytes == pSourceBuffer->sizeInBytes, "Trying to copy source buffer '%s' into destination texture '%s' but they don't have the same size.", pSourceBuffer->pName, pDestinationTexture->pName))
    {
        return;
    }

    updateResourceState(pRenderPass, &pSourceBuffer->resource, gpu_resource_state_flag_t::transfer_src);
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

    // We have to wait for the copy pass in case the texture was created this frame and is the destination of a copy from a staging buffer
    if(pDestinationTexture->resource.flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op) || pSourceBuffer->resource.flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op))
    {
        pRenderPass->currentPipelineState.resourcesAreDependingOnCopyPass = true;
    }

    pRenderPass->pGpuCommandBuffer->pCommandList->CopyTextureRegion(&destinationCopyLocation, 0u, 0u, 0u, &sourceCopyLocation, nullptr);
}

bool ValidateGpuBuffer(const gpu_buffer_t* pGpuBuffer)
{
    if(pGpuBuffer == nullptr)
    {
        return false;
    }

    if(pGpuBuffer->resourceFlags.isFlagSet(gpu_resource_flag_t::marked_as_free))
    {
        return false;
    }

    return true;
}

void copyGpuTexture(graphics_frame_t* pGraphicsFrame, gpu_texture_t* pDestinationTexture, gpu_texture_t* pSourceTexture)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pDestinationTexture != nullptr);
    ASSERT_DEBUG(pSourceTexture != nullptr);

    gpu_copy_operation_t* pCopyOperation = (gpu_copy_operation_t*)pushBackFromDynamicArrayDontGrow(&pGraphicsFrame->gpuCopyOperations, 1u);
    pCopyOperation->type = gpu_copy_operation_type_t::copy_texture_to_texture;
    pCopyOperation->destination.pTexture = pDestinationTexture;
    pCopyOperation->source.pTexture = pSourceTexture;

    pDestinationTexture->resource.flags |= gpu_resource_flag_t::used_in_copy_op;
    pSourceTexture->resource.flags |= gpu_resource_flag_t::used_in_copy_op;
}

void copyGpuTextureFromBuffer(graphics_frame_t* pGraphicsFrame, gpu_texture_t* pDestinationTexture, gpu_buffer_t* pSourceBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pDestinationTexture != nullptr);
    ASSERT_DEBUG(pSourceBuffer != nullptr);

    gpu_copy_operation_t* pCopyOperation = (gpu_copy_operation_t*)pushBackFromDynamicArrayDontGrow(&pGraphicsFrame->gpuCopyOperations, 1u);
    pCopyOperation->type = gpu_copy_operation_type_t::copy_buffer_to_texture;
    pCopyOperation->destination.pTexture = pDestinationTexture;
    pCopyOperation->source.pBuffer = pSourceBuffer;

    pDestinationTexture->resource.flags |= gpu_resource_flag_t::used_in_copy_op;
    pSourceBuffer->resource.flags |= gpu_resource_flag_t::used_in_copy_op;
}

void copyGpuBuffer(graphics_frame_t* pGraphicsFrame, gpu_buffer_t* pDestinationBuffer, gpu_buffer_t* pSourceBuffer)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pDestinationBuffer != nullptr);
    ASSERT_DEBUG(pSourceBuffer != nullptr);

    gpu_copy_operation_t* pCopyOperation = (gpu_copy_operation_t*)pushBackFromDynamicArrayDontGrow(&pGraphicsFrame->gpuCopyOperations, 1u);
    pCopyOperation->type = gpu_copy_operation_type_t::copy_buffer_to_buffer;
    pCopyOperation->destination.pBuffer = pDestinationBuffer;
    pCopyOperation->source.pBuffer = pSourceBuffer;

    pDestinationBuffer->resource.flags |= gpu_resource_flag_t::used_in_copy_op;
    pSourceBuffer->resource.flags |= gpu_resource_flag_t::used_in_copy_op;
}

void copyGpuBuffer(render_pass_t* pRenderPass, gpu_buffer_t* pDestinationBuffer, gpu_buffer_t* pSourceBuffer)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pDestinationBuffer != nullptr);
    ASSERT_DEBUG(pSourceBuffer != nullptr);

    if(!Validate(pSourceBuffer->sizeInBytes == pDestinationBuffer->sizeInBytes, "Trying to copy source buffer '%s' into destination buffer '%s' but they don't have the same size.", pSourceBuffer->pName, pDestinationBuffer->pName))
    {
        return;
    }

    // We have to wait for the copy pass in case the texture was created this frame and is the destination of a copy from a staging buffer
    if(pDestinationBuffer->resource.flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op) || pSourceBuffer->resource.flags.isFlagSet(gpu_resource_flag_t::used_in_copy_op))
    {
        pRenderPass->currentPipelineState.resourcesAreDependingOnCopyPass = true;
    }

    const uint32_t bufferSizeInBytes = pSourceBuffer->sizeInBytes;
    pRenderPass->pGpuCommandBuffer->pCommandList->CopyBufferRegion(pDestinationBuffer->resource.pResource, 0u, pSourceBuffer->resource.pResource, 0u, bufferSizeInBytes);
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

    descriptor_handle_t samplerDescriptorHandle = {};
    if(!allocateDescriptor(&samplerDescriptorHandle, pGraphicsFrame->pSamplerDescriptorHeap))
    {
        //freeSampler(pGraphicsFrame, pSampler);
        return nullptr;
    }

    pGraphicsFrame->pDevice->CreateSampler(&samplerDesc, samplerDescriptorHandle.cpuDescriptorHandle);
    pSampler->descriptorHandle      = samplerDescriptorHandle;
    pSampler->minifactionFilter     = pParameter->minifactionFilter;
    pSampler->magnificationFilter   = pParameter->magnificationFilter;
    pSampler->mipmapFilter          = pParameter->mipmapFilter;
    pSampler->addressModeU          = pParameter->addressModeU;
    pSampler->addressModeV          = pParameter->addressModeV;
    pSampler->addressModeW          = pParameter->addressModeW;

    return pSampler;
}

NO_DISCARD gpu_buffer_t* createGpuBuffer(graphics_frame_t* pGraphicsFrame, uint32_t sizeInBytes, const void* pInitialData, flags8_t<gpu_buffer_usage_flag_t> bufferUsageMask, gpu_memory_usage_hint_t memoryUsageHint, const char* pName = "GpuBuffer")
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
    desc.Flags              = bufferUsageMask.isFlagSet(gpu_buffer_usage_flag_t::storage_buffer) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                 = mapGpuMemoryHintToHeapType(memoryUsageHint);
    heapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    bool useStagingBuffer = (pInitialData != nullptr) && (memoryUsageHint == gpu_memory_usage_hint_t::gpuExclusiveAccess);
    const flags32_t<gpu_resource_state_flag_t> finalResourceStates = mapBufferUsageToResourceStates(bufferUsageMask);

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

    pGpuBuffer->resource.currentStateMask.clearFlags();
    pGpuBuffer->resource.pResource            = pGpuBufferResource.pPointer;
    pGpuBuffer->resource.futureStateMask      = finalResourceStates;
    pGpuBuffer->sizeInBytes                   = sizeInBytes;
    pGpuBuffer->bufferUsageMask               = bufferUsageMask;
    pGpuBuffer->memoryUsageHint               = memoryUsageHint;
    pGpuBuffer->pName                         = pName;

    if(pInitialData != nullptr)
    {
        if(!useStagingBuffer)
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
            gpu_buffer_t* pStagingBuffer = createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, gpu_buffer_usage_flag_t::transfer_src, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Staging Buffer");
            if(pStagingBuffer == nullptr)
            {
                return nullptr;
            }
            
            copyGpuBuffer(pGraphicsFrame, pGpuBuffer, pStagingBuffer);
            releaseGpuBuffer(pGraphicsFrame, pStagingBuffer);
        }   
    }

    setD3D12ObjectDebugName(pGpuBuffer->resource.pResource, pGpuBuffer->pName);
    pGpuBufferResource.takeOwnership();
    return pGpuBuffer;
}

NO_DISCARD gpu_texture_view_t* createGpuTextureView(graphics_frame_t* pGraphicsFrame, uint8_t mipMapLevels, ID3D12Resource* pResource, flags8_t<gpu_texture_usage_flag_t> textureUsageFlags, gpu_texture_format_t format, gpu_texture_format_type_t formatType)
{
    const DXGI_FORMAT dxgiFormat = mapTextureFormatToD3D12Format(format, formatType);
    ASSERT_DEBUG(dxgiFormat != DXGI_FORMAT_UNKNOWN);

    gpu_texture_view_t* pTextureView = getFreeGpuTextureView(pGraphicsFrame->pRenderResourceCache);
    if(pTextureView == nullptr)
    {
        return nullptr;
    }

    if(textureUsageFlags.isFlagSet(gpu_texture_usage_flag_t::shader_resource_view))
    {
        allocateDescriptor(&pTextureView->shaderResourceView, pGraphicsFrame->pShaderVisibleDescriptorHeap);

        D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
        shaderResourceViewDesc.Format = dxgiFormat;
        shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MipLevels = mipMapLevels;
        pGraphicsFrame->pDevice->CreateShaderResourceView(pResource, &shaderResourceViewDesc, pTextureView->shaderResourceView.cpuDescriptorHandle);
    }
    if(textureUsageFlags.isFlagSet(gpu_texture_usage_flag_t::color_render_target))
    {
        allocateDescriptor(&pTextureView->renderTargetColorView, pGraphicsFrame->pRenderTargetColorViewDescriptorHeap);
        
        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
        renderTargetViewDesc.Format = dxgiFormat;
        renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        renderTargetViewDesc.Texture2D.MipSlice = 0;
        renderTargetViewDesc.Texture2D.PlaneSlice = 0;
        pGraphicsFrame->pDevice->CreateRenderTargetView(pResource, &renderTargetViewDesc, pTextureView->renderTargetColorView.cpuDescriptorHandle);
    }
    if(textureUsageFlags.isFlagSet(gpu_texture_usage_flag_t::depth_render_target))
    {
        allocateDescriptor(&pTextureView->renderTargetDepthView, pGraphicsFrame->pRenderTargetDepthViewDescriptorHeap);

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDepth = {};
        depthStencilViewDepth.Format = dxgiFormat;
        depthStencilViewDepth.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
        depthStencilViewDepth.Texture2D.MipSlice = 0;
        pGraphicsFrame->pDevice->CreateDepthStencilView(pResource, &depthStencilViewDepth, pTextureView->renderTargetDepthView.cpuDescriptorHandle);
    }

    pTextureView->usageFlags = textureUsageFlags;
    pTextureView->format     = format;
    pTextureView->formatType = formatType;
    return pTextureView;
}

NO_DISCARD gpu_texture_view_t* createGpuTextureViewForTexture(graphics_frame_t* pGraphicsFrame, uint8_t mipMapLevels, const gpu_texture_t* pTexture, flags8_t<gpu_texture_usage_flag_t> textureUsageFlags, gpu_texture_format_t format, gpu_texture_format_type_t formatType)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pTexture != nullptr);
    ASSERT_DEBUG(textureUsageFlags.value != 0);

    if(pTexture->pView != nullptr)
    {
        if(!Validate((pTexture->pView->usageFlags & textureUsageFlags) > 0, "Texture has already a view that supports the usage flags 0x%x", textureUsageFlags.value))
        {
            return nullptr;
        }
    }

    return createGpuTextureView(pGraphicsFrame, mipMapLevels, pTexture->resource.pResource, textureUsageFlags, format, formatType);
}

NO_DISCARD gpu_texture_t* createGpuTexture(graphics_frame_t* pGraphicsFrame, uint3_t dimensions, uint8_t mipMapLevels, const void* pInitialData, flags8_t<gpu_texture_usage_flag_t> textureUsageFlags, gpu_texture_format_t format, gpu_texture_format_type_t formatType, const char* pName = "GpuTexture")
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(dimensions.x != 0);

    dimensions.y = dimensions.y == 0u ? 1u : dimensions.y;
    dimensions.z = dimensions.z == 0u ? 1u : dimensions.z;
    mipMapLevels = mipMapLevels == 0u ? 1u : mipMapLevels;

    D3D12_RESOURCE_DIMENSION resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    if(dimensions.y > 1u && dimensions.z == 1u)
    {
        resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    }
    if(dimensions.y > 1u && dimensions.z > 1u)
    {
        resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }

    const DXGI_FORMAT dxgiFormat = mapTextureFormatToD3D12Format(format, formatType);
    ASSERT_DEBUG(dxgiFormat != DXGI_FORMAT_UNKNOWN);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = resourceDimension;
    desc.Alignment          = 0u;
    desc.Format             = dxgiFormat;
    desc.DepthOrArraySize   = dimensions.z;
    desc.Height             = dimensions.y;
    desc.Width              = dimensions.x;
    desc.MipLevels          = mipMapLevels;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.SampleDesc.Count   = 1u;
    desc.SampleDesc.Quality = 0u;
    
    if(textureUsageFlags.isFlagSet(gpu_texture_usage_flag_t::color_render_target))
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }

    if(textureUsageFlags.isFlagSet(gpu_texture_usage_flag_t::depth_render_target))
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
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

    pGpuTexture->resource.pResource            = pGpuTextureResource.pPointer;
    pGpuTexture->dimensions                    = dimensions;
    pGpuTexture->format                        = format;
    pGpuTexture->formatType                    = formatType;
    pGpuTexture->usageFlags                    = textureUsageFlags;
    pGpuTexture->sizeInBytes                   = sizeInBytes;
    pGpuTexture->pName                         = pName;

    pGpuTexture->pView = createGpuTextureViewForTexture(pGraphicsFrame, mipMapLevels, pGpuTexture, textureUsageFlags, format, formatType);
    if(pGpuTexture->pView == nullptr)
    {
        releaseGpuTexture(pGraphicsFrame, pGpuTexture);
        return nullptr;
    }
    
    if(pInitialData != nullptr)
    {
        gpu_buffer_t* pStagingBuffer = createGpuBuffer(pGraphicsFrame, sizeInBytes, pInitialData, gpu_buffer_usage_flag_t::transfer_src, gpu_memory_usage_hint_t::cpuWriteGpuReadAccess, "Staging Texture Buffer");
        if(pStagingBuffer == nullptr)
        {
            return nullptr;
        }
        
        copyGpuTextureFromBuffer(pGraphicsFrame, pGpuTexture, pStagingBuffer);
        releaseGpuBuffer(pGraphicsFrame, pStagingBuffer);
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

    //trackMappedGpuBufferForValidation(pGraphicsFrame, pGpuBuffer);

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

    //untrackMappedGpuBufferForValidation(pGraphicsFrame, pGpuBuffer);

    pGpuBuffer->resource.pResource->Unmap(0u, nullptr);
    pGpuBuffer->flags.clearFlag(gpu_buffer_flag_t::is_mapped);
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

void freeCompilationResult(memory_allocator_t* pMemoryAllocator, shader_compilation_result_t* pShaderCompilationResult)
{
    if(pShaderCompilationResult->pWarningMessage != nullptr)
    {
        freeFromAllocator(pMemoryAllocator, (void*)pShaderCompilationResult->pWarningMessage);
        pShaderCompilationResult->pWarningMessage = nullptr;
    }

    if(pShaderCompilationResult->pErrorMessage != nullptr)
    {
        freeFromAllocator(pMemoryAllocator, (void*)pShaderCompilationResult->pErrorMessage);
        pShaderCompilationResult->pErrorMessage = nullptr;
    }
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

const char* createShaderProfileName(memory_allocator_t* pMemoryAllocator, const shader_type_t shaderType, const shader_model_t shaderModel)
{
    char* pShaderProfileName = (char*)allocateFromAllocator(pMemoryAllocator, 7u, alloc_flags_t::clear_memory);
    if(pShaderProfileName == nullptr)
    {
        return nullptr;
    }

    switch(shaderType)
    {
        case shader_type_t::vertex_shader:
            strcat_s(pShaderProfileName, 7u, "vs_");
            break;
        case shader_type_t::pixel_shader:
            strcat_s(pShaderProfileName, 7u, "ps_");
            break;
        case shader_type_t::compute_shader:
            strcat_s(pShaderProfileName, 7u, "cs_");
            break;
        default:
            ASSERT_DEBUG_UNREACHABLE_CODE();
            break;
    }

    switch(shaderModel)
    {
        case shader_model_t::model_6_0:
            strcat_s(pShaderProfileName, 7u, "6_0");
            break;
        default:
            ASSERT_DEBUG_UNREACHABLE_CODE();
            break;
    }

    return pShaderProfileName;
}

const char* copyFormattedString(memory_allocator_t* pMemoryAllocator, const char* pFormattedString, ...)
{
    char localTempBuffer[1024] = {};

    va_list vaList;
    va_start(vaList, pFormattedString);
    const int charsWritten = vsprintf_s(localTempBuffer, sizeof(localTempBuffer), pFormattedString, vaList);
    va_end(vaList);

    char* pStringCopyBuffer = (char*)allocateFromAllocator(pMemoryAllocator, charsWritten + 1);
    if(pStringCopyBuffer == nullptr)
    {
        return nullptr;
    }

    copyMemoryNonOverlapping(pStringCopyBuffer, localTempBuffer, charsWritten);
    pStringCopyBuffer[charsWritten] = 0;
    return pStringCopyBuffer;
}

void* allocateBufferCopy(memory_allocator_t* pMemoryAllocator, const void* pBufferData, const uint64_t bufferSizeInBytes)
{
    void* pBufferCopy = allocateFromAllocator(pMemoryAllocator, bufferSizeInBytes);
    if(pBufferCopy == nullptr)
    {
        return nullptr;
    }

    copyMemoryNonOverlapping(pBufferCopy, pBufferData, bufferSizeInBytes);
    return pBufferCopy;
}

NO_DISCARD shader_compilation_result_t compileShaderCodeInternally(graphics_frame_t* pGraphicsFrame, const char* pShaderCode, const uint64_t shaderCodeLength, const shader_compilation_parameters_t* pCompilationParameters)
{
    DxcBuffer shaderSourceBuffer = {};
    shaderSourceBuffer.Ptr = pShaderCode;
    shaderSourceBuffer.Size = shaderCodeLength;

    shader_compilation_result_t compilationResult = {};

    result_t<dxc_arguments_t> compileArgumentsResult = generateCompilerArgumentsIntoNewBuffer(pGraphicsFrame->pFrameAllocator, pCompilationParameters);
    if(!isResultSuccessful(compileArgumentsResult))
    {
        compilationResult.result = compileArgumentsResult.status;
        return compilationResult;
    }

    com_auto_release_t<IDxcResult> pCompileResult = nullptr;

    shader_compiler_context_t* pShaderCompilerContext = pGraphicsFrame->pShaderCompilerContext;
    const HRESULT compileResult = COM_CALL(pShaderCompilerContext->pShaderCompiler->Compile(&shaderSourceBuffer, compileArgumentsResult.value.ppArguments, compileArgumentsResult.value.argumentCount, pShaderCompilerContext->pIncludeHandler, IID_PPV_ARGS(&pCompileResult)));
    freeCompilerArguments(pGraphicsFrame->pFrameAllocator, &compileArgumentsResult.value);

    if(compileResult != S_OK)
    {
        compilationResult.result = result_status_t::compilation_error;
        return compilationResult;
    }

    if(pCompileResult->HasOutput(DXC_OUT_ERRORS))
    {
        com_auto_release_t<IDxcBlobEncoding> pErrorBufferEncoding = nullptr;
        const HRESULT getErrorBufferResult = COM_CALL(pCompileResult->GetErrorBuffer(&pErrorBufferEncoding));
        if(getErrorBufferResult != S_OK)
        {
            compilationResult.result = result_status_t::compilation_error;
            return compilationResult;
        }

        if(pErrorBufferEncoding != nullptr && pErrorBufferEncoding->GetBufferSize() > 0)
        {
            compilationResult.result = result_status_t::compilation_error;
            compilationResult.pErrorMessage = (const char*)allocateBufferCopy(pGraphicsFrame->pFrameAllocator, pErrorBufferEncoding->GetBufferPointer(), pErrorBufferEncoding->GetBufferSize());
            return compilationResult;
        }
    }

    com_auto_release_t<IDxcBlob> pCompileShaderBlob = nullptr;
    const HRESULT getBlobOutputResult = COM_CALL(pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pCompileShaderBlob), nullptr));
    if(getBlobOutputResult != S_OK)
    {
        compilationResult.result = result_status_t::compilation_error;
        return compilationResult;
    }

    DxcBuffer compiledShaderBuffer = {};
    compiledShaderBuffer.Ptr = pCompileShaderBlob->GetBufferPointer();
    compiledShaderBuffer.Size = pCompileShaderBlob->GetBufferSize();

    com_auto_release_t<ID3D12ShaderReflection> pShaderReflection = nullptr;
    const HRESULT shaderReflectionResult = COM_CALL(pShaderCompilerContext->pUtils->CreateReflection(&compiledShaderBuffer, IID_PPV_ARGS(&pShaderReflection)));
    if(shaderReflectionResult != S_OK)
    {
        compilationResult.result = result_status_t::reflection_error;
        return compilationResult;
    }

    D3D12_SHADER_DESC shaderDesc = {};
    pShaderReflection->GetDesc(&shaderDesc);

    shader_binding_point_t shaderBindingPoints[maxShaderBindingPoints] = {};

    uint32_t shaderresourceBindingCount = shaderDesc.BoundResources;
    if(shaderresourceBindingCount > maxShaderBindingPoints)
    {
        compilationResult.pWarningMessage = copyFormattedString(pGraphicsFrame->pFrameAllocator, "Shader '%s' has more bound resources than %u, some won't be accessible.", pCompilationParameters->pShaderName, maxShaderBindingPoints);
        shaderresourceBindingCount = maxShaderBindingPoints;
    }

    for(uint32_t boundResourceIndex = 0u; boundResourceIndex < shaderresourceBindingCount; ++boundResourceIndex)
    {
        D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc = {};
        const HRESULT resourceBindDescResult = COM_CALL(pShaderReflection->GetResourceBindingDesc(boundResourceIndex, &shaderInputBindDesc));
        if(resourceBindDescResult != S_OK)
        {
            compilationResult.pErrorMessage = copyFormattedString(pGraphicsFrame->pFrameAllocator, "Could not reflect shader '%s' bound resource '%u' - %s", pCompilationParameters->pShaderName, getHResultString(resourceBindDescResult));
            compilationResult.result = result_status_t::reflection_error;
            return compilationResult;
        }

        strncpy(shaderBindingPoints[boundResourceIndex].name, shaderInputBindDesc.Name, maxShaderBindingPointNameLength);
        shaderBindingPoints[boundResourceIndex].slot                = rangeCheckCast<uint16_t>(shaderInputBindDesc.BindPoint);
        shaderBindingPoints[boundResourceIndex].space               = rangeCheckCast<uint16_t>(shaderInputBindDesc.Space);
        shaderBindingPoints[boundResourceIndex].type                = mapShaderInputType(shaderInputBindDesc.Type);
        shaderBindingPoints[boundResourceIndex].shaderAccessMask    = pCompilationParameters->shaderType;
    }

    const uint32_t shaderBlobSizeInBytes = rangeCheckCast<uint32_t>(pCompileShaderBlob->GetBufferSize());
    uint8_t* pShaderBlobCopy = (uint8_t*)allocateFromAllocator(pGraphicsFrame->pMemoryAllocator, pCompileShaderBlob->GetBufferSize());
    if(pShaderBlobCopy == nullptr)
    {
        compilationResult.pErrorMessage = copyFormattedString(pGraphicsFrame->pFrameAllocator, "Shader compilation of shader '%s' was successful but we ran out of memory trying to copy the shader blob.", pCompilationParameters->pShaderName);
        compilationResult.result = result_status_t::out_of_memory;
        return compilationResult;
    }

    memcpy(pShaderBlobCopy, pCompileShaderBlob->GetBufferPointer(), pCompileShaderBlob->GetBufferSize());

    shader_binary_t* pShaderBinary = getFreeShaderBinary(pGraphicsFrame->pRenderResourceCache);
    if(pShaderBinary == nullptr)
    {
        freeFromAllocator(pGraphicsFrame->pMemoryAllocator, pShaderBlobCopy);
        compilationResult.result = result_status_t::out_of_memory;
        return compilationResult;
    }

    memcpy(pShaderBinary->bindingPoints, shaderBindingPoints, sizeof(shaderBindingPoints));
    pShaderBinary->bindingPointCount        = shaderresourceBindingCount;
    pShaderBinary->pShaderBlob              = pShaderBlobCopy;
    pShaderBinary->shaderBlobSizeInBytes    = shaderBlobSizeInBytes;

    compilationResult.result = result_status_t::success;
    compilationResult.pShaderBinary = pShaderBinary;
    return compilationResult;
}

NO_DISCARD shader_binary_t* compileShaderCode(graphics_frame_t* pGraphicsFrame, const char* pShaderCode, const uint64_t shaderCodeLength, const char* pEntryPoint, const char* pDefines, const char* pShaderName, const shader_type_t shaderType, const shader_model_t shaderModel = shader_model_t::model_6_0)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pShaderCode != nullptr);
    ASSERT_DEBUG(pEntryPoint != nullptr);
    ASSERT_DEBUG(pShaderName != nullptr);
    ASSERT_DEBUG(shaderCodeLength > 0);

    shader_compilation_parameters_t compilationParameters = {};
    compilationParameters.pEntryPoint = pEntryPoint;
    compilationParameters.pShaderProfile = createShaderProfileName(pGraphicsFrame->pFrameAllocator, shaderType, shaderModel);
    compilationParameters.pDefines = pDefines;
    compilationParameters.pShaderName = pShaderName;

    shader_compilation_result_t compilationResult = compileShaderCodeInternally(pGraphicsFrame, pShaderCode, shaderCodeLength, &compilationParameters);
    if(compilationResult.pErrorMessage != nullptr)
    {
        logError(compilationResult.pErrorMessage);
    }
    else if(compilationResult.pWarningMessage != nullptr)
    {
        logWarning(compilationResult.pWarningMessage);
    }

    freeCompilationResult(pGraphicsFrame->pFrameAllocator, &compilationResult);
    return compilationResult.pShaderBinary;
}

NO_DISCARD shader_binary_t* loadAndCompileShaderCodeFromFile(graphics_frame_t* pGraphicsFrame, const char* pShaderFilePath, const char* pEntryPoint, const char* pDefines, const char* pShaderName, const shader_type_t shaderType, const shader_model_t shaderModel = shader_model_t::model_6_0)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pShaderFilePath != nullptr);
    ASSERT_DEBUG(pEntryPoint != nullptr);
    ASSERT_DEBUG(pDefines != nullptr);
    ASSERT_DEBUG(pShaderName != nullptr);

    result_t<memory_buffer_t> shaderCodeResult = readWholeFileIntoNewBuffer(pGraphicsFrame->pFrameAllocator, pShaderFilePath);
    if(!isResultSuccessful(shaderCodeResult))
    {
        logError("Could not read shader file '%s' - error: %s.", pShaderFilePath, getResultString(shaderCodeResult));
        return nullptr;
    }
    
    shader_compilation_parameters_t compilationParameters = {};
    compilationParameters.pEntryPoint = pEntryPoint;
    compilationParameters.pShaderProfile = createShaderProfileName(pGraphicsFrame->pFrameAllocator, shaderType, shaderModel);
    compilationParameters.pDefines = pDefines;
    compilationParameters.pShaderName = pShaderName;

    shader_compilation_result_t compilationResult = compileShaderCodeInternally(pGraphicsFrame, (const char*)shaderCodeResult.value.pData, shaderCodeResult.value.sizeInBytes, &compilationParameters);
    if(compilationResult.pErrorMessage != nullptr)
    {
        logError(compilationResult.pErrorMessage);
    }
    else if(compilationResult.pWarningMessage != nullptr)
    {
        logWarning(compilationResult.pWarningMessage);
    }

    freeCompilationResult(pGraphicsFrame->pFrameAllocator, &compilationResult);
    freeFromAllocator(pGraphicsFrame->pFrameAllocator, shaderCodeResult.value.pData);
    return compilationResult.pShaderBinary;
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
    ASSERT_DEBUG(pRenderContext != nullptr);

    destroyGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection);
    destroySwapChain(&pRenderContext->swapChain);
    destroyRenderResourceCache(&pRenderContext->renderResourceCache);
    destroyDescriptorHeap(&pRenderContext->shaderVisibleDescriptorHeap);
    destroyDescriptorHeap(&pRenderContext->samplerDescriptorHeap);

    const bool debugEnabled = (pRenderContext->pDebugLayer != nullptr);
    ID3D12DebugDevice* pDebugDevice = nullptr;
    pRenderContext->pDevice->QueryInterface(IID_PPV_ARGS(&pDebugDevice));

    const uint32_t queueCount = (uint32_t)gpu_pass_type_t::count;
    for(uint32_t queueIndex = 0u; queueIndex < queueCount; ++queueIndex)
    {
        COM_RELEASE(pRenderContext->ppCommandQueues[queueIndex]);
    }

    freeFromAllocator(&pRenderContext->defaultAllocator, pRenderContext->ppCommandQueues);
    pRenderContext->ppCommandQueues = nullptr;

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
    ASSERT_DEBUG(pRenderContext != nullptr);

    flushAllFrames(pRenderContext);

    for(uint32_t bufferIndex = 0u; bufferIndex < pRenderContext->swapChain.backBufferCount; ++bufferIndex)
    {
        COM_RELEASE(pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex].colorTextureResource.pResource);
    }

    pRenderContext->swapChain.pSwapChain->ResizeBuffers(0u, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    pRenderContext->swapChain.width = width;
    pRenderContext->swapChain.height = height;

    resetDescriptorHeap(&pRenderContext->swapChain.backBufferRenderTargetDescriptorHeap);

    for(uint32_t bufferIndex = 0u; bufferIndex < pRenderContext->swapChain.backBufferCount; ++bufferIndex)
    {
        ID3D12Resource* pFrameBuffer = nullptr;
        COM_CALL(pRenderContext->swapChain.pSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pFrameBuffer)));

        descriptor_handle_t colorTargetDescriptorHandle = {};
        allocateDescriptor(&colorTargetDescriptorHandle, &pRenderContext->swapChain.backBufferRenderTargetDescriptorHeap);

        pRenderContext->pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, colorTargetDescriptorHandle.cpuDescriptorHandle);
        pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex].dimensions                              = createUint3(width, height, 1u);
        pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex].colorTextureResource.pResource          = pFrameBuffer;
        pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex].colorTextureResource.currentStateMask   = gpu_resource_state_flag_t::present;
        pRenderContext->swapChain.pBackBufferRenderTargets[bufferIndex].colorBufferDescriptorHandle             = colorTargetDescriptorHandle;
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
    parameters.limits.maxGpuTextureViewCount            = 32u;
    parameters.limits.maxRenderPassCount                = 32u;
    parameters.limits.maxComputePassCount               = 32u;
    parameters.limits.maxCopyPassCount                  = 8u;
    parameters.limits.maxGraphicsPipelineCount          = 32u;
    parameters.limits.maxComputePipelineCount           = 32u;
    parameters.limits.maxRenderTargetCount              = 32u;
    parameters.limits.maxShaderBinaryCount              = 32u;
    parameters.limits.maxVertexFormatCount              = 32u;
    parameters.limits.maxSamplerCount                   = 32u;
    return parameters;
}