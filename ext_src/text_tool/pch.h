#pragma once

#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include "screenkirk.h"

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define ID_HOTKEY_SCREENSHOT (20)

extern HINSTANCE g_hinst;