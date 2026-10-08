#include "pch.h"
#include "portablep.h"

typedef BOOL (WINAPI *AlphaBlend_t)(
	HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest,
	HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn
);

AlphaBlend_t g_pfnAlphaBlend = nullptr;

HRESULT PortableAlphaBlend(
	HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest,
	HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn
)
{
	if (!g_pfnAlphaBlend)
	{
		return E_NOTIMPL;
	}

	return g_pfnAlphaBlend(hdcDest, xoriginDest, yoriginDest, wDest, hDest, hdcSrc, xoriginSrc, yoriginSrc, wSrc, hSrc, ftn)
		? S_OK
		: S_FALSE;
}

bool PortableIsAlphaBlendAvailable()
{
	return g_pfnAlphaBlend ? true : false;
}

HRESULT PortableInitialize()
{
	HMODULE hmMsimg32 = LoadLibrary(TEXT("msimg32.dll"));
	if (hmMsimg32)
	{
		g_pfnAlphaBlend = (AlphaBlend_t)GetProcAddress(hmMsimg32, "AlphaBlend");
	}

	return S_OK;
}