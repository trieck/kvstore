#pragma once

class utf8str
{
public:
    explicit utf8str(const std::wstring& input);
    explicit utf8str(LPCWSTR input);

    explicit operator LPCSTR() const;
    explicit operator const std::string&() const;

    const std::string& str() const;

private:
    std::string m_str;
};
