#include "kvstorelib.h"
#include "util.h"

namespace utility {

std::string comma(uint64_t i)
{
    std::ostringstream os;

    std::ostringstream ss;
    ss << i;
    auto input = ss.str();

    auto n = static_cast<int>(input.length());

    for (auto j = n - 1, k = 1; j >= 0; j--, k++) {
        os << input[j];
        if (k % 3 == 0 && j > 0 && j < n - 1) {
            os << ',';
        }
    }

    auto output(os.str());
    reverse(output.begin(), output.end());

    return output;
}

} // utility
