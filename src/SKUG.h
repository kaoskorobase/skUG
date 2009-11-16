#ifndef SKUG_H_INCLUDED
#define SKUG_H_INCLUDED

#include <SC_PlugIn.h>
#include <stdexcept>
#include <cstring>

namespace SKUG
{
    template <class T> inline T* rtAlloc(InterfaceTable* ft, World* world, size_t n)
    {
        T* ptr = (T*)RTAlloc(world, n*sizeof(T));
        if (ptr == 0) throw std::runtime_error("rtAlloc failed");
        return ptr;
    }
    
    template <class T> inline void rtFree(InterfaceTable* ft, World* world, T* ptr)
    {
        RTFree(world, ptr);
    }
    
    template <class T> inline void memCopy(T *dst, const T *src, size_t n)
    {
        memcpy(dst, src, n * sizeof(T));
    }
    
    template <class T> inline void memMove(T *dst, const T *src, size_t n)
    {
        memmove(dst, src, n * sizeof(T));
    }
}; // namespace SKUG

#endif // SKUG_H_INCLUDED