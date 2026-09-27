#pragma once
#include "pch.h"

const TCHAR *PathFindFileName(const TCHAR *pszPath);
HRESULT PathPopFileName(TCHAR *pszPath);
HRESULT PathAppend(TCHAR *pszPath, const TCHAR *pszPath2);
HRESULT CopyBitmap(HBITMAP *phbmDest, HBITMAP hbmSrc, RECT *prcCrop = nullptr);
