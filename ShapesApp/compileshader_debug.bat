@REM cd ..

dxc.exe -Od Zi -T vs_6_0 /enable_unbounded_descriptor_tables -E VS Shaders/color.hlsl -Fo Shaders/vs.bin
dxc.exe -Od Zi -T ps_6_0 /enable_unbounded_descriptor_tables -E PS Shaders/color.hlsl -Fo Shaders/ps.bin