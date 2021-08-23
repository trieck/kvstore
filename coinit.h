#pragma once

class CoInit
{
public:
    CoInit()
    {
        CoInitialize(nullptr);
    }

    ~CoInit()
    {
        CoUninitialize();
    }
};
