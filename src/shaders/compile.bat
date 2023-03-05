@echo off

v-fxc.exe -i ./lib -i ./lib/lq -o laserbeam_lq.fxc laserbeam.fx
if ERRORLEVEL 1 goto exit
v-fxc.exe -i ./lib -i ./lib/hq -o laserbeam.fxc laserbeam.fx
if ERRORLEVEL 1 goto exit

@REM copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\win32_40_final\laserbeam.fxc"
@REM copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\win32_40_lq_final\laserbeam.fxc"
@REM copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\win32_nvstereo_final\laserbeam.fxc"
@REM copy laserbeam.fxc "D:\sources\fivem\code\bin\five\debug\plugins\shaders\laserbeam.fxc"

:exit