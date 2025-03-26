
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.hpp"

union matrix4x4f_t
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };

    float m[16];
};

struct vector4f_t
{
    float x, y, z, w;
};

struct vector3f_t
{
    float x, y, z;
};

struct vector2f_t
{
    float x, y;
};

struct material_t
{
	graphics_pipeline_t* pGraphicsPipeline;
};

struct mesh_t
{
	gpu_buffer_t* pVertexBuffer;
	vertex_format_t* pVertexFormat;

	uint32_t vertexOffset;
	uint32_t vertexCount;
};

struct indexed_mesh_t
{
	gpu_buffer_t* pVertexBuffer;
	gpu_buffer_t* pIndexBuffer;
	vertex_format_t* pVertexFormat;

	uint32_t indexOffset;
	uint32_t indexCount;
};

struct test_context_frame_parameter_t;
typedef void(*testContextFrameCallback)(test_context_frame_parameter_t*);
typedef bool(*testContextInitCallback)(test_context_frame_parameter_t*);

struct test_context_frame_parameter_t
{
    HWND                pWindowHandle;
	memory_allocator_t*	pAllocator;
    render_context_t*   pRenderContext;
	graphics_frame_t* 	pGraphicsFrame;
	render_target_t* 	pRenderTarget;

	void* 				pUserData;

	float 				deltaTimeInMs;
	float 				totalFrameTimeInMs;

	uint32_t 			windowWidth;
	uint32_t 			windowHeight;
};

struct test_context_t
{
	test_context_frame_parameter_t 	frameParameter;
    HWND                       		pWindowHandle;
    render_context_t*          		pRenderContext;
    const char*                		pWindowTitle;
    testContextFrameCallback    	pFrameCallback;
	testContextInitCallback 		pInitCallback;
	testContextFrameCallback 		pShutdownCallback;
};

struct test_context_parameters_t
{
    bool                        useDebugLayer;
    testContextFrameCallback    pFrameCallback;
	testContextInitCallback 	pInitCallback;
	testContextFrameCallback 	pShutdownCallback;
};

constexpr const matrix4x4f_t IdentityMatrix4x4[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

constexpr float pi = 3.141f;

void createProjectionMatrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float nearDistance, float farDistance, float fov)
{
    memset(pOutMatrix, 0, sizeof(matrix4x4f_t));

    const float widthF = (float)width;
    const float heightF = (float)height;
    const float fovRad = fov/180.f * pi;
    const float aspect = widthF / heightF;
    const float e = 1.0f/tanf(fovRad/2.0f);

    pOutMatrix->m00 = e/aspect;
    pOutMatrix->m11 = e;
    pOutMatrix->m22 = ((farDistance + nearDistance)/(farDistance - nearDistance));
    pOutMatrix->m23 = -((2.0f * farDistance * nearDistance)/(farDistance - nearDistance));
    pOutMatrix->m32 = 1.0f;
}

void createOrthographicMatrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float nearDistance, float farDistance)
{
    memset(pOutMatrix, 0, sizeof(matrix4x4f_t));

    pOutMatrix->m00 = 1.0f / (float)width;
    pOutMatrix->m11 = 1.0f / (float)height;
    pOutMatrix->m22 = -2.0f / (farDistance-nearDistance);
    pOutMatrix->m23 = -((farDistance+nearDistance)/(farDistance-nearDistance));
    pOutMatrix->m33 = 1.0f;
}

void setIdentityMatrix(matrix4x4f_t* pMatrix)
{
    memcpy(pMatrix, IdentityMatrix4x4, sizeof(IdentityMatrix4x4));
}

bool matrixIsEqual(const matrix4x4f_t* __restrict pA, const matrix4x4f_t* __restrict pB)
{
    return memcmp(pA, pB, sizeof(matrix4x4f_t)) == 0;
}

