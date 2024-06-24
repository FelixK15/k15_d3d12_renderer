#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#define D3DCOMPILE_DEBUG 1
#define K15_USE_D3D12_DEBUG 1
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>

#include <stdio.h>
#include <stdint.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "DXGI.lib")

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#if K15_USE_D3D12_DEBUG
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

struct memory_allocator_t;
typedef void*(*allocate_from_memory_allocator_fnc)(memory_allocator_t*, uint64_t, uint64_t);
typedef void(*free_from_memory_allocator_fnc)(memory_allocator_t*, void*);

struct memory_allocator_t
{
    allocate_from_memory_allocator_fnc  allocateFnc;
    free_from_memory_allocator_fnc      freeFnc;
};

constexpr uint64_t defaultAllocationAlignment = 16u;

struct d3d12_resource_t
{
    ID3D12Resource* pResource;
    D3D12_RESOURCE_STATES currentState;
};

struct d3d12_render_target_t
{
    d3d12_resource_t            resource;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
};

struct d3d12_descriptor_heap_t
{
    uint8_t* pCPUBaseAddress;
    uint8_t* pGPUBaseAddress;
    uint8_t* pCPUCurrent;
    uint8_t* pGPUCurrent;
    ID3D12DescriptorHeap* pDescriptorHeap;
    uint64_t incrementSizeInBytes;
};

struct d3d12_swap_chain_t
{
    d3d12_render_target_t*          pBackBuffers;
    HANDLE*                         ppFrameEvents;
    IDXGISwapChain4*                pSwapChain;
    ID3D12Fence**                   ppFrameFence;
    ID3D12CommandAllocator**        ppFrameCommandAllocator;
    D3D12_CPU_DESCRIPTOR_HANDLE*    pBackBufferRenderTargetHandles;
    uint8_t                         backBufferCount;
};

struct d3d12_context_t
{
    ID3D12Device10*             pDevice;
    ID3D12Debug6*               pDebugLayer;
    IDXGIFactory6*              pFactory;
    
    memory_allocator_t          defaultAllocator;
    d3d12_swap_chain_t          swapChain;
    d3d12_descriptor_heap_t     renderTargetViewDescriptorHeap;
    ID3D12CommandQueue*         pDefaultCommandQueue;
    ID3D12GraphicsCommandList*  pDefaultGraphicsCommandList;
};

enum assert_result_t
{
    assert_result_debug,
    assert_result_continue,
    assert_result_exit
};

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

HRESULT logOnHResultError(const HRESULT originalResult, const char* pFunctionCall, const char* pFile, const uint32_t lineNumber)
{
    if(originalResult != S_OK)
    {
        printf("Error during call '%s' in %s:%u\nError:%s\n", pFunctionCall, pFile, lineNumber, getHResultString(originalResult));
    }

    return originalResult;
}

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

void* allocateFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes)
{
    return pAllocator->allocateFnc(pAllocator, sizeInBytes, defaultAllocationAlignment);
}

void* allocateAlignedFromAllocator(memory_allocator_t* pAllocator, uint64_t sizeInBytes, uint64_t alignmentInBytes)
{
    return pAllocator->allocateFnc(pAllocator, sizeInBytes, alignmentInBytes);
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

void initializeD3D12RenderTarget(d3d12_render_target_t* pRenderTarget, ID3D12Resource* pRenderTargetResource, D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorHandle, D3D12_RESOURCE_STATES state)
{
    pRenderTarget->resource.pResource       = pRenderTargetResource;
    pRenderTarget->resource.currentState    = state;
    pRenderTarget->cpuDescriptorHandle      = *pDescriptorHandle;
}

void destroyD3D12RenderTarget(d3d12_render_target_t* pRenderTarget)
{
    COM_RELEASE(pRenderTarget->resource.pResource);
}

D3D12_CPU_DESCRIPTOR_HANDLE getNextCPUDescriptorHandle(d3d12_descriptor_heap_t* pDescriptorHeap)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    cpuHandle.ptr = (size_t)pDescriptorHeap->pCPUCurrent;
    pDescriptorHeap->pCPUCurrent += pDescriptorHeap->incrementSizeInBytes;

    return cpuHandle;
}

