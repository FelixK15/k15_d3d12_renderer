#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#define D3DCOMPILE_DEBUG 1
#define USE_D3D12_DEBUG 1
#define CLEAR_NEW_MEMORY_WITH_ZEROES 1
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>

#include <stdio.h>
#include <stdint.h>

#include "include/WinPixEventRuntime/pix3.h"

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "bin/WinPixEventRuntime.lib")

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#if USE_D3D12_DEBUG
#define COM_CALL(func) logOnHResultError(func, #func, __FILE__, __LINE__)
#else
#define COM_CALL(func) func
#endif

#define COM_RELEASE(ptr)if(ptr != nullptr) { (ptr)->Release(); ptr = nullptr; }
#define ASSERT_DEBUG_MSG(x, msg)                \
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

#define ASSERT_DEBUG(x) ASSERT_DEBUG_MSG(x, nullptr)
#define ASSERT_DEBUG_UNREACHABLE_CODE() ASSERT_DEBUG(false)

struct memory_allocator_t;
struct render_context_t;

typedef void*(*allocate_from_memory_allocator_fnc)(memory_allocator_t*, uint64_t, uint64_t);
typedef void(*free_from_memory_allocator_fnc)(memory_allocator_t*, void*);

typedef ID3D12Device10  D3D12DeviceType;
typedef ID3D12Debug6    D3D12DebugType;
typedef IDXGIFactory7   DXGIFactoryType;
typedef IDXGISwapChain4 DXGISwapChainType;

enum alloc_flags_t
{
    none         = 0x0,
    clear_memory = 0x1
};

struct memory_allocator_t
{
    allocate_from_memory_allocator_fnc  allocateFnc;
    free_from_memory_allocator_fnc      freeFnc;
};

constexpr uint64_t defaultAllocationAlignment   = 16u;
constexpr uint32_t frameBufferCount             = 2u;

struct d3d12_resource_t
{
    ID3D12Resource* pResource;
    D3D12_RESOURCE_STATES currentState;
};

struct render_target_t
{
    d3d12_resource_t            resource;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
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
    uint8_t                         backBufferCount;

};

struct render_pass_t
{
    ID3D12CommandAllocator*     pGraphicsCommandAllocator;
    ID3D12GraphicsCommandList*  pGraphicsCommandList;
    const char*                 pName;
    bool                        isOpen;
    render_pass_t*              pNext;
};

struct graphics_frame_t
{
    render_context_t*                       pRenderContext;
    render_target_t*                        pBackBuffer;
    render_pass_t*                          pRenderPassBuffer;
    memory_allocator_t*                     pMemoryAllocator;
    render_pass_t*                          pFirstRenderPassToExecute;
    render_pass_t*                          pLastRenderPassToExecute;
    uint64_t                                frameIndex;
    uint32_t                                renderPassIndex;
    uint32_t                                renderPassCount;
    uint32_t                                openRenderPassCount;
    ID3D12Fence*                            pFrameFence;
    ID3D12GraphicsCommandList*              pFrameGeneralCopyQueue;
    ID3D12GraphicsCommandList*              pFrameGeneralGraphicsQueue;
    ID3D12CommandAllocator*                 pFrameGeneralCopyCommandAllocator;
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
    uint32_t maxDirectAllocatorCount;
    uint32_t maxCopyAllocatorCount;
    uint32_t maxComputeAllocatorCount;
};

struct render_context_t
{
    D3D12DeviceType*            pDevice;
    D3D12DebugType*             pDebugLayer;
    DXGIFactoryType*            pFactory;
    
    memory_allocator_t          defaultAllocator;
    graphics_frame_collection_t graphicsFramesCollection;
    const graphics_frame_t*     pCurrentGraphicsFrame;