vector4f_t mulVectorMatrix(const vector4f_t* pVector, const matrix4x4f_t* pMatrix)
{
    vector4f_t multipliedVector = {};
    multipliedVector.x = pVector->x * pMatrix->m00 + pVector->y * pMatrix->m01 + pVector->z * pMatrix->m02 + pVector->w * pMatrix->m03;
    multipliedVector.y = pVector->x * pMatrix->m10 + pVector->y * pMatrix->m11 + pVector->z * pMatrix->m12 + pVector->w * pMatrix->m13;
    multipliedVector.z = pVector->x * pMatrix->m20 + pVector->y * pMatrix->m21 + pVector->z * pMatrix->m22 + pVector->w * pMatrix->m23;
    multipliedVector.w = pVector->x * pMatrix->m30 + pVector->y * pMatrix->m31 + pVector->z * pMatrix->m32 + pVector->w * pMatrix->m33;
    return multipliedVector;
}

matrix4x4f_t mulMatrices(const matrix4x4f_t* __restrict pMatA, const matrix4x4f_t* __restrict pMatB)
{
    matrix4x4f_t mat = {};
    mat.m00 = pMatA->m00 * pMatB->m00 + pMatA->m01 * pMatB->m10 + pMatA->m02 * pMatB->m20 + pMatA->m03 * pMatB->m30;
    mat.m01 = pMatA->m00 * pMatB->m01 + pMatA->m01 * pMatB->m11 + pMatA->m02 * pMatB->m21 + pMatA->m03 * pMatB->m31;
    mat.m02 = pMatA->m00 * pMatB->m02 + pMatA->m01 * pMatB->m12 + pMatA->m02 * pMatB->m22 + pMatA->m03 * pMatB->m32;
    mat.m03 = pMatA->m00 * pMatB->m03 + pMatA->m01 * pMatB->m13 + pMatA->m02 * pMatB->m23 + pMatA->m03 * pMatB->m33;

    mat.m10 = pMatA->m10 * pMatB->m00 + pMatA->m11 * pMatB->m10 + pMatA->m12 * pMatB->m20 + pMatA->m13 * pMatB->m30;
    mat.m11 = pMatA->m10 * pMatB->m01 + pMatA->m11 * pMatB->m11 + pMatA->m12 * pMatB->m21 + pMatA->m13 * pMatB->m31;
    mat.m12 = pMatA->m10 * pMatB->m02 + pMatA->m11 * pMatB->m12 + pMatA->m12 * pMatB->m22 + pMatA->m13 * pMatB->m32;
    mat.m13 = pMatA->m10 * pMatB->m03 + pMatA->m11 * pMatB->m13 + pMatA->m12 * pMatB->m23 + pMatA->m13 * pMatB->m33;

    mat.m20 = pMatA->m20 * pMatB->m00 + pMatA->m21 * pMatB->m10 + pMatA->m22 * pMatB->m20 + pMatA->m23 * pMatB->m30;
    mat.m21 = pMatA->m20 * pMatB->m01 + pMatA->m21 * pMatB->m11 + pMatA->m22 * pMatB->m21 + pMatA->m23 * pMatB->m31;
    mat.m22 = pMatA->m20 * pMatB->m02 + pMatA->m21 * pMatB->m12 + pMatA->m22 * pMatB->m22 + pMatA->m23 * pMatB->m32;
    mat.m23 = pMatA->m20 * pMatB->m03 + pMatA->m21 * pMatB->m13 + pMatA->m22 * pMatB->m23 + pMatA->m23 * pMatB->m33;

    mat.m30 = pMatA->m30 * pMatB->m00 + pMatA->m31 * pMatB->m10 + pMatA->m32 * pMatB->m20 + pMatA->m33 * pMatB->m30;
    mat.m31 = pMatA->m30 * pMatB->m01 + pMatA->m31 * pMatB->m11 + pMatA->m32 * pMatB->m21 + pMatA->m33 * pMatB->m31;
    mat.m32 = pMatA->m30 * pMatB->m02 + pMatA->m31 * pMatB->m12 + pMatA->m32 * pMatB->m22 + pMatA->m33 * pMatB->m32;
    mat.m33 = pMatA->m30 * pMatB->m03 + pMatA->m31 * pMatB->m13 + pMatA->m32 * pMatB->m23 + pMatA->m33 * pMatB->m33;

    return mat;
}