void K15_WindowCreated(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{

}

void K15_WindowClosed(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{

}

void K15_KeyInput(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{

}

void K15_MouseButtonInput(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{

}

void K15_MouseMove(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{

}

void K15_MouseWheel(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{

}

void K15_WindowResized(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    d3d12_context_t* pDxContext = (d3d12_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pDxContext == nullptr)
        return;

    const uint32_t newWidth = LOWORD(lparam);
    const uint32_t newHeight = HIWORD(lparam);

    for(uint32_t bufferIndex = 0u; bufferIndex < pDxContext->swapChain.backBufferCount; ++bufferIndex)
    {
        WaitForSingleObject(pDxContext->swapChain.ppFrameEvents[bufferIndex], INFINITE);
        SetEvent(pDxContext->swapChain.ppFrameEvents[bufferIndex]);

        destroyD3D12RenderTarget(&pDxContext->swapChain.pBackBuffers[bufferIndex]);
    }

    pDxContext->swapChain.pSwapChain->ResizeBuffers(0u, newWidth, newHeight, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    pDxContext->renderTargetViewDescriptorHeap.pCPUCurrent = pDxContext->renderTargetViewDescriptorHeap.pCPUBaseAddress;

     for(uint32_t bufferIndex = 0u; bufferIndex < pDxContext->swapChain.backBufferCount; ++bufferIndex)
    {
        ID3D12Resource* pFrameBuffer = nullptr;
        COM_CALL(pDxContext->swapChain.pSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pFrameBuffer)));

        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = getNextCPUDescriptorHandle(&pDxContext->renderTargetViewDescriptorHeap);
        pDxContext->pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, descriptorHandle);

        initializeD3D12RenderTarget(&pDxContext->swapChain.pBackBuffers[bufferIndex], pFrameBuffer, &descriptorHandle, D3D12_RESOURCE_STATE_PRESENT);
    }
}

LRESULT CALLBACK K15_WNDPROC(HWND p_HWND, UINT p_Message, WPARAM p_wParam, LPARAM p_lParam)
{
	bool messageHandled = false;

	switch (p_Message)
	{
	case WM_CREATE:
		K15_WindowCreated(p_HWND, p_Message, p_wParam, p_lParam);
		break;

	case WM_CLOSE:
		K15_WindowClosed(p_HWND, p_Message, p_wParam, p_lParam);
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
		K15_KeyInput(p_HWND, p_Message, p_wParam, p_lParam);
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		K15_MouseButtonInput(p_HWND, p_Message, p_wParam, p_lParam);
		break;

	case WM_MOUSEMOVE:
		K15_MouseMove(p_HWND, p_Message, p_wParam, p_lParam);
		break;

	case WM_MOUSEWHEEL:
		K15_MouseWheel(p_HWND, p_Message, p_wParam, p_lParam);
		break;
    
    case WM_SIZE:
        K15_WindowResized(p_HWND, p_Message, p_wParam, p_lParam);
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
	wndClass.lpszClassName = "K15_D3D12RenderWindow";
	wndClass.lpfnWndProc = K15_WNDPROC;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("K15_D3D12RenderWindow", "D3D12 Renderer",
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

bool setupD3D12Device(ID3D12Device10** pOutDevice)
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

bool setupD3D12DebugLayer(ID3D12Device* pDevice)
{
    ID3D12InfoQueue* pInfoQueue = nullptr;
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

bool setupD3D12Factory(IDXGIFactory6** pOutFactory)
{
    uint32_t factoryFlags = 0u;

#if K15_USE_D3D12_DEBUG
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    if(COM_CALL(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(pOutFactory))) != S_OK)
    {
        return false;
    }

    return true;
}

bool setupD3D12DirectCommandQueue(ID3D12Device* pDevice, ID3D12CommandQueue** pOutCommandQueue)
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

bool setupD3D12SwapChain(d3d12_swap_chain_t* pOutSwapChain, memory_allocator_t* pAllocator, d3d12_descriptor_heap_t* pRenderTargetDescriptorHeap, IDXGIFactory6* pFactory, ID3D12Device10* pDevice, ID3D12CommandQueue* pCommandQueue, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, const uint32_t frameBufferCount)
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
    
    IDXGISwapChain1* pTempSwapChain = nullptr;
    IDXGISwapChain4* pSwapChain = nullptr;
    d3d12_render_target_t* pFrameBuffers = nullptr;
    HANDLE* ppFrameEvents = nullptr;
    ID3D12Fence** ppFrameFences = nullptr;
    ID3D12CommandAllocator** ppFrameCommandAllocators = nullptr;
    if(COM_CALL(pFactory->CreateSwapChainForHwnd(pCommandQueue, pWindowHandle, &swapChainDesc, nullptr, nullptr, &pTempSwapChain)) != S_OK)
    {
        goto cleanup_and_exit_failure;
    }

    if(COM_CALL(pTempSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain))) != S_OK)
    {
        goto cleanup_and_exit_failure;
    }

    pFrameBuffers = (d3d12_render_target_t*)allocateFromAllocator(pAllocator, sizeof(d3d12_render_target_t) * frameBufferCount);
    ppFrameFences = (ID3D12Fence**)allocateFromAllocator(pAllocator, sizeof(void*) * frameBufferCount);
    ppFrameEvents = (HANDLE*)allocateFromAllocator(pAllocator, sizeof(HANDLE) * frameBufferCount);
    ppFrameCommandAllocators = (ID3D12CommandAllocator**)allocateFromAllocator(pAllocator, sizeof(void*) * frameBufferCount);
    if(pFrameBuffers == nullptr || ppFrameFences == nullptr || ppFrameEvents == nullptr || ppFrameCommandAllocators == nullptr)
    {
        goto cleanup_and_exit_failure;
    }

    for(uint32_t bufferIndex = 0u; bufferIndex < frameBufferCount; ++bufferIndex)
    {
        if(COM_CALL(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ppFrameFences[bufferIndex]))) != S_OK)
        {
            goto cleanup_and_exit_failure;
        }

        ID3D12Resource* pFrameBuffer = nullptr;
        if(COM_CALL(pSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&pFrameBuffer))) != S_OK)
        {
            goto cleanup_and_exit_failure;
        }

        ID3D12CommandAllocator* pCommandAllocator = nullptr;
        if(COM_CALL(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&ppFrameCommandAllocators[bufferIndex]))) != S_OK)
        {
            goto cleanup_and_exit_failure;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = getNextCPUDescriptorHandle(pRenderTargetDescriptorHeap);
        pDevice->CreateRenderTargetView(pFrameBuffer, nullptr, descriptorHandle);

        initializeD3D12RenderTarget(&pFrameBuffers[bufferIndex], pFrameBuffer, &descriptorHandle, D3D12_RESOURCE_STATE_PRESENT);

        ppFrameEvents[bufferIndex] = CreateEvent(nullptr, FALSE, TRUE, nullptr);
    }
    
    pOutSwapChain->backBufferCount = frameBufferCount;
    pOutSwapChain->ppFrameEvents = ppFrameEvents;
    pOutSwapChain->pBackBuffers = pFrameBuffers;
    pOutSwapChain->ppFrameFence = ppFrameFences;
    pOutSwapChain->ppFrameCommandAllocator = ppFrameCommandAllocators;
    pOutSwapChain->pSwapChain = pSwapChain;
    COM_RELEASE(pTempSwapChain);
    return true;

    cleanup_and_exit_failure:
        COM_RELEASE(pSwapChain);
        for(uint32_t bufferIndex = 0; bufferIndex < frameBufferCount; ++bufferIndex)
        {
            destroyD3D12RenderTarget(&pFrameBuffers[bufferIndex]);
            ppFrameFences[bufferIndex]->Release();
        }

        freeFromAllocator(pAllocator, pFrameBuffers);
        freeFromAllocator(pAllocator, ppFrameFences);
        return false;
}

