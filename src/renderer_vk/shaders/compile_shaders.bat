set DXC=%VULKAN_SDK%\bin\dxc.exe
del *_vs.h
del *_ps.h

%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T vs_6_0 -E vs %cd%\generic.hlsl -Fh %cd%\generic_vs.h -Vn generic_vs -D VS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T ps_6_0 -E ps %cd%\generic.hlsl -Fh %cd%\generic_ps.h -Vn generic_ps -D PS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T vs_6_0 -E vs %cd%\gamma.hlsl -Fh %cd%\gamma_vs.h -Vn gamma_vs -D VS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T ps_6_0 -E ps %cd%\gamma.hlsl -Fh %cd%\gamma_ps.h -Vn gamma_ps -D PS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T vs_6_0 -E vs %cd%\imgui.hlsl -Fh %cd%\imgui_vs.h -Vn imgui_vs -D VS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T ps_6_0 -E ps %cd%\imgui.hlsl -Fh %cd%\imgui_ps.h -Vn imgui_ps -D PS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T vs_6_0 -E vs %cd%\blit.hlsl -Fh %cd%\blit_vs.h -Vn blit_vs -D VS
%DXC% -fspv-debug=vulkan-with-source -O0 -Qembed_debug -Zi -spirv -T ps_6_0 -E ps %cd%\blit.hlsl -Fh %cd%\blit_ps.h -Vn blit_ps -D PS