    d3d12_swap_chain_t          swapChain;
    ID3D12CommandQueue*         pDefaultCommandQueue;

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

assert_result_t handleAssert(const char* pExpression, const char* pUserMessage)
{
    char messageBuffer[1024] = {0};
    if(pUserMessage == nullptr)
    {
        sprintf_s(messageBuffer, sizeof(messageBuffer), "Error in Expression '%s'\n\nPress Try to continue\nPress Continue to debug\nPress Cancel to exit application.", pExpression);
    }
    else
    {
        sprintf_s(messageBuffer, sizeof(messageBuffer), "Error in Expression '%s'\n===============\n%s\n===============\n\nPress Try to continue\nPress Continue to debug\nPress Cancel to exit application.", pExpression, pUserMessage);
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
        printf("Error during call '%s' in %s:%u\nError:%s\n", pFunctionCall, pFile, lineNumber, getHResultString(originalResult));
    }

    return originalResult;
}

void* allocateFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, alloc_flags_t flags = alloc_flags_t::none)
{
    void* pMemory = pAllocator->allocateFnc(pAllocator, sizeInBytes, defaultAllocationAlignment);
    
#if FORCE_CLEAR_ALLOCATIONS
    const bool clearMemory = true;
#else
    const bool clearMemory = (flags & alloc_flags_t::clear_memory);
#endif

    if(pMemory && clearMemory)
    {
        memset(pMemory, 0, sizeInBytes);
    }

    return pMemory;
}

void* allocateAlignedFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, uint64_t alignmentInBytes, alloc_flags_t flags = alloc_flags_t::none)
{
    void* pMemory = pAllocator->allocateFnc(pAllocator, sizeInBytes, alignmentInBytes);
    
#if FORCE_CLEAR_ALLOCATIONS
    const bool clearMemory = true;
#else
    const bool clearMemory = (flags & alloc_flags_t::clear_memory);
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

void printErrorToFile(const char* p_FileName)
{
	DWORD errorId = GetLastError();
	char* textBuffer = 0;
	DWORD writtenChars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, errorId, 
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&textBuffer, 512, 0);

	if (writtenChars > 0)
	{
		FILE* file = fopen(p_FileName, "w");

		if (file)
		{
			fwrite(textBuffer, writtenChars, 1, file);			
			fflush(file);
			fclose(file);
		}
	}
}