bool setupD3D12DescriptorHeap(d3d12_descriptor_heap_t* pOutDescriptorHeap, ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t descriptorCount )
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
    pOutDescriptorHeap->pCPUBaseAddress = (uint8_t*)cpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pGPUBaseAddress = (uint8_t*)gpuDescriptorHeapStartAddress;
    pOutDescriptorHeap->pCPUCurrent = pOutDescriptorHeap->pCPUBaseAddress;
    pOutDescriptorHeap->pGPUCurrent = pOutDescriptorHeap->pGPUBaseAddress;
    pOutDescriptorHeap->pDescriptorHeap = pDescriptorHeap;

    return true;
}

bool setupD3D12DirectCommandAllocator(ID3D12Device* pDevice, ID3D12CommandAllocator** pOutCommandAllocator)
{
    if(COM_CALL(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pOutCommandAllocator))) != S_OK)
    {
        return false;
    }

    return true;
}

bool setupD3D12DirectGraphicsCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator, ID3D12GraphicsCommandList** pOutCommandList)
{
    if(COM_CALL(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(pOutCommandList))) != S_OK)
    {
        return false;
    }

    (*pOutCommandList)->Close();

    return true;
}

bool createD3D12Fence(ID3D12Device* pDevice, ID3D12Fence** pOutFence, const uint32_t initialValue)
{
    if(COM_CALL(pDevice->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(pOutFence))) != S_OK)
    {
        return false;
    }

    return true;
}

