@REM cd ..

dxc.exe -Od Zi -T vs_6_0 /enable_unbounded_descriptor_tables -E VS Shaders/Basic.hlsl -Fo Shaders/vs.bin
dxc.exe -Od Zi -T ps_6_0 /enable_unbounded_descriptor_tables -E PS Shaders/Basic.hlsl -Fo Shaders/ps.bin
dxc.exe -Od Zi -T cs_6_0 -E processGIF Shaders/ProcessGIF_CS.hlsl -Fo Shaders/cs.bin