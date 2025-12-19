using K15;
using System.Collections.Generic;
using System.IO;

namespace Buildscript
{
    public class D3D12RendererBuildSetup : BaseBuildSetup
    {
        public void addCompilers(BuildPlatform buildPlatform, ref List<BaseCompiler> compilers)
        {
            compilers.Add(new VisualStudioCPPCompiler());
        }

        public void addBuildConfigurations(BuildPlatform buildPlatform, ref BuildConfigurationCollection buildConfigurations)
        {
            
        }
        
        public void setupBuild(BuildContext buildContext)
        {
            BuildProject imGuiLibrary = buildContext.addProject(BuildProjectType.StaticLibrary, "imgui");
            imGuiLibrary.addFiles("tests/imgui/imgui*.cpp");
            imGuiLibrary.addFile("tests/imgui/backends/imgui_impl_dx12.cpp");
            imGuiLibrary.addFile("tests/imgui/backends/imgui_impl_win32.cpp");

            BuildProject samples = buildContext.addProject(BuildProjectType.Executable, "samples");
            samples.addDependency(imGuiLibrary);
            samples.addLibrary(imGuiLibrary.outputFilePath);
            samples.addFile("tests/test_base_imgui.cpp");

            samples.copyFile("win32/x64/dxcompiler.dll");
            samples.copyFile("win32/x64/d3dcompiler_47.dll");
            samples.copyFile("win32/x64/dxil.dll");
            samples.copyFile("win32/x64/WinPixEventRuntime.dll");

            samples.copyFile("tests/smiley.png");
            samples.copyFile("tests/sample_pixel_shader.hlsl");
            samples.copyFile("tests/sample_vertex_shader.hlsl");
            samples.copyFile("tests/Box.gltf");
            samples.copyFile("tests/Box0.bin");
        }
    };
}
