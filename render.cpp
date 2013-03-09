#include "main.h"

HRESULT Application::CreateDeviceIndependentResources()
{
	HRESULT hr = S_OK;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pDirect2dFactory);
	return hr;
}

HRESULT Application::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if(!_pRenderTarget)
	{
		RECT rc;
		GetClientRect(_hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
		hr = _pDirect2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(_hWnd, size), &_pRenderTarget);

		if(SUCCEEDED(hr)) {
			hr = _pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &_pLightSlateGrayBrush);
		}
		if(SUCCEEDED(hr)) {
			hr = _pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &_pCornflowerBlueBrush);
		}
	}

	return hr;
}

void Application::DiscardDeviceResources()
{
	SafeRelease(&_pRenderTarget);
	SafeRelease(&_pLightSlateGrayBrush);
	SafeRelease(&_pCornflowerBlueBrush);
}

void Application::OnResize(UINT width, UINT height)
{
	if(_pRenderTarget) {
		_pRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}

HRESULT Application::OnRender()
{
	HRESULT hr = S_OK;
	hr = CreateDeviceResources();

	if(SUCCEEDED(hr))
	{
		_pRenderTarget->BeginDraw();
		_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		D2D1_SIZE_F rtSize = _pRenderTarget->GetSize();

		// Draw a grid background.
		int width = static_cast<int>(rtSize.width);
		int height = static_cast<int>(rtSize.height);

		for (int x = 0; x < width; x += 10) {
			_pRenderTarget->DrawLine(D2D1::Point2F(static_cast<FLOAT>(x), 0.0f), D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height), _pLightSlateGrayBrush, 0.5f);
		}

		for (int y = 0; y < height; y += 10) {
			_pRenderTarget->DrawLine(D2D1::Point2F(0.0f, static_cast<FLOAT>(y)), D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)), _pLightSlateGrayBrush, 0.5f);
		}

		hr = _pRenderTarget->EndDraw();
	}

	if(hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		DiscardDeviceResources();
	}

	return hr;
}