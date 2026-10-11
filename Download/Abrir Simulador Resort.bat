@echo off
title Simulador Resort - compilando
cd /d D:\
echo Compilando o Simulador Resort (pode levar varios minutos na primeira vez)...
call "D:\UE_5.8\Engine\Build\BatchFiles\Build.bat" SimuladorUeEditor Win64 Development -Project="D:\SimuladorUE\SimuladorUe.uproject" -WaitMutex > "D:\SimuladorUE\build_log.txt" 2>&1
if errorlevel 1 (
  echo.
  echo ERRO NA COMPILACAO. Veja o arquivo D:\SimuladorUE\build_log.txt
  echo Ultimas linhas:
  powershell -NoProfile -Command "Get-Content 'D:\SimuladorUE\build_log.txt' -Tail 25"
  pause
  exit /b 1
)
start "" "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\SimuladorUE\SimuladorUe.uproject"
