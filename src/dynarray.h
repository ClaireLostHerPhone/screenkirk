#pragma once
#include "pch.h"
#include <vector>

// Dynamic array implementation. In this implementation, it's an interface wrapper for
// std::vector which already provides the desired functionality. For portability's sake
// (in case I'm compiling to a platform without C++'s standard library) I am making this
// interface.
template <typename TContained>
class CDynamicArray
{
    using Base = std::vector<TContained>;
    using Iterator = typename Base::iterator;
    Base _v;

public:
    inline Iterator begin()
    {
        return _v.begin();
    }

    inline Iterator end()
    {
        return _v.end();
    }

    inline bool IsEmpty()
    {
        return _v.empty();
    }

    inline size_t GetSize()
    {
        return _v.size();
    }

    inline size_t GetCapacity()
    {
        return _v.capacity();
    }

    inline TContained &At(size_t position)
    {
        return _v.at(position);
    }

    inline TContained *First()
    {
        return &_v.front();
    }

    inline TContained *Last()
    {
        return &_v.back();
    }

    inline TContained &operator[](size_t position)
    {
        return _v.operator[](position);
    }

    inline HRESULT Push(const TContained &scalar)
    {
        try
        {
            _v.push_back(scalar);
            return S_OK;
        }
        catch (std::bad_alloc ex)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_FAIL;
        }
    }

    inline HRESULT Push(TContained &&moved)
    {
        try
        {
            _v.push_back(std::move(moved));
            return S_OK;
        }
        catch (std::bad_alloc ex)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_FAIL;
        }
    }

    inline HRESULT Pop(TContained *pOut = nullptr)
    {
        try
        {
            if (pOut)
            {
                *pOut = *Last();
            }
            _v.pop_back();
            return S_OK;
        }
        catch (std::bad_alloc ex)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_FAIL;
        }
    }

    HRESULT Remove(size_t offset)
    {
        if (offset == GetSize() - 1)
        {
            return Pop();
        }
        else if (offset < 0 || offset > GetSize())
        {
            return E_BOUNDS;
        }
        else
        {
            try
            {
                _v.erase(_v.begin() + offset);
                return S_OK;
            }
            catch (std::bad_alloc ex)
            {
                return E_OUTOFMEMORY;
            }
            catch (...)
            {
                return E_FAIL;
            }
        }
    }

    inline HRESULT Resize(size_t sizeNew)
    {
        try
        {
            _v.resize(sizeNew);
            return S_OK;
        }
        catch (std::bad_alloc ex)
        {
            return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_FAIL;
        }
    }
};