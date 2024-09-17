
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <stdio.h>
#include <stdint.h>

struct material_t
{
	graphics_pipeline_state_t* pGraphicsPipelineState;
};

struct mesh_t
{
	vertex_buffer_t* pVertexBuffer;
	vertex_format_t* pVertexFormat;

	uint32_t vertexOffset;
	uint32_t vertexCount;
};

void bindGraphicsPipelineState(render_pass_t* pRenderPass, graphics_pipeline_state_t* pGraphicsPipelineState)
{
    pRenderPass->pGraphicsCommandList->SetPipelineState(pGraphicsPipelineState->pPipelineState);
}

void draw(render_pass_t* pRenderPass, const uint32_t vertexOffset, const uint32_t vertexCount)
{
	pRenderPass->pGraphicsCommandList->DrawInstanced(vertexCount, 1u, vertexOffset, 0u);
}

void drawMesh(mesh_t* pMesh, material_t* pMaterial, render_pass_t* pRenderPass)
{
	bindGraphicsPipelineState(pRenderPass, pMaterial->pGraphicsPipelineState);
	bindVertexBuffer(pRenderPass, pMesh->pVertexBuffer, pMesh->pVertexFormat, 0u);
	draw(pRenderPass, pMesh->vertexOffset, pMesh->vertexCount);
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

uint32_t getTimeInMilliseconds(LARGE_INTEGER p_PerformanceFrequency)
{
	LARGE_INTEGER appTime = {0};
	QueryPerformanceFrequency(&appTime);

	appTime.QuadPart *= 1000; //to milliseconds

	return (uint32_t)(appTime.QuadPart / p_PerformanceFrequency.QuadPart);
}

void windowResized(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    render_context_t* pRenderContext = (render_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pRenderContext == nullptr)
        return;

    const uint32_t newWidth = LOWORD(lparam);
    const uint32_t newHeight = HIWORD(lparam);

    resizeBackBuffer(pRenderContext, newWidth, newHeight);
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

HWND setupWindow(HINSTANCE p_Instance, int p_Width, int p_Height, const char* pWindowTitle)
{
	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.hInstance = p_Instance;
	wndClass.lpszClassName = "D3D12RenderWindow";
	wndClass.lpfnWndProc = D3D12TestAppWindowProc;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("D3D12RenderWindow", pWindowTitle,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		p_Width, p_Height, 0, 0, p_Instance, 0);

	if (hwnd == INVALID_HANDLE_VALUE)
		MessageBox(0, "Error creating Window.\n", "Error!", 0);
	else
		ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}

bool setup(render_context_t* pRenderContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, bool useDebugLayer)
{
    const uint32_t frameBufferCount = 3u;
    const render_context_parameters_t parameters = createDefaultRenderContextParameters(pWindowHandle, frameBufferCount, windowWidth, windowHeight, useDebugLayer);
    if(!createRenderContext(pRenderContext, &parameters))
    {
        return false;
    }

    SetWindowLongPtrA(pWindowHandle, GWLP_USERDATA, (LONG_PTR)pRenderContext);
    return true;
}

struct test_context_frame_parameter_t;
typedef void(*testContextFrameCallback)(const test_context_frame_parameter_t*);

struct test_context_t
{
    HWND                        pWindowHandle;
    render_context_t*           pRenderContext;
    const char*                 pWindowTitle;
    testContextFrameCallback    pFrameCallback;
};

struct test_context_frame_parameter_t
{
    HWND                pWindowHandle;
    render_context_t*   pRenderContext;
};

struct test_context_parameters_t
{
    bool                        useDebugLayer;
    testContextFrameCallback    pFrameCallback;
};

result_t<test_context_t> initTestEnvironmentAndWindow(HINSTANCE hInstance, uint32_t width, uint32_t height, const char* pWindowTitle, const test_context_parameters_t* pParameters)
{
    HWND hwnd = setupWindow(hInstance, width, height, pWindowTitle);
    
    if (hwnd == INVALID_HANDLE_VALUE)
    {
		return result_status_t::internal_error;
    }

    render_context_t* pRenderContext = (render_context_t*)calloc(1u, sizeof(render_context_t));
    
    RECT windowRect = {};
    GetClientRect(hwnd, &windowRect);
    const uint32_t actualWindowWidth = windowRect.right - windowRect.left;
    const uint32_t actualWindowHeight = windowRect.bottom - windowRect.top;

    if(!setup(pRenderContext, hwnd, actualWindowWidth, actualWindowHeight, pParameters->useDebugLayer))
    {
        shutdownRenderContext(pRenderContext);
        return result_status_t::internal_error;
    }

    test_context_t testContext = {};
    testContext.pFrameCallback = pParameters->pFrameCallback;
    testContext.pRenderContext = pRenderContext;
    testContext.pWindowHandle  = hwnd;
    testContext.pWindowTitle   = pWindowTitle;

    return testContext;
}

int startTest(test_context_t* pTestContext)
{
    LARGE_INTEGER performanceFrequency, startTime, endTime;
	QueryPerformanceFrequency(&performanceFrequency);

    bool loopRunning = true;
	MSG msg = {0};

    test_context_frame_parameter_t frameParameters = {};
    frameParameters.pRenderContext = pTestContext->pRenderContext;
    frameParameters.pWindowHandle = pTestContext->pWindowHandle;

    char windowTitleBuffer[512] = {};

	while (loopRunning)
	{
        QueryPerformanceCounter(&startTime);
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (msg.message == WM_QUIT)
                    loopRunning = false;
            }
            pTestContext->pFrameCallback(&frameParameters);
        }
        QueryPerformanceCounter(&endTime);

        const LONGLONG frameTimeDelta = endTime.QuadPart - startTime.QuadPart;
        const float frameTimeDeltaInMs = ((float)frameTimeDelta / (float)performanceFrequency.QuadPart) * 1000.f;

        snprintf(windowTitleBuffer, sizeof(windowTitleBuffer), "%s - %.3f ms", pTestContext->pWindowTitle, frameTimeDeltaInMs);
        SetWindowTextA(pTestContext->pWindowHandle, windowTitleBuffer);
	}

    shutdownRenderContext(pTestContext->pRenderContext);
    return 0;
}