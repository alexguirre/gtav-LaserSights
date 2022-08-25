@echo off

v-fxc.exe -i ./lib laserbeam.fx
if ERRORLEVEL 1 goto exit

copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\win32_40_final\laserbeam.fxc"
copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\win32_40_lq_final\laserbeam.fxc"
copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\win32_nvstereo_final\laserbeam.fxc"
copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\laserbeam.fxc"

:exit