bool setupD3D12(d3d12_context_t* pDxContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, const uint32_t frameBufferCount)
{
    createDefaultMemoryAllocator(&pDxContext->defaultAllocator);

    if(!setupD3D12Factory(&pDxContext->pFactory))
    {
        return false;
    }

    if(!setupD3D12Device(&pDxContext->pDevice))
    {
        return false;
    }

    if(!setupD3D12DirectCommandQueue(pDxContext->pDevice, &pDxContext->pDefaultCommandQueue))
    {
        return false;
    }

    if(!setupD3D12DescriptorHeap(&pDxContext->renderTargetViewDescriptorHeap, pDxContext->pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16u))
    {
        return false;
    }

    if(!setupD3D12SwapChain(&pDxContext->swapChain, &pDxContext->defaultAllocator, &pDxContext->renderTargetViewDescriptorHeap, pDxContext->pFactory, pDxContext->pDevice, pDxContext->pDefaultCommandQueue, pWindowHandle, windowWidth, windowHeight, frameBufferCount))
    {
        return false;
    }

    if(!setupD3D12DirectGraphicsCommandList(pDxContext->pDevice, pDxContext->swapChain.ppFrameCommandAllocator[0], &pDxContext->pDefaultGraphicsCommandList))
    {
        return false;
    }

    return true;
}

bool setup(d3d12_context_t* pDxContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, bool useDebugLayer)
{
    static constexpr uint32_t frameBufferCount = 2u;
    if(useDebugLayer)
    {
        useDebugLayer = enableD3D12DebugLayer(&pDxContext->pDebugLayer);
    }

    if(!setupD3D12(pDxContext, pWindowHandle, windowWidth, windowHeight, frameBufferCount))
    {
        return false;
    }

    if(useDebugLayer)
    {
        if(!setupD3D12DebugLayer(pDxContext->pDevice))
        {

        }
    }

    SetWindowLongPtrA(pWindowHandle, GWLP_USERDATA, (LONG_PTR)pDxContext);
    return true;
}

