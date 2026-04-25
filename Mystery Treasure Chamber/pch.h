// Crucial Win32 definitions to keep the namespace clean and fast
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // Prevents Windows from breaking std::min and std::max

#include <windows.h>

// DirectX and COM Pointers
#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>

// Math and Colors
#include <DirectXColors.h>
#include <DirectXMath.h>

// Standard C++
#include <memory>

// --- DirectX & Windows Libraries ---
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")