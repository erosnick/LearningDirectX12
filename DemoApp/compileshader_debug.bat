@REM cd ..

dxc.exe -Od Zi -T vs_6_0 -E VS Shaders/color.hlsl -Fo Shaders/vs.bin
dxc.exe -Od Zi -T ps_6_0 -E PS Shaders/color.hlsl -Fo Shaders/ps.bin