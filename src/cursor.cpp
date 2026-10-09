#include "pch.h"
#include "cursor.h"
#include "util.h"

//
// It is practically impossible to retrieve the current frame of the current animated cursor, since this
// is opaque data in Windows USER. Even at the extent to which it is possible, i.e. listening for cursor
// changes ourselves and calculating the time difference, I find it difficult to find worthwhile.
//

HRESULT CCursorRenderer::_BlurCursorBitmap(HBITMAP hbm)
{
	// Not yet implemented.
	return S_OK;
}

HRESULT CCursorRenderer::_PremultiplyAlpha(HBITMAP hbmIn, OUT HBITMAP *phbmOut)
{
	BITMAP bm;
	if (!GetObject(hbmIn, sizeof(bm), &bm))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	bool fAnyAlphaPixel = false;

	HDC hdcDesktop = GetDC(HWND_DESKTOP);
	if (hdcDesktop)
	{
		HDC hdc = CreateCompatibleDC(hdcDesktop);
		if (hdc)
		{
			BITMAPINFO bmi = { 0 };
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = bm.bmWidth;
			bmi.bmiHeader.biHeight = bm.bmHeight;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biSizeImage = 0;

			void *pvPixels = nullptr;
			*phbmOut = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvPixels, nullptr, 0);
			if (*phbmOut)
			{
				HGDIOBJ hObjOld = SelectObject(hdc, *phbmOut);

				HDC hdcOrig = CreateCompatibleDC(hdcDesktop);
				HGDIOBJ hObjOld2 = SelectObject(hdcOrig, hbmIn);

				BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcOrig, 0, 0, SRCCOPY);

				SelectObject(hdcOrig, hObjOld2);
				DeleteDC(hdcOrig);

				int cLength = bm.bmWidth * bm.bmHeight;
				ULONG *pulSrc = (ULONG *)pvPixels;
				BYTE bFirstAlpha = (pulSrc[0] & 0xFF000000) >> 24;
				for (int i = 0; i < cLength; i++)
				{
					BYTE b = GetBValue(*pulSrc);
					BYTE g = GetGValue(*pulSrc);
					BYTE r = GetRValue(*pulSrc);
					BYTE a = (pulSrc[i] & 0xFF000000) >> 24;

					if (a != bFirstAlpha)
					{
						b = (b * a) / 255;
						g = (g * a) / 255;
						r = (r * a) / 255;

						*pulSrc = b | (g << 8) | (r << 16) | (a << 24);

						fAnyAlphaPixel = true;
					}
				}

				SelectObject(hdc, hObjOld);
			}

			DeleteDC(hdc);
		}

		ReleaseDC(HWND_DESKTOP, hdcDesktop);
	}

	if (!fAnyAlphaPixel)
	{
		DeleteObject(*phbmOut);
		*phbmOut = nullptr;
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CCursorRenderer::RenderCursor(HBITMAP *phbmCursor, RECT *prcCursor)
{
	BITMAP bm;
	if (!GetObject(_hbmCursorColor, sizeof(bm), &bm) &&
		!GetObject(_hbmCursorMask, sizeof(bm), &bm))
	{
		return E_FAIL;
	}

	HDC hdcDesktop = GetDC(HWND_DESKTOP);
	HDC hdc = CreateCompatibleDC(hdcDesktop);
	HBITMAP hbm = CreateCompatibleBitmap(hdcDesktop, bm.bmWidth, bm.bmHeight);
	HGDIOBJ hBmpOld = SelectObject(hdc, hbm);

	// Paint the background of the cursor:
	HDC hdcScreenshot = CreateCompatibleDC(hdcDesktop);
	HGDIOBJ hScreenshotOld = SelectObject(hdcScreenshot, _hbmScreenshot);
	BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcScreenshot, _ptCursor.x, _ptCursor.y, SRCCOPY);
	SelectObject(hdcScreenshot, hScreenshotOld);
	DeleteDC(hdcScreenshot);

	// We just use this procedure to check if the cursor has premultiplied alpha. We don't actually
	// use the result.
	HBITMAP hbmCursorPremult = nullptr;
	_PremultiplyAlpha(_hbmCursorColor, &hbmCursorPremult);
	
	if (hbmCursorPremult)
	{
		HDC hdcCursor = CreateCompatibleDC(hdcDesktop);
		HGDIOBJ hBmpOld = SelectObject(hdcCursor, hbmCursorPremult);

		// If we have an alpha cursor, then we'll just draw it with DrawIcon. This avoids alpha issues
		// with animated cursors that I couldn't figure out how to avoid with AlphaBlending the bitmap
		// from GetIconInfo.
		DrawIcon(hdc, 0, 0, _hcursor);

		DeleteDC(hdcCursor);
	}
	else if (_hbmCursorColor) // Premultiplied cursor is not available.
	{
		HDC hdcCursor = CreateCompatibleDC(hdcDesktop);
		HGDIOBJ hBmpOld = SelectObject(hdcCursor, _hbmCursorMask);

		BitBlt(
			hdc,
			0, 0,
			bm.bmWidth, bm.bmHeight,
			hdcCursor,
			0, 0,
			SRCAND
		);

		SelectObject(hdcCursor, hBmpOld);
		hBmpOld = SelectObject(hdcCursor, _hbmCursorColor);

		BitBlt(
			hdc,
			0, 0,
			bm.bmWidth, bm.bmHeight,
			hdcCursor,
			0, 0,
			SRCINVERT
		);
		SelectObject(hdcCursor, hBmpOld);
		DeleteDC(hdcCursor);
	}
	else // No color cursor is available, so the mask cursor will be drawn instead.
	{
		HDC hdcCursor = CreateCompatibleDC(hdcDesktop);
		HGDIOBJ hBmpOld = SelectObject(hdcCursor, _hbmCursorMask);

		// The upper half of the mask is the AND channel, and the lower half is the XOR channel.
		BitBlt(
			hdc,
			0, 0,
			bm.bmWidth, bm.bmHeight / 2,
			hdcCursor,
			0, 0,
			SRCAND
		);
		BitBlt(
			hdc,
			0, 0,
			bm.bmWidth, bm.bmHeight / 2,
			hdcCursor,
			0, bm.bmHeight / 2,
			SRCINVERT
		);
		SelectObject(hdcCursor, hBmpOld);
		DeleteDC(hdcCursor);

		bm.bmHeight /= 2;
	}

	if (hbmCursorPremult)
		DeleteObject(hbmCursorPremult);

	SelectObject(hdc, hBmpOld);
	DeleteDC(hdc);
	ReleaseDC(HWND_DESKTOP, hdcDesktop);

	RECT rcCursor;
	rcCursor.left = _ptCursor.x;
	rcCursor.top = _ptCursor.y;
	rcCursor.right = rcCursor.left + bm.bmWidth;
	rcCursor.bottom = rcCursor.top + bm.bmHeight;
	*phbmCursor = hbm;
	*prcCursor = rcCursor;
	return S_OK;
}
