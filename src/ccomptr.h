#pragma once
#include <windows.h>

template <class TUnknown>
class CComPtr
{
	TUnknown *_p;

public:
	inline CComPtr()
		: _p(nullptr)
	{
	}

	inline CComPtr(CComPtr &rOther)
		: _p(rOther._p)
	{
	}

	inline CComPtr(TUnknown *pUnk)
		: _p(pUnk)
	{
	}

	inline CComPtr(TUnknown &rUnk)
		: _p(&rUnk)
	{
	}

	inline ~CComPtr()
	{
		if (_p)
		{
			_p->Release();
		}
	}

	inline TUnknown *ReleaseAndGet()
	{
		TUnknown *p = _p;
		_p = nullptr;
		return p;
	}

	inline TUnknown *Get()
	{
		return _p;
	}

	inline TUnknown **operator &()
	{
		return &_p;
	}

	inline TUnknown *operator ->()
	{
		return _p;
	}
};