void allocateDebugConsole()
{
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	freopen("CONOUT$", "w", stdout);
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

void flushFrame(graphics_frame_t* pGraphicsFrame)
{
    WaitForSingleObject(pGraphicsFrame->pFrameFinishedEvent, INFINITE);
    SetEvent(pGraphicsFrame->pFrameFinishedEvent);
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

void windowResized(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    render_context_t* pRenderContext = (render_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pRenderContext == nullptr)
        return;

    const uint32_t newWidth = LOWORD(lparam);
    const uint32_t newHeight = HIWORD(lparam);

    flushAllFrames(pRenderContext);

    for(uint32_t bufferIndex = 0u; bufferIndex < pRenderContext->swapChain.backBufferCount; ++bufferIndex)
    {
        destroyRenderTarget(&pRenderContext->swapChain.pBackBuffers[bufferIndex]);
    }

    pRenderContext->swapChain.pSwapChain->ResizeBuffers(0u, newWidth, newHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

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

LRESULT CALLBACK D3D12TestAppWindowProc(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{
	bool messageHandled = false;

	switch (p_Message)
	{
	case WM_CLOSE:
        DestroyWindow(p_HWND);
		messageHandled = true;
		break;

    case WM_DESTROY:
		PostQuitMessage(0);
		messageHandled = true;
        break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		break;

	case WM_MOUSEMOVE:
		break;

	case WM_MOUSEWHEEL:
		break;
    
    case WM_SIZE:
        windowResized(p_HWND, p_Message, p_wParam, p_lParam);
        messageHandled = true;
        break;
	}

	if (messageHandled == false)
	{
		return DefWindowProc(p_HWND, p_Message, p_wParam, p_lParam);
	}

	return 0;
}

HWND setupWindow(HINSTANCE p_Instance, int p_Width, int p_Height)
{
	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.hInstance = p_Instance;
	wndClass.lpszClassName = "D3D12RenderWindow";
	wndClass.lpfnWndProc = D3D12TestAppWindowProc;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("D3D12RenderWindow", "D3D12 Renderer",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		p_Width, p_Height, 0, 0, p_Instance, 0);

	if (hwnd == INVALID_HANDLE_VALUE)
		MessageBox(0, "Error creating Window.\n", "Error!", 0);
	else
		ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}

uint32_t getTimeInMilliseconds(LARGE_INTEGER p_PerformanceFrequency)
{
	LARGE_INTEGER appTime = {0};
	QueryPerformanceFrequency(&appTime);

	appTime.QuadPart *= 1000; //to milliseconds

	return (uint32_t)(appTime.QuadPart / p_PerformanceFrequency.QuadPart);
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

bool createDirectCommandQueue(D3D12DeviceType* pDevice, ID3D12CommandQueue** pOutCommandQueue)
{
    D3D12_COMMAND_QUEUE_DESC defaultCommandQueueDesc = {};
    defaultCommandQueueDesc.Type        = D3D12_COMMAND_LIST_TYPE_DIRECT;
    defaultCommandQueueDesc.Priority    = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    defaultCommandQueueDesc.Flags       = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if(COM_CALL(pDevice->CreateCommandQueue(&defaultCommandQueueDesc, IID_PPV_ARGS(pOutCommandQueue))) != S_OK)
    {
        return false;
    }

    (*pOutCommandQueue)->SetName(L"D3D12 Default Command Queue");

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

    for(uint32_t renderPassIndex = 0u; renderPassIndex < pGraphicsFrame->renderPassCount; ++renderPassIndex)
    {
        COM_RELEASE(pGraphicsFrame->pRenderPassBuffer[renderPassIndex].pGraphicsCommandList);
        COM_RELEASE(pGraphicsFrame->pRenderPassBuffer[renderPassIndex].pGraphicsCommandAllocator);
    }

    freeFromAllocator(pGraphicsFrame->pMemoryAllocator, pGraphicsFrame->pRenderPassBuffer);

    COM_RELEASE(pGraphicsFrame->pFrameGeneralCopyCommandAllocator);
    COM_RELEASE(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator);
    COM_RELEASE(pGraphicsFrame->pFrameGeneralGraphicsQueue);
    COM_RELEASE(pGraphicsFrame->pFrameGeneralCopyQueue);
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

bool createGraphicsFrame(graphics_frame_t* pOutGraphicFrame, const graphics_frame_parameters_t* pGraphicsFrameParameters, memory_allocator_t* pMemoryAllocator, D3D12DeviceType* pDevice)
{
    ASSERT_DEBUG(pGraphicsFrameParameters != nullptr);
    ASSERT_DEBUG(pMemoryAllocator != nullptr);
    ASSERT_DEBUG(pDevice != nullptr);
    ASSERT_DEBUG(pGraphicsFrameParameters->maxRenderPassCount > 0u);
    ASSERT_DEBUG(pGraphicsFrameParameters->maxComputeAllocatorCount > 0u);
    ASSERT_DEBUG(pGraphicsFrameParameters->maxCopyAllocatorCount > 0u);
    ASSERT_DEBUG(pGraphicsFrameParameters->maxDirectAllocatorCount > 0u);

    graphics_frame_t graphicsFrame = {0};
    graphicsFrame.pMemoryAllocator = pMemoryAllocator;
    graphicsFrame.pFrameFinishedEvent = CreateEvent(nullptr, FALSE, TRUE, "");
    if(graphicsFrame.pFrameFinishedEvent == nullptr)
    {
        return false;
    }

    render_pass_t* pRenderPasses = (render_pass_t*)allocateFromAllocator(pMemoryAllocator, (sizeof(render_pass_t) * pGraphicsFrameParameters->maxRenderPassCount), alloc_flags_t::clear_memory);
    if(pRenderPasses == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &graphicsFrame.pFrameGeneralGraphicsCommandAllocator))
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandAllocator(pDevice, D3D12_COMMAND_LIST_TYPE_COPY, &graphicsFrame.pFrameGeneralCopyCommandAllocator))
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, graphicsFrame.pFrameGeneralGraphicsCommandAllocator, &graphicsFrame.pFrameGeneralGraphicsQueue))
    {
        goto cleanup_and_exit_failure;
    }

    if(!createCommandList(pDevice, D3D12_COMMAND_LIST_TYPE_COPY, graphicsFrame.pFrameGeneralCopyCommandAllocator, &graphicsFrame.pFrameGeneralCopyQueue))
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

    graphicsFrame.pRenderPassBuffer = pRenderPasses;
    graphicsFrame.renderPassCount   = pGraphicsFrameParameters->maxRenderPassCount;

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

bool createGraphicsFrameCollection(graphics_frame_collection_t* pOutGraphicFrameCollection, memory_allocator_t* pMemoryAllocator, D3D12DeviceType* pDevice, const uint8_t frameCount)
{
    graphics_frame_collection_t graphicFrameCollection = {};
    graphicFrameCollection.pMemoryAllocator = pMemoryAllocator;
    graphicFrameCollection.frameCount       = frameCount;
    graphicFrameCollection.pGraphicsFrames  = (graphics_frame_t*)allocateFromAllocator(pMemoryAllocator, sizeof(graphics_frame_t) * frameCount);
    if(graphicFrameCollection.pGraphicsFrames == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    for(uint32_t frameIndex = 0u; frameIndex < frameBufferCount; ++frameIndex)
    {
        graphics_frame_parameters_t graphicsFrameParameters = {};
        graphicsFrameParameters.maxRenderPassCount          = 64u;
        graphicsFrameParameters.maxComputeAllocatorCount    = 32u;
        graphicsFrameParameters.maxDirectAllocatorCount     = 128u;
        graphicsFrameParameters.maxCopyAllocatorCount       = 8u;
        if(!createGraphicsFrame(&graphicFrameCollection.pGraphicsFrames[frameIndex], &graphicsFrameParameters, pMemoryAllocator, pDevice))
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

bool initializeRenderContext(render_context_t* pRenderContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, const uint32_t frameBufferCount, bool useDebugLayer)
{
    createDefaultMemoryAllocator(&pRenderContext->defaultAllocator);

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

    if(!createDirectCommandQueue(pRenderContext->pDevice, &pRenderContext->pDefaultCommandQueue))
    {
        return false;
    }

    if(!createSwapChain(&pRenderContext->swapChain, &pRenderContext->defaultAllocator, pRenderContext->pFactory, pRenderContext->pDevice, pRenderContext->pDefaultCommandQueue, pWindowHandle, windowWidth, windowHeight, frameBufferCount))
    {
        return false;
    }

    if(!createGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, &pRenderContext->defaultAllocator, pRenderContext->pDevice, frameBufferCount))
    {
        return false;
    }

    if(useDebugLayer)
    {
        if(!setupD3D12DebugLayer(pRenderContext->pDevice))
        {

        }
    }

    return true;
}

bool setup(render_context_t* pRenderContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, bool useDebugLayer)
{
    if(!initializeRenderContext(pRenderContext, pWindowHandle, windowWidth, windowHeight, frameBufferCount, useDebugLayer))
    {
        return false;
    }
    SetWindowLongPtrA(pWindowHandle, GWLP_USERDATA, (LONG_PTR)pRenderContext);
    return true;
}

graphics_frame_t* beginNextFrame(render_context_t* pRenderContext)
{
    ASSERT_DEBUG(pRenderContext != nullptr);
    ASSERT_DEBUG(pRenderContext->pCurrentGraphicsFrame == nullptr);
    const uint64_t frameIndex = pRenderContext->frameIndex % pRenderContext->graphicsFramesCollection.frameCount;

    graphics_frame_t* pGraphicsFrame = getGraphicsFrameFromGraphicsFrameCollection(&pRenderContext->graphicsFramesCollection, frameIndex);

    const DWORD waitResult = WaitForSingleObject(pGraphicsFrame->pFrameFinishedEvent, INFINITE);
    ASSERT_DEBUG(waitResult == WAIT_OBJECT_0);

    COM_CALL(pGraphicsFrame->pFrameGeneralCopyCommandAllocator->Reset());
    COM_CALL(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator->Reset());
    COM_CALL(pGraphicsFrame->pFrameGeneralGraphicsQueue->Reset(pGraphicsFrame->pFrameGeneralGraphicsCommandAllocator, nullptr));
    COM_CALL(pGraphicsFrame->pFrameGeneralCopyQueue->Reset(pGraphicsFrame->pFrameGeneralCopyCommandAllocator, nullptr));

    const uint32_t currentBackBufferIndex = pRenderContext->swapChain.pSwapChain->GetCurrentBackBufferIndex();

    pRenderContext->pCurrentGraphicsFrame = pGraphicsFrame;
    pGraphicsFrame->frameIndex = pRenderContext->frameIndex;
    pGraphicsFrame->pBackBuffer = pRenderContext->swapChain.pBackBuffers + currentBackBufferIndex;
    ++pRenderContext->frameIndex;
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
    pGraphicsFrame->pFrameGeneralCopyQueue->Close();
    pRenderContext->pDefaultCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pGraphicsFrame->pFrameGeneralGraphicsQueue);

    render_pass_t* pRenderPass = pGraphicsFrame->pFirstRenderPassToExecute;
    while(pRenderPass)
    {
        pRenderContext->pDefaultCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pRenderPass->pGraphicsCommandList);
        pRenderPass = pRenderPass->pNext;
    }
    
    pGraphicsFrame->pFirstRenderPassToExecute = nullptr;
    pGraphicsFrame->pLastRenderPassToExecute = nullptr;
    pGraphicsFrame->renderPassIndex = 0;

    COM_CALL(pRenderContext->swapChain.pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    COM_CALL(pRenderContext->pDefaultCommandQueue->Signal(pGraphicsFrame->pFrameFence, pGraphicsFrame->frameIndex));
    COM_CALL(pGraphicsFrame->pFrameFence->SetEventOnCompletion(pGraphicsFrame->frameIndex, pGraphicsFrame->pFrameFinishedEvent));

    pRenderContext->pCurrentGraphicsFrame = nullptr;
}

render_pass_t* startRenderPass(graphics_frame_t* pGraphicsFrame, const char* pRenderPassName)
{
    ASSERT_DEBUG(pGraphicsFrame != nullptr);

    const uint32_t nextRenderPassIndex = (pGraphicsFrame->renderPassIndex + 1);
    if(nextRenderPassIndex >= pGraphicsFrame->renderPassCount)
    {
        return nullptr;
    }

    render_pass_t* pRenderPass = pGraphicsFrame->pRenderPassBuffer + pGraphicsFrame->renderPassIndex;
    pGraphicsFrame->renderPassIndex = nextRenderPassIndex;
    ++pGraphicsFrame->openRenderPassCount;
    
    COM_CALL(pRenderPass->pGraphicsCommandAllocator->Reset());
    COM_CALL(pRenderPass->pGraphicsCommandList->Reset(pRenderPass->pGraphicsCommandAllocator, nullptr));
    pRenderPass->isOpen = true;
    pRenderPass->pName = pRenderPassName;

    setD3D12ObjectDebugName(pRenderPass->pGraphicsCommandList, pRenderPassName);    
    addBeginMarker(pRenderPass->pGraphicsCommandList, pRenderPassName);
    return pRenderPass;
}

void endRenderPass(graphics_frame_t* pGraphicsFrame, render_pass_t* pRenderPass)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pGraphicsFrame != nullptr);
    ASSERT_DEBUG(pGraphicsFrame->openRenderPassCount > 0);
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

void clearColorRenderTarget(render_pass_t* pRenderPass, render_target_t* pRenderTarget, const float r, const float g, const float b, const float a)
{
    ASSERT_DEBUG(pRenderPass != nullptr);
    ASSERT_DEBUG(pRenderTarget != nullptr);
    ASSERT_DEBUG(isColorRenderTarget(pRenderTarget));
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

struct upload_buffer_t
{
    const void* pData;
    uint32_t sizeInBytes;

    d3d12_resource_t uploadHeapResource;
};

void createVertexBuffer(graphics_frame_t* pGraphicsFrame, upload_buffer_t* pUploadBuffer)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension  = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment  = 0u;
    desc.Width      = pUploadBuffer->sizeInBytes;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    ID3D12Resource* pResource = nullptr;
    pGraphicsFrame->pRenderContext->pDevice->CreateCommittedResource1(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, nullptr, IID_PPV_ARGS(&pResource));
}

upload_buffer_t* getNextFreeUploadBuffer(graphics_frame_t* pGraphicsFrame)
{
    return nullptr;
}

upload_buffer_t* createUploadBuffer(graphics_frame_t* pGraphicsFrame, const void* pData, const size_t dataSizeInBytes)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension  = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment  = 0u;
    desc.Width      = dataSizeInBytes;
    
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    ID3D12Resource* pResource = nullptr;
    pGraphicsFrame->pRenderContext->pDevice->CreateCommittedResource1(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, nullptr, IID_PPV_ARGS(&pResource));

    D3D12_RANGE readWriteRange = {};
    readWriteRange.Begin = 0u;
    readWriteRange.End = dataSizeInBytes;

    void* pMappedData = nullptr;
    COM_CALL(pResource->Map(0u, &readWriteRange, &pMappedData));
    memcpy(pMappedData, pData, dataSizeInBytes);
    pResource->Unmap(0u, &readWriteRange);

    upload_buffer_t* pUploadBuffer = getNextFreeUploadBuffer(pGraphicsFrame);
    pUploadBuffer->pData = pData;
    pUploadBuffer->sizeInBytes = (uint32_t)dataSizeInBytes;
    pUploadBuffer->uploadHeapResource.currentState = D3D12_RESOURCE_STATE_GENERIC_READ;
    pUploadBuffer->uploadHeapResource.pResource = pResource;

    return pUploadBuffer;
}

void renderFrame(HWND hwnd, graphics_frame_t* pGraphicsFrame)
{
    POINT cursorPos;
    RECT clientRect;
    GetCursorPos(&cursorPos);
    GetClientRect(hwnd, &clientRect);

    const float r = (float)cursorPos.x / (float)(clientRect.right - clientRect.left);
    const float g = (float)cursorPos.y / (float)(clientRect.bottom - clientRect.top);

    const FLOAT backBufferRGBA[4] = {r, g, 0.2f, 1.0f};

    render_pass_t* pRenderPass = startRenderPass(pGraphicsFrame, "Clear Background");
    clearColorRenderTarget(pRenderPass, pGraphicsFrame->pBackBuffer, r, g, 0.2f, 1.0f);
    endRenderPass(pGraphicsFrame, pRenderPass);

    const float triangleVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f
    };

    //render_pass_t* pDrawRenderPass = startRenderPass(pGraphicsFrame, "Draw Triangle");
    
    #if 0
    upload_buffer_t* pVertexBufferData = createUploadBuffer(pGraphicsFrame, triangleVertices, sizeof(triangleVertices));
    createVertexBuffer(pGraphicsFrame, pVertexBufferData)
    #endif

    executeRenderPass(pGraphicsFrame, pRenderPass);
}

void doFrame(HWND hwnd, render_context_t* pRenderContext)
{
    graphics_frame_t* pFrame = beginNextFrame(pRenderContext);
    renderFrame(hwnd, pFrame);
    finishFrame(pRenderContext, pFrame);
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
    COM_RELEASE(pRenderContext->pDefaultCommandQueue);
    COM_RELEASE(pRenderContext->pFactory);
    COM_RELEASE(pRenderContext->pDebugLayer);
    COM_RELEASE(pRenderContext->pDevice);

    clearMemoryWithZeroes(pRenderContext);
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

    uint32_t windowWidth = 1024;
    uint32_t windowHeight = 768;
    HWND hwnd = setupWindow(hInstance, windowWidth, windowHeight);

	if (hwnd == INVALID_HANDLE_VALUE)
		return -1;

    render_context_t renderContext = {};
    const bool useDebugLayer = true;

    RECT windowRect = {};
    GetClientRect(hwnd, &windowRect);
    windowWidth = windowRect.right - windowRect.left;
    windowHeight = windowRect.bottom - windowRect.top;
	if(!setup(&renderContext, hwnd, windowWidth, windowHeight, useDebugLayer))
    {
        shutdownRenderContext(&renderContext);
        return -1;
    }

	bool loopRunning = true;
	MSG msg = {0};

	while (loopRunning)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				loopRunning = false;
		}

		doFrame(hwnd, &renderContext);
	}

    shutdownRenderContext(&renderContext);
	return 0;
}