void doFrame(HWND hwnd, d3d12_context_t* pD3D12Context, uint32_t p_DeltaTimeInMS, uint32_t frameIndex)
{
    POINT cursorPos;
    RECT clientRect;
    GetCursorPos(&cursorPos);
    GetClientRect(hwnd, &clientRect);

    const float r = (float)cursorPos.x / (float)(clientRect.right - clientRect.left);
    const float g = (float)cursorPos.y / (float)(clientRect.bottom - clientRect.top);

    const FLOAT backBufferRGBA[4] = {r, g, 0.2f, 1.0f};
    const uint32_t currentBufferIndex = pD3D12Context->swapChain.pSwapChain->GetCurrentBackBufferIndex();

    WaitForSingleObject(pD3D12Context->swapChain.ppFrameEvents[currentBufferIndex], INFINITE);

    ID3D12CommandAllocator* pFrameCommandAllocator = pD3D12Context->swapChain.ppFrameCommandAllocator[currentBufferIndex];
    pFrameCommandAllocator->Reset();
    pD3D12Context->pDefaultGraphicsCommandList->Reset(pFrameCommandAllocator, nullptr);
    
    d3d12_render_target_t* pCurrentBackBuffer = &pD3D12Context->swapChain.pBackBuffers[currentBufferIndex];
    transitionResource(pD3D12Context->pDefaultGraphicsCommandList, &pCurrentBackBuffer->resource, D3D12_RESOURCE_STATE_RENDER_TARGET);

    pD3D12Context->pDefaultGraphicsCommandList->ClearRenderTargetView(pCurrentBackBuffer->cpuDescriptorHandle, backBufferRGBA, 0, nullptr);

    transitionResource(pD3D12Context->pDefaultGraphicsCommandList, &pCurrentBackBuffer->resource, D3D12_RESOURCE_STATE_PRESENT);

    pD3D12Context->pDefaultGraphicsCommandList->Close();
    pD3D12Context->pDefaultCommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList* const*)&pD3D12Context->pDefaultGraphicsCommandList);

    pD3D12Context->pDefaultCommandQueue->Signal(pD3D12Context->swapChain.ppFrameFence[currentBufferIndex], frameIndex);
    pD3D12Context->swapChain.pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

    pD3D12Context->swapChain.ppFrameFence[currentBufferIndex]->SetEventOnCompletion(frameIndex, pD3D12Context->swapChain.ppFrameEvents[currentBufferIndex]);
}

void clearD3D12Context(d3d12_context_t* pD3D12Context)
{
    COM_RELEASE(pD3D12Context->pDefaultCommandQueue);
    COM_RELEASE(pD3D12Context->pFactory);
    COM_RELEASE(pD3D12Context->pDebugLayer);
    COM_RELEASE(pD3D12Context->pDevice);
    COM_RELEASE(pD3D12Context->pDebugLayer);
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

    RECT bla = {};
    bla.right = 1024;
    bla.bottom = 768;
    AdjustWindowRect(&bla, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = setupWindow(hInstance, bla.right - bla.left, bla.bottom - bla.top);

	if (hwnd == INVALID_HANDLE_VALUE)
		return -1;

    d3d12_context_t d3d12Context = {};
    const bool useDebugLayer = true;

    RECT windowRect = {};
    GetClientRect(hwnd, &windowRect);

    const uint32_t windowWidth = windowRect.right - windowRect.left;
    const uint32_t windowHeight = windowRect.bottom - windowRect.top;
	if(!setup(&d3d12Context, hwnd, windowWidth, windowHeight, useDebugLayer))
    {
        clearD3D12Context(&d3d12Context);
        return -1;
    }

	uint32_t timeFrameStarted = 0;
	uint32_t timeFrameEnded = 0;
	uint32_t deltaMs = 0;

	bool loopRunning = true;
	MSG msg = {0};

    uint32_t frameIndex = 1u;
	while (loopRunning)
	{
		timeFrameStarted = getTimeInMilliseconds(performanceFrequency);

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				loopRunning = false;
		}

		doFrame(hwnd, &d3d12Context, deltaMs, frameIndex);

		timeFrameEnded = getTimeInMilliseconds(performanceFrequency);
		deltaMs = timeFrameEnded - timeFrameStarted;

        ++frameIndex;
	}

    clearD3D12Context(&d3d12Context);
	return 0;
}