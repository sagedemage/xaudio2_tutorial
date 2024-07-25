#include "hresult_debugger.h"

void verbose_debug_hresult(HRESULT hr, std::string description) {
	// Description
	std::string des = description + "\n";
	OutputDebugStringA(des.c_str());

	// Error message
	_com_error err(hr);
	LPCTSTR errMsg = err.ErrorMessage();
	OutputDebugStringA("Error message: ");
	OutputDebugStringW(errMsg);
	OutputDebugStringA("\n");

	// Error code
	std::string errCode = "Error code: " + std::to_string(err.WCode());
	OutputDebugStringA(errCode.c_str());
	OutputDebugStringA("\n");
}