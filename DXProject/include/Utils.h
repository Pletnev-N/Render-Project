#pragma once

#include <iostream>

#ifndef NDEBUG
#define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

//if (FAILED(hr)) \
//{ \
//    std::wstring message = std::system_category().message(hr); \
//    message = L"file: " + __FILE__ + L"\nline: " + __LINE__ + L"\nhr: " + std::to_string(hr) + L"\nmessage: " + message; \
//    MessageBox(0, message.c_str(), 0, 0); \
//} \

#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x) \
    { \
        HRESULT hr = (x); \
        assert(SUCCEEDED(hr)); \
    }
#endif
#else
#ifndef HR
#define HR(x) (x)
#endif
#endif

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }


template<typename T>
static T Clamp(const T& x, const T& low, const T& high)
{
    return x < low ? low : (x > high ? high : x);
}