mesh_t* createMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, const float* pVertices, const uint32_t vertexCount, vertex_format_t* pVertexFormat)
{
    const uint32_t vertexBufferSizeInBytes = vertexCount * calculateVertexStrideSizeInBytes(pVertexFormat);

    mesh_t* pMesh = (mesh_t*)allocateFromAllocator(pMemoryAllocator, sizeof(mesh_t), alloc_flags_t::clear_memory);
    pMesh->vertexCount = vertexCount;
    pMesh->vertexOffset = 0u;
    pMesh->pVertexFormat = pVertexFormat;
    pMesh->pVertexBuffer = createGpuBuffer(pGraphicsFrame, vertexBufferSizeInBytes, pVertices, gpu_buffer_usage_flag_t::vertex_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess, "Vertex buffer");

    return pMesh;
}

indexed_mesh_t* createIndexedMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, const float* pVertices, const uint32_t* pIndices, const uint32_t vertexCount, const uint32_t indexCount, vertex_format_t* pVertexFormat)
{
	const uint32_t vertexBufferSizeInBytes = vertexCount * calculateVertexStrideSizeInBytes(pVertexFormat);
	const uint32_t indexBufferSizeInBytes = indexCount * sizeof(uint32_t);

    indexed_mesh_t* pMesh = (indexed_mesh_t*)allocateFromAllocator(pMemoryAllocator, sizeof(indexed_mesh_t), alloc_flags_t::clear_memory);
    pMesh->indexCount = indexCount;
    pMesh->indexOffset = 0u;
    pMesh->pVertexFormat = pVertexFormat;
	pMesh->pIndexBuffer = createGpuBuffer(pGraphicsFrame, indexBufferSizeInBytes, pIndices, gpu_buffer_usage_flag_t::index_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess, "Index Buffer");
    pMesh->pVertexBuffer = createGpuBuffer(pGraphicsFrame, vertexBufferSizeInBytes, pVertices, gpu_buffer_usage_flag_t::vertex_buffer, gpu_memory_usage_hint_t::gpuExclusiveAccess, "Vertex Buffer");

    return pMesh;
}

material_t* createMaterial(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, vertex_format_t* pVertexFormat, const shader_compilation_parameters_t* pVertexShaderParameters, const shader_compilation_parameters_t* pPixelShaderParameters)
{
    graphics_pipeline_parameters_t pipelineParameters = {};
    pipelineParameters.pVertexShader   = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pVertexShaderParameters, shader_type_flag_t::vertex_shader);
    pipelineParameters.pPixelShader    = loadAndCompileShaderCodeFromFile(pGraphicsFrame, pPixelShaderParameters, shader_type_flag_t::pixel_shader);
    pipelineParameters.pVertexFormat   = pVertexFormat;
    pipelineParameters.pName           = "Sample Material";
	pipelineParameters.topology 	   = topology_t::triangle_list;

    graphics_pipeline_t* pDefaultPipelineObject = createGraphicsPipeline(pGraphicsFrame, &pipelineParameters);
    material_t* pMaterial = (material_t*)allocateFromAllocator(pMemoryAllocator, sizeof(material_t), alloc_flags_t::clear_memory);
    pMaterial->pGraphicsPipeline = pDefaultPipelineObject;
    return pMaterial;
}

void destroyMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, mesh_t* pMesh)
{
	if(pMesh->pVertexBuffer != nullptr)
	{
	    releaseGpuBuffer(pGraphicsFrame, pMesh->pVertexBuffer);
		pMesh->pVertexBuffer = nullptr;
	}

	if(pMesh->pVertexFormat != nullptr)
	{
    	releaseVertexFormat(pGraphicsFrame, pMesh->pVertexFormat);
		pMesh->pVertexFormat = nullptr;
	}

    freeFromAllocator(pMemoryAllocator, pMesh);
}

void destroyIndexedMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, indexed_mesh_t* pMesh)
{
	if(pMesh->pVertexBuffer != nullptr)
	{
	    releaseGpuBuffer(pGraphicsFrame, pMesh->pVertexBuffer);
		pMesh->pVertexBuffer = nullptr;
	}

	if(pMesh->pIndexBuffer != nullptr)
	{
	    releaseGpuBuffer(pGraphicsFrame, pMesh->pIndexBuffer);
		pMesh->pIndexBuffer = nullptr;
	}

	if(pMesh->pVertexFormat != nullptr)
	{
    	releaseVertexFormat(pGraphicsFrame, pMesh->pVertexFormat);
		pMesh->pVertexFormat = nullptr;
	}

    freeFromAllocator(pMemoryAllocator, pMesh);
}

