#include "../../k15_d3d12_renderer.hpp"
#include "../test_base.hpp"

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

    executeRenderPass(pGraphicsFrame, pRenderPass);
}

void doFrame(const test_context_frame_parameter_t* pFrameParameter)
{
    graphics_frame_t* pFrame = beginNextFrame(pFrameParameter->pRenderContext);
    renderFrame(pFrameParameter->pWindowHandle, pFrame);
    finishFrame(pFrameParameter->pRenderContext, pFrame);
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
    test_context_parameters_t parameters = {};
    parameters.useDebugLayer = true;
    parameters.pFrameCallback = doFrame;

    result_t<test_context_t> testContextResult = initTestEnvironmentAndWindow(hInstance, 1024, 768, "[DX12] clear backbuffer", &parameters);
    if(!isResultSuccessful(testContextResult))
    {
        return -1;
    }

    return startTest(&testContextResult.value);
}