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

		// Try to draw bitmap.
		if(_buffer != 0) {
			// Why? http://social.msdn.microsoft.com/Forums/en-US/vclanguage/thread/17e9e6bd-aa91-40a3-afc9-3241a24afe00
			IWICBitmap *pEmbeddedBitmap;
			ID2D1Bitmap *pBitmap;
			IWICImagingFactory *pFactory = NULL;
			HDC screen = GetDC(0);
			float dpiScaleX = GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
			float dpiScaleY = GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
			ReleaseDC(0, screen);

			CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*) &pFactory);

			HDC memDC = CreateCompatibleDC(screen);
			hr = pFactory->CreateBitmapFromMemory(_width, _height, GUID_WICPixelFormat32bppPBGRA, _width * 4, _numBytes, _buffer, &pEmbeddedBitmap);

			if(SUCCEEDED(hr)) {
				hr = _pRenderTarget->CreateBitmapFromWicBitmap(pEmbeddedBitmap, &pBitmap);
				_pRenderTarget->DrawBitmap(pBitmap);
			}
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

void Application::OnVideoLoad(PWSTR pszFileName)
{
	// Use ffmpeg to load the video frames from the file. http://dranger.com/ffmpeg/tutorial01.html
	AVFormatContext *pFormatContext = NULL;
	int err;
	char err_buf[256];
	wchar_t werr_buf[256];

	// Warning: assumes pszFileName does not contain non-ASCII chars.
	WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, err_buf, sizeof(err_buf), NULL, NULL);
	
	if((err = avformat_open_input(&pFormatContext, err_buf, NULL, NULL)) < 0) {
		av_strerror(err, err_buf, sizeof(err_buf));
		swprintf(werr_buf, sizeof(werr_buf), L"%hs", err_buf);
		MessageBox(NULL, werr_buf, L"Could not open video file.", MB_OK);
		return;
	}

	if(avformat_find_stream_info(pFormatContext, NULL) < 0) {
		MessageBox(NULL, L"Could not find get file stream info.", L"Error", MB_OK);
		avformat_close_input(&pFormatContext);
		return;
	}

	// Find first video stream and open with codec.
	AVCodecContext *pCodecContext = NULL;
	AVCodec *pCodec = NULL;
	int videoStream = -1;

	for(int i = 0; i < (int)pFormatContext->nb_streams; i++) {
		if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}

	if(videoStream == -1) {
		MessageBox(NULL, L"Could not find any video streams in file.", L"Error", MB_OK);
		avformat_close_input(&pFormatContext);
		return;
	}

	pCodecContext = pFormatContext->streams[videoStream]->codec;
	pCodec = avcodec_find_decoder(pCodecContext->codec_id);

	if(pCodec == NULL) {
		MessageBox(NULL, L"Could not find a decoder for the video format.", L"Error", MB_OK);
		avformat_close_input(&pFormatContext);
		return;
	}

	if(avcodec_open(pCodecContext, pCodec) < 0) {
		MessageBox(NULL, L"Could not open codec.", L"Error", MB_OK);
		avformat_close_input(&pFormatContext);
		return;
	}

	// Read data from middle frame.
	AVFrame *pFrame = avcodec_alloc_frame();
	AVFrame *pRGBFrame = avcodec_alloc_frame();

	_numBytes = avpicture_get_size(PIX_FMT_BGRA, pCodecContext->width, pCodecContext->height);
	_buffer = (unsigned char *) av_malloc(_numBytes * sizeof(unsigned char));

	avpicture_fill((AVPicture*) pRGBFrame, _buffer, PIX_FMT_BGRA, pCodecContext->width, pCodecContext->height);

	int frameFinished, nFrame = 0;
	AVPacket packet;
	SwsContext* pSWSContext = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height, PIX_FMT_BGRA, SWS_BILINEAR, 0, 0, 0);

	while(av_read_frame(pFormatContext, &packet) >= 0) {
		if(packet.stream_index == videoStream) {
			avcodec_decode_video2(pCodecContext, pFrame, &frameFinished, &packet);

			if(frameFinished) {
				if(++nFrame == 1000) {
					sws_scale(pSWSContext, pFrame->data, pFrame->linesize, 0, pCodecContext->height, pRGBFrame->data, pRGBFrame->linesize);
					_width = pCodecContext->width;
					_height = pCodecContext->height;
					_stride = pRGBFrame->linesize[0];

					av_free_packet(&packet);
					break;
				}
			}
		}

		av_free_packet(&packet);
	}

	av_free(pSWSContext);
	av_free(pRGBFrame);
	av_free(pFrame);

	avcodec_close(pCodecContext);
	avformat_close_input(&pFormatContext);

	// Render
	OnRender();
}