void destroyMaterial(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator, material_t* pMaterial)
{
	if(pMaterial->pGraphicsPipeline != nullptr)
	{
	    releaseGraphicsPipeline(pGraphicsFrame, pMaterial->pGraphicsPipeline);
		pMaterial->pGraphicsPipeline = nullptr;
	}
    freeFromAllocator(pMemoryAllocator, pMaterial);
}

mesh_t* createSingleTriangleMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator)
{
    const float triangleVertices[] = {
        // Color (R, G, B)   	Position (X, Y, Z, W)
        1.0f, 0.0f, 0.0f,  	   -0.5f, -0.5f, 0.5f, 1.0f,
        0.0f, 1.0f, 0.0f,  		0.0f,  0.5f, 0.5f, 1.0f,
        0.0f, 0.0f, 1.0f,  		0.5f, -0.5f, 0.5f, 1.0f
    };

    vertex_attribute_entry_t pVertexAttributes[] = {
        {vertex_attribute_t::color, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 3u},
        {vertex_attribute_t::position, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 12u, 4u}
    };

    vertex_format_t* pVertexFormat = createVertexFormat(pGraphicsFrame, pVertexAttributes, 2u);
	if(pVertexFormat == nullptr)
	{
		return nullptr;
	}

    return createMesh(pGraphicsFrame, pMemoryAllocator, triangleVertices, 3u, pVertexFormat);
}

indexed_mesh_t* createUnitCubeIndexedMesh(graphics_frame_t* pGraphicsFrame, memory_allocator_t* pMemoryAllocator)
{
    // Cube vertices with color (red) and position
	constexpr float cubeVertices[] = {
		// Front face
		-0.5f, -0.5f,  0.5f,   	0.0f, 1.0f,   0.0f,  0.0f,  1.0f, // 0
		 0.5f, -0.5f,  0.5f,   	1.0f, 1.0f,   0.0f,  0.0f,  1.0f, // 1
		 0.5f,  0.5f,  0.5f,   	1.0f, 0.0f,   0.0f,  0.0f,  1.0f, // 2
		-0.5f,  0.5f,  0.5f,   	0.0f, 0.0f,   0.0f,  0.0f,  1.0f, // 3
	
		// Back face
		 0.5f, -0.5f, -0.5f,   	0.0f, 1.0f,   0.0f,  0.0f, -1.0f, // 4
		-0.5f, -0.5f, -0.5f,   	1.0f, 1.0f,   0.0f,  0.0f, -1.0f, // 5
		-0.5f,  0.5f, -0.5f,   	1.0f, 0.0f,   0.0f,  0.0f, -1.0f, // 6
		 0.5f,  0.5f, -0.5f,   	0.0f, 0.0f,   0.0f,  0.0f, -1.0f, // 7
	
		// Left face
		-0.5f, -0.5f, -0.5f, 	0.0f, 1.0f,  -1.0f,  0.0f,  0.0f, // 8
		-0.5f, -0.5f,  0.5f, 	1.0f, 1.0f,  -1.0f,  0.0f,  0.0f, // 9
		-0.5f,  0.5f,  0.5f, 	1.0f, 0.0f,  -1.0f,  0.0f,  0.0f, // 10
		-0.5f,  0.5f, -0.5f, 	0.0f, 0.0f,  -1.0f,  0.0f,  0.0f, // 11
	
		// Right face
		 0.5f, -0.5f,  0.5f, 	0.0f, 1.0f,   1.0f,  0.0f,  0.0f, // 12
		 0.5f, -0.5f, -0.5f, 	1.0f, 1.0f,   1.0f,  0.0f,  0.0f, // 13
		 0.5f,  0.5f, -0.5f, 	1.0f, 0.0f,   1.0f,  0.0f,  0.0f, // 14
		 0.5f,  0.5f,  0.5f, 	0.0f, 0.0f,   1.0f,  0.0f,  0.0f, // 15
	
		// Top face
		-0.5f,  0.5f,  0.5f,	0.0f, 1.0f,   0.0f,  1.0f,  0.0f, // 16
		 0.5f,  0.5f,  0.5f,	1.0f, 1.0f,   0.0f,  1.0f,  0.0f, // 17
		 0.5f,  0.5f, -0.5f,	1.0f, 0.0f,   0.0f,  1.0f,  0.0f, // 18
		-0.5f,  0.5f, -0.5f,	0.0f, 0.0f,   0.0f,  1.0f,  0.0f, // 19
	
		// Bottom face
		-0.5f, -0.5f, -0.5f, 	0.0f, 1.0f,   0.0f, -1.0f,  0.0f, // 20
		 0.5f, -0.5f, -0.5f, 	1.0f, 1.0f,   0.0f, -1.0f,  0.0f, // 21
		 0.5f, -0.5f,  0.5f, 	1.0f, 0.0f,   0.0f, -1.0f,  0.0f, // 22
		-0.5f, -0.5f,  0.5f, 	0.0f, 0.0f,   0.0f, -1.0f,  0.0f  // 23
	};
	
	constexpr uint32_t cubeIndices[] = {
		0, 1, 2,  2, 3, 0,  // Front
		4, 5, 6,  6, 7, 4,  // Back
		8, 9, 10, 10,11, 8,  // Left
		12,13,14, 14,15,12, // Right
		16,17,18, 18,19,16, // Top
		20,21,22, 22,23,20  // Bottom
	};

    vertex_attribute_entry_t vertexAttributes[] = {
        {vertex_attribute_t::position, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 3u},
        {vertex_attribute_t::texcoord, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 2u},
        {vertex_attribute_t::normal, vertex_attribute_type_t::float32, vertex_attribute_frequency_t::vertex, 0u, 3u}
    };

	const uint32_t vertexAttributeCount = sizeof(vertexAttributes) / sizeof(vertexAttributes[0]);
    vertex_format_t* pVertexFormat = createVertexFormat(pGraphicsFrame, vertexAttributes, vertexAttributeCount);

	const uint32_t vertexCount = sizeof(cubeVertices) / calculateVertexStrideSizeInBytes(pVertexFormat);
	constexpr uint32_t indexCount = sizeof(cubeIndices) / sizeof(uint32_t);
    return createIndexedMesh(pGraphicsFrame, pMemoryAllocator, cubeVertices, cubeIndices, vertexCount, indexCount, pVertexFormat);
}

