#include "../Source/StillAwake/Core/SARuleChecks.h"
#include <iostream>
int main()
{
    const auto R = SA::RunChecks();
    for (const auto &Error : R.Errors)
        std::cerr << "FAIL: " << Error << '\n';
    std::cout << R.Count << " checks, " << R.Errors.size() << " failures.\n";
    return R.Errors.empty() ? 0 : 1;
}
