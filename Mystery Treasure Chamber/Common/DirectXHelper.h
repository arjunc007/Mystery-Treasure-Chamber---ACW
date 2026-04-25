#pragma once

#include <stdexcept>
#include <fstream>
#include <vector>
#include <string>
#include <stdint.h>

namespace DX
{
	class com_exception : public std::exception
	{
	private:
		HRESULT result;
	public:
		com_exception(HRESULT hr) : result(hr) {}
		virtual const char* what() const override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}
		HRESULT get_result() const { return result; }
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw com_exception(hr);
		}
	}

	// Function that reads from a binary file asynchronously.
	inline std::vector<uint8_t> ReadData(const std::wstring& filename)
	{
		std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
		
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file.");
		}

		std::streampos size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> returnBuffer(static_cast<size_t>(size));
		file.read(reinterpret_cast<char*>(returnBuffer.data()), size);
		file.close();

		return returnBuffer;
	}

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif
}
