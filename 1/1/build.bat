@echo off
chcp 65001 >nul
echo ============================================
echo  任务拆解引擎 - 编译构建脚本
echo ============================================

:: 检测编译器
where g++ >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] 使用 MinGW g++ 编译...
    g++ -std=c++17 -O2 -Wall -o TaskDecomposer.exe main.cpp RequirementParser.cpp TaskDecompositionEngine.cpp
    goto :done
)

where clang++ >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] 使用 clang++ 编译...
    clang++ -std=c++17 -O2 -Wall -o TaskDecomposer.exe main.cpp RequirementParser.cpp TaskDecompositionEngine.cpp
    goto :done
)

where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] 使用 MSVC 编译...
    cl /EHsc /std:c++17 /O2 /Fe:TaskDecomposer.exe main.cpp RequirementParser.cpp TaskDecompositionEngine.cpp
    goto :done
)

echo [ERROR] 未找到可用的C++编译器 (g++, clang++, cl)！
echo         请安装 MinGW-w64 或 Visual Studio Build Tools
pause
exit /b 1

:done
if %errorlevel% equ 0 (
    echo [OK] 编译成功！运行: TaskDecomposer.exe
) else (
    echo [ERROR] 编译失败，请检查错误信息。
)
pause
