#pragma once

/////////////////////////////////////////////////////////////////////////////
struct IValue
{
    virtual ~IValue() = default;

    virtual void copy(const uint8_t* data, uint32_t size) = 0;
    virtual uint8_t* data() const = 0;
    virtual uint32_t size() const = 0;
};
