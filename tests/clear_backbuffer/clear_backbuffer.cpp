void doClearBackgroundSample(sample_frame_parameter_t* pFrameParameter, const int x, const int y, const int width, const int height)
{
    POINT cursorPos;
    RECT clientRect;
    GetCursorPos(&cursorPos);
    GetClientRect(pFrameParameter->pWindowHandle, &clientRect);

    const float r = (float)cursorPos.x / (float)(clientRect.right - clientRect.left);
    const float g = (float)cursorPos.y / (float)(clientRect.bottom - clientRect.top);

    const FLOAT backBufferRGBA[4] = {r, g, 0.2f, 1.0f};

    render_pass_t* pRenderPass = startRenderPass(pFrameParameter->pGraphicsFrame, "Clear Background", pFrameParameter->pGraphicsFrame->pBackBuffer);
    setViewport(pRenderPass, x, y, width, height, 0.0f, 100.f);
    setScissor(pRenderPass, x, y, width, height);
   //clearColorRenderTarget(pRenderPass, pFrameParameter->pGraphicsFrame->pBackBuffer, r, g, 0.2f, 1.0f);
    endRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass);
    executeRenderPass(pFrameParameter->pGraphicsFrame, pRenderPass, render_pass_execution_order_t::push_back);
}