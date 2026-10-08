#pragma once
#include <windows.h>

HRESULT PortableInitialize();

HRESULT PortableAlphaBlend(
	HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest,
	HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn
);

bool PortableIsAlphaBlendAvailable();