void drawMesh(render_pass_t* pRenderPass, mesh_t* pMesh, material_t* pMaterial)
{
	bindGraphicsPipeline(pRenderPass, pMaterial->pGraphicsPipeline);
	bindVertexBuffer(pRenderPass, pMesh->pVertexBuffer, pMesh->pVertexFormat, 0u);
	draw(pRenderPass, pMesh->vertexOffset, pMesh->vertexCount);
}

void drawIndexedMesh(render_pass_t* pRenderPass, indexed_mesh_t* pIndexedMesh, material_t* pMaterial)
{
	bindGraphicsPipeline(pRenderPass, pMaterial->pGraphicsPipeline);
	bindVertexBuffer(pRenderPass, pIndexedMesh->pVertexBuffer, pIndexedMesh->pVertexFormat, 0u);
	bindIndexBuffer(pRenderPass, pIndexedMesh->pIndexBuffer, index_format_t::unsigned_int_32bit);
	drawIndexed(pRenderPass, pIndexedMesh->indexOffset, pIndexedMesh->indexCount);
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
    test_context_t* pTestContext = (test_context_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if(pTestContext == nullptr)
        return;

    const uint32_t newWidth = LOWORD(lparam);
    const uint32_t newHeight = HIWORD(lparam);

	pTestContext->frameParameter.windowHeight = newHeight;
	pTestContext->frameParameter.windowWidth = newWidth;
    resizeBackBuffer(pTestContext->pRenderContext, newWidth, newHeight);
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

bool setup(test_context_t* pTestContext, HWND pWindowHandle, const uint32_t windowWidth, const uint32_t windowHeight, bool useDebugLayer)
{
    const uint32_t frameBufferCount = 3u;
    const render_context_parameters_t parameters = createDefaultRenderContextParameters(pWindowHandle, frameBufferCount, windowWidth, windowHeight, useDebugLayer);
    if(!createRenderContext(pTestContext->pRenderContext, &parameters))
    {
        return false;
    }

    SetWindowLongPtrA(pWindowHandle, GWLP_USERDATA, (LONG_PTR)pTestContext);
    return true;
}

result_t<test_context_t*> initTestEnvironmentAndWindow(HINSTANCE hInstance, uint32_t width, uint32_t height, const char* pWindowTitle, const test_context_parameters_t* pParameters)
{
    HWND hwnd = setupWindow(hInstance, width, height, pWindowTitle);
    
    if (hwnd == INVALID_HANDLE_VALUE)
    {
		return result_status_t::internal_error;
    }

    render_context_t* pRenderContext = (render_context_t*)calloc(1u, sizeof(render_context_t));

    test_context_t* pTestContext = (test_context_t*)calloc(1u, sizeof(test_context_t));
	pTestContext->pInitCallback 		= pParameters->pInitCallback;
	pTestContext->pShutdownCallback 	= pParameters->pShutdownCallback;
    pTestContext->pFrameCallback 		= pParameters->pFrameCallback;
    pTestContext->pRenderContext 		= pRenderContext;
    pTestContext->pWindowHandle  		= hwnd;
    pTestContext->pWindowTitle   		= pWindowTitle;

    RECT windowRect = {};
    GetClientRect(hwnd, &windowRect);
    const uint32_t actualWindowWidth = windowRect.right - windowRect.left;
    const uint32_t actualWindowHeight = windowRect.bottom - windowRect.top;

    if(!setup(pTestContext, hwnd, actualWindowWidth, actualWindowHeight, pParameters->useDebugLayer))
    {
        shutdownRenderContext(pTestContext->pRenderContext);
        return result_status_t::internal_error;
    }
	
	pTestContext->frameParameter.pAllocator = (memory_allocator_t*)calloc(1u, sizeof(memory_allocator_t));
	createDefaultMemoryAllocator(pTestContext->frameParameter.pAllocator);

	pTestContext->frameParameter.pRenderContext 	= pRenderContext;
	pTestContext->frameParameter.pWindowHandle 		= hwnd;
	pTestContext->frameParameter.windowWidth 		= actualWindowWidth;
	pTestContext->frameParameter.windowHeight 		= actualWindowHeight;
    return pTestContext;
}

int startTest(test_context_t* pTestContext)
{
    LARGE_INTEGER performanceFrequency, startTime, endTime;
	QueryPerformanceFrequency(&performanceFrequency);

	bool firstFrame = true;
    bool loopRunning = true;
	MSG msg = {0};

    char windowTitleBuffer[512] = {};

	test_context_frame_parameter_t* pFrameParameter = &pTestContext->frameParameter;

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
			
			pFrameParameter->pGraphicsFrame = beginNextFrame(pTestContext->pRenderContext);
			if(firstFrame)
			{
				if(pTestContext->pInitCallback != nullptr)
				{
					pTestContext->pInitCallback(pFrameParameter);
				}

				firstFrame = false;
			}

            pTestContext->pFrameCallback(pFrameParameter);

			if(loopRunning == false && pTestContext->pShutdownCallback != nullptr)
			{
				pTestContext->pShutdownCallback(pFrameParameter);
			}
			finishFrame(pTestContext->pRenderContext, pFrameParameter->pGraphicsFrame);
        }
        QueryPerformanceCounter(&endTime);

        const LONGLONG frameTimeDelta = endTime.QuadPart - startTime.QuadPart;
        const float frameTimeDeltaInMs = ((float)frameTimeDelta / (float)performanceFrequency.QuadPart) * 1000.f;

		pTestContext->frameParameter.deltaTimeInMs = frameTimeDeltaInMs;
		pTestContext->frameParameter.totalFrameTimeInMs += frameTimeDeltaInMs;

        snprintf(windowTitleBuffer, sizeof(windowTitleBuffer), "%s - %.3f ms", pTestContext->pWindowTitle, frameTimeDeltaInMs);
        SetWindowTextA(pTestContext->pWindowHandle, windowTitleBuffer);
	}

    shutdownRenderContext(pTestContext->pRenderContext);
    return 0;
}