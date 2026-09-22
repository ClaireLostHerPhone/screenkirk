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

    inline Base::iterator begin()
    {
        return Base::begin();
    }

    inline Base::iterator end()
    {
        return Base::end();
    }

    inline bool IsEmpty()
    {
        return Base::empty();
    }

    inline size_t GetSize()
    {
        return Base::size();
    }

    inline size_t GetCapacity()
    {
        return Base::capacity();
    }

    inline TContained &At(size_t position)
    {
        return Base::at(position);
    }

    inline TContained *First()
    {
        return &Base::front();
    }

    inline TContained *Last()
    {
        return &Base::back();
    }

    inline TContained &operator[](size_t position)
    {
        return Base::operator[](position);
    }

    inline HRESULT Push(const TContained &scalar)
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

    inline HRESULT Push(TContained &&moved)
    {
        try
        {
            Base::push_back(std::move(moved));
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
            Base::pop_back();
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
                Base::erase(Base::begin() + offset);
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