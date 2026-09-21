#pragma once
#include "pch.h"
#include <vector>

// Dynamic array implementation. In this implementation, it's an interface wrapper for
// std::vector which already provides the desired functionality. For portability's sake
// (in case I'm compiling to a platform without C++'s standard library) I am making this
// interface.
template <typename TContained>
class CDynamicArray : private std::vector<TContained>
{
    using Base = std::vector<TContained>;

public:
    CDynamicArray()
        : Base()
    {
    }

    ~CDynamicArray()
    {
    }

    inline bool IsEmpty()
    {
        return Base::empty();
    }

    inline size_t GetSize()
    {
        return Base::size();
    }

    inline size_t GetTotalSize()
    {
        return Base::capacity();
    }

    inline TContained &At(size_t position)
    {
        return Base::at(position);
    }

    inline TContained &operator[](size_t position)
    {
        return Base::operator[](position);
    }

    inline HRESULT Push(TContained scalar)
    {
        try
        {
            Base::push_back(scalar);
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

    inline HRESULT Resize(size_t sizeNew)
    {
        try
        {
            Base::resize(sizeNew);
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