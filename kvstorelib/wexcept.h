#pragma once
#include "utf8str.h"

class wexception : public std::exception
{
public:
    wexception(LPCWSTR message);
    wexception(const boost::wformat& message);
};

inline wexception::wexception(LPCWSTR message)
    : exception(static_cast<LPCSTR>(utf8str(message)))
{
}

inline wexception::wexception(const boost::wformat& message)
    : wexception(message.str().c_str())
{
}
