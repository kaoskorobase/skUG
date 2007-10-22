#ifndef VEP_BUFFER_H_INCLUDED
#define VEP_BUFFER_H_INCLUDED

#include "VEPFFT.h"
#include <vector>

namespace VEP
{
  template <class T> class Buffer
  {
  public:
    typedef std::vector<T*> Data;
    typedef typename Data::iterator iterator;
    typedef typename Data::const_iterator const_iterator;
  
  public:
    Buffer(size_t numChannels, size_t numFrames, bool clear=true)
      : m_numFrames(numFrames)
    {
      m_data.reserve(numChannels);
      for (size_t i=0; i < numChannels; ++i)
      {
        m_data.push_back(memAlloc<T>(numFrames));
        if (clear) memZero(m_data.back(), numFrames);
      }
    }
    ~Buffer()
    {
      for (iterator it = m_data.begin(); it != m_data.end(); ++it)
      {
        memFree<T>(*it);
      }
    }
  
    size_t numChannels() const { return m_data.size(); }
    size_t numFrames() const { return m_numFrames; }
  
    const T* operator[](size_t i) const { return m_data[i]; }
    T* operator[](size_t i) { return m_data[i]; }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }
  
    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
  
  private:
    size_t  m_numFrames;
    Data    m_data;
  };

  typedef Buffer<float> AudioBuffer;
};

#endif // VEP_BUFFER_H_INCLUDED