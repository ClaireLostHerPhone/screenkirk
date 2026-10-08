#pragma once
#include "pch.h"

class CCursorRenderer
{
	HBITMAP _hbmScreenshot;
	HBITMAP _hbmCursorColor;
	HBITMAP _hbmCursorMask;
	POINT _ptCursor;

	HRESULT _BlurCursorBitmap(HBITMAP hbm);
	HRESULT _PremultiplyAlpha(HBITMAP hbmIn, OUT HBITMAP *phbmOut);

public:
	CCursorRenderer(HBITMAP hbmScreenshot, HBITMAP hbmCursorColor, HBITMAP hbmCursorMask, POINT ptCursor)
		: _hbmScreenshot(hbmScreenshot)
		, _hbmCursorColor(hbmCursorColor)
		, _hbmCursorMask(hbmCursorMask)
	{
		_ptCursor.x = ptCursor.x;
		_ptCursor.y = ptCursor.y;
	}

	HRESULT RenderCursor(HBITMAP *phbmCursor, RECT *prcCursor);
};