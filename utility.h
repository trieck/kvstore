#pragma once

std::wstring GuidToString(REFGUID guid);
HRESULT StringToGUID(LPCOLESTR str, GUID& guid);