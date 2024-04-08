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
#define COM_LOG_ON_ERROR_HRESULT(func) logOnHResultError(func, #func, __FILE__, __LINE__)
#else
#define COM_LOG_ON_ERROR_HRESULT(func) func
#endif

#define COM_RELEASE(ptr)if(ptr != nullptr) { (ptr)->Release(); ptr = nullptr; }

struct d3d12_context_t
{
    ID3D12Device10*         pDevice;
    ID3D12Debug6*           pDebugLayer;
    ID3D12CommandQueue*     pDefaultCommandQueue;
    ID3D12DescriptorHeap*   pRenderTargetViewDescriptorHeap;
    IDXGISwapChain1*        pSwapChain;
    IDXGIFactory6*          pFactory;
};

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

bool setupD3D12Device(ID3D12Device10** pOutDevice)
{
    if(COM_LOG_ON_ERROR_HRESULT(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(pOutDevice))) != S_OK)
    {
        return false;
    }

    (*pOutDevice)->SetName(L"D3D12Device");
    return true;
}

bool setupD3D12DebugLayer(ID3D12Device* pDevice)
{
    ID3D12InfoQueue* pInfoQueue = nullptr;
    if(COM_LOG_ON_ERROR_HRESULT(pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue))) != S_OK)
    {
        return false;
    }

    if(COM_LOG_ON_ERROR_HRESULT(pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE)) != S_OK)
    {
        return false;
    }
    
    if(COM_LOG_ON_ERROR_HRESULT(pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE)) != S_OK)
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

    if(COM_LOG_ON_ERROR_HRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(pOutFactory))) != S_OK)
    {
        return false;
    }

    return true;
}

bool enableD3D12DebugLayer(ID3D12Debug6** pOutDebugLayer)
{
    if(COM_LOG_ON_ERROR_HRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(pOutDebugLayer))) != S_OK)
    {
        return false;
    }

    (*pOutDebugLayer)->EnableDebugLayer();
    (*pOutDebugLayer)->SetEnableAutoName(TRUE);
    return true;
}

bool setupD3D12DefaultCommandQueue(ID3D12Device* pDevice, ID3D12CommandQueue** pOutCommandQueue)
{
    D3D12_COMMAND_QUEUE_DESC defaultCommandQueueDesc = {};
    defaultCommandQueueDesc.Type        = D3D12_COMMAND_LIST_TYPE_DIRECT;
    defaultCommandQueueDesc.Priority    = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    defaultCommandQueueDesc.Flags       = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if(COM_LOG_ON_ERROR_HRESULT(pDevice->CreateCommandQueue(&defaultCommandQueueDesc, IID_PPV_ARGS(pOutCommandQueue))) != S_OK)
    {
        return false;
    }

    (*pOutCommandQueue)->SetName(L"D3D12 Default Command Queue");

    return true;
}

bool setupD3D12SwapChain(IDXGIFactory6* pFactory, ID3D12CommandQueue* pCommandQueue, IDXGISwapChain1** pOutSwapChain, HWND pWindowHandle, const uint32_t windowHeight, const uint32_t windowWidth, const uint32_t frameBufferCount)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount   = 2;
    swapChainDesc.SampleDesc    = {1, 0};
    swapChainDesc.Stereo        = FALSE;
    swapChainDesc.BufferUsage   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect    = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Height        = windowHeight;
    swapChainDesc.Width         = windowWidth;
    swapChainDesc.Scaling       = DXGI_SCALING_NONE;
    swapChainDesc.AlphaMode     = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags         = 0;
    
    if(COM_LOG_ON_ERROR_HRESULT(pFactory->CreateSwapChainForHwnd(pCommandQueue, pWindowHandle, &swapChainDesc, nullptr, nullptr, pOutSwapChain)) != S_OK)
    {
        return false;
    }

    return true;
}

bool setupD3D12DescriptorHeap(ID3D12Device* pDevice, ID3D12DescriptorHeap** pOutDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t descriptorCount )
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.Type             = type;
    descriptorHeapDesc.NumDescriptors   = descriptorCount;
    descriptorHeapDesc.Flags            = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if(COM_LOG_ON_ERROR_HRESULT(pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(pOutDescriptorHeap))) != S_OK)
    {
        return false;
    }

    return true;
}

bool setupD3D12(d3d12_context_t* pDxContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, const uint32_t frameBufferCount)
{
    if(!setupD3D12Factory(&pDxContext->pFactory))
    {
        return false;
    }

#if K15_USE_D3D12_DEBUG
    if(!enableD3D12DebugLayer(&pDxContext->pDebugLayer))
    {
        return false;
    }
#endif

    if(!setupD3D12Device(&pDxContext->pDevice))
    {
        return false;
    }

#if K15_USE_D3D12_DEBUG
    if(!setupD3D12DebugLayer(pDxContext->pDevice))
    {
        return false;
    }
#endif

    if(!setupD3D12DefaultCommandQueue(pDxContext->pDevice, &pDxContext->pDefaultCommandQueue))
    {
        return false;
    }

    if(!setupD3D12SwapChain(pDxContext->pFactory, pDxContext->pDefaultCommandQueue, &pDxContext->pSwapChain, pWindowHandle, windowWidth, windowHeight, frameBufferCount))
    {
        return false;
    }

    if(!setupD3D12DescriptorHeap(pDxContext->pDevice, &pDxContext->pRenderTargetViewDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16u))
    {
        return false;
    }

    return true;
}

bool setup(d3d12_context_t* pDxContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight)
{
	//allocateDebugConsole();

    static constexpr uint32_t frameBufferCount = 3u;
    return setupD3D12(pDxContext, pWindowHandle, windowWidth, windowHeight, frameBufferCount);
}

void doFrame(uint32_t p_DeltaTimeInMS)
{

}

void clearD3D12Context(d3d12_context_t* pD3D12Context)
{
    COM_RELEASE(pD3D12Context->pDefaultCommandQueue);
    COM_RELEASE(pD3D12Context->pFactory);
    COM_RELEASE(pD3D12Context->pRenderTargetViewDescriptorHeap);
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

	HWND hwnd = setupWindow(hInstance, 1024, 768);

	if (hwnd == INVALID_HANDLE_VALUE)
		return -1;

    d3d12_context_t d3d12Context = {};
	if(!setup(&d3d12Context, hwnd, 1024, 768))
    {
        clearD3D12Context(&d3d12Context);
        return -1;
    }

	uint32_t timeFrameStarted = 0;
	uint32_t timeFrameEnded = 0;
	uint32_t deltaMs = 0;

	bool loopRunning = true;
	MSG msg = {0};

	while (loopRunning)
	{
		timeFrameStarted = getTimeInMilliseconds(performanceFrequency);

		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				loopRunning = false;
		}


		doFrame(deltaMs);

		timeFrameEnded = getTimeInMilliseconds(performanceFrequency);
		deltaMs = timeFrameEnded - timeFrameStarted;
	}

	return 0;
}