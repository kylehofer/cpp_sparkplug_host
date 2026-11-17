#include "Test.h"
#include "utilities/Logger.h"
#include <cstdio>
#include <cstring>

int main(int argc, char **argv)
{
    // Tests deliberately send bad messages; keep the output readable unless asked
    Logger::setLevel(argc > 1 && strcmp(argv[1], "-v") == 0 ? LogLevel::Debug : LogLevel::None);

    int failures = 0;
    for (const auto &testCase : test::registry())
    {
        try
        {
            testCase.body();
            printf("[ PASS ] %s\n", testCase.name);
        }
        catch (const std::exception &error)
        {
            failures++;
            printf("[ FAIL ] %s\n         %s\n", testCase.name, error.what());
        }
    }

    printf("\n%zu tests, %d failed\n", test::registry().size(), failures);
    return failures == 0 ? 0 : 1;
}
