#pragma once

struct CoInit
{
    CoInit()
    {
        CoInitialize(nullptr);
    }

    ~CoInit()
    {
        CoUninitialize();
    }
};
