set DXC=%VULKAN_SDK%\bin\dxc.exe
del triangle_vs.h
del triangle_ps.h
%DXC% -spirv -T vs_6_0 -E vs %cd%\triangle.hlsl -Fh %cd%\triangle_vs.h -Vn triangle_vs -D VS
%DXC% -spirv -T ps_6_0 -E ps %cd%\triangle.hlsl -Fh %cd%\triangle_ps.h -Vn triangle_ps -D PS
