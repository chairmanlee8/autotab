#include "main.h"

Application::Application()
	: _hWnd(NULL)
	, _pDirect2dFactory(NULL)
	, _pRenderTarget(NULL)
	, _pLightSlateGrayBrush(NULL)
	, _pCornflowerBlueBrush(NULL)
	, _buffer(0)
	, _numBytes(0)
{
}

Application::~Application()
{
	SafeRelease(&_pDirect2dFactory);
	SafeRelease(&_pRenderTarget);
	SafeRelease(&_pLightSlateGrayBrush);
	SafeRelease(&_pCornflowerBlueBrush);
}

void Application::Run()
{
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

HRESULT Application::Initialize()
{
	HRESULT hr;
	hr = CreateDeviceIndependentResources();

	if(SUCCEEDED(hr)) {
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = Application::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
		wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
		wcex.lpszClassName = L"Autotab";

		RegisterClassEx(&wcex);

		FLOAT dpiX, dpiY;
		_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);

		_hWnd = CreateWindow(
			L"Autotab", L"Autotab", 
			WS_OVERLAPPEDWINDOW, 
			CW_USEDEFAULT, CW_USEDEFAULT,
			static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
			static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
			NULL,
			NULL,
			HINST_THISCOMPONENT,
			this
		);

		hr = _hWnd ? S_OK : E_FAIL;
		if(SUCCEEDED(hr))
		{
			ShowWindow(_hWnd, SW_SHOWNORMAL);
			UpdateWindow(_hWnd);
		}
	}

	return hr;
}

LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if(msg == WM_CREATE) {
		LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
		Application *app = (Application *)pcs->lpCreateParams;

		::SetWindowLongPtrW(hWnd, GWLP_USERDATA, PtrToUlong(app));

		// Initialize ffmpeg
		av_register_all();

		result = 1;
	} else {
		Application *app = reinterpret_cast<Application *>(static_cast<LONG_PTR>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA)));
		bool wasHandled = false;

		if(app) {
			switch(msg) {
			case WM_SIZE:
				{
					UINT width = LOWORD(lParam), height = HIWORD(lParam);
					app->OnResize(width, height);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_DISPLAYCHANGE:
				{
					InvalidateRect(hWnd, NULL, FALSE);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_PAINT:
				{
					app->OnRender();
					ValidateRect(hWnd, NULL);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_COMMAND:
				{
					switch(LOWORD(wParam)) {
					case ID_VIDEO_LOAD:
						{
							// Get video filename http://msdn.microsoft.com/en-us/library/windows/desktop/ff485843(v=vs.85).aspx
							IFileDialog *pfd = NULL;
							HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pfd));

							if(SUCCEEDED(hr))
							{
								hr = pfd->Show(NULL);
								if(SUCCEEDED(hr))
								{
									IShellItem *pItem;
									hr = pfd->GetResult(&pItem);
									if(SUCCEEDED(hr))
									{
										PWSTR pszFilePath;
										hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

										if(SUCCEEDED(hr))
										{
											app->OnVideoLoad(pszFilePath);
											CoTaskMemFree(pszFilePath);
										}
										pItem->Release();
									}
								}

								pfd->Release();
							}

							CoUninitialize();
						}
						break;
					case ID_VIDEO_EXIT:
						PostQuitMessage(0);
						break;
					}
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_DESTROY:
				{
					PostQuitMessage(0);
				}
				result = 1;
				wasHandled = true;
				break;
			}
		}

		if(!wasHandled) {
			result = DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}

	return result;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if(SUCCEEDED(CoInitialize(NULL)))
	{
		{
			Application app;
			if(SUCCEEDED(app.Initialize())) 
			{
				app.Run();
			}
		}
		CoUninitialize();
	}

	return 0;
}