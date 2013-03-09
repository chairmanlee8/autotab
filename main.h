#ifndef __MAIN_H
#define __MAIN_H

// Start template code via http://msdn.microsoft.com/en-us/library/windows/desktop/dd370994(v=vs.85).aspx

// Disable warning C4005 cf. http://connect.microsoft.com/VisualStudio/feedback/details/621653
#pragma warning (disable: 4005)

// Windows
#include <Windows.h>
#include <CommCtrl.h>
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include "resource.h"
#pragma comment(lib, "Comctl32.lib")

// C++ Standard Library
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>

// Direct2D
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment(lib, "d2d1.lib")

// FFmpeg
extern "C" {
	#include "ffmpeg/libavcodec/avcodec.h"
	#include "ffmpeg/libavformat/avformat.h"
}
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")

// Common Control Styles
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/* Definitions */

template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
    if (*ppInterfaceToRelease != NULL) {
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

/* Globals */

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
    {L"MP4 Container (*.mp4)",       L"*.mp4"},
    {L"FLV Container (*.flv)",       L"*.flv"},
    {L"All Documents (*.*)",         L"*.*"}
};

/* Application Class */

class Application {
public:
	Application();
	~Application();

	HRESULT Initialize();
	void Run();
private:
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();

	HRESULT OnRender();
	void OnResize(UINT width, UINT height);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	HWND _hWnd;
	ID2D1Factory* _pDirect2dFactory;
	ID2D1HwndRenderTarget* _pRenderTarget;
	ID2D1SolidColorBrush* _pLightSlateGrayBrush;
	ID2D1SolidColorBrush* _pCornflowerBlueBrush;
};

#endif