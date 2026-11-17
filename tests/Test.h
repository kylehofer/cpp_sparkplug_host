/*
 * Minimal test harness for cpp_sparkplug_host. No external dependencies.
 *
 * TEST(name) { CHECK(condition); CHECK_EQ(a, b); }
 */

#ifndef TESTS_TEST
#define TESTS_TEST

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test
{
    struct Case
    {
        const char *name;
        std::function<void()> body;
    };

    inline std::vector<Case> &registry()
    {
        static std::vector<Case> cases;
        return cases;
    }

    struct Registrar
    {
        Registrar(const char *name, std::function<void()> body)
        {
            registry().push_back({name, std::move(body)});
        }
    };

    struct Failure : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    inline std::string location(const char *file, int line)
    {
        return std::string(file) + ":" + std::to_string(line);
    }
}

#define TEST(name)                                                   \
    static void test_##name();                                       \
    static test::Registrar registrar_##name(#name, test_##name);     \
    static void test_##name()

#define CHECK(condition)                                                                  \
    do                                                                                    \
    {                                                                                     \
        if (!(condition))                                                                 \
        {                                                                                 \
            throw test::Failure(test::location(__FILE__, __LINE__) + ": CHECK(" #condition ")"); \
        }                                                                                 \
    } while (0)

#define CHECK_EQ(actual, expected)                                                         \
    do                                                                                     \
    {                                                                                      \
        auto actualValue = (actual);                                                       \
        auto expectedValue = (expected);                                                   \
        if (!(actualValue == expectedValue))                                               \
        {                                                                                  \
            std::ostringstream message;                                                    \
            message << test::location(__FILE__, __LINE__) << ": CHECK_EQ(" #actual ", " #expected ") got " \
                    << actualValue << ", expected " << expectedValue;                      \
            throw test::Failure(message.str());                                            \
        }                                                                                  \
    } while (0)

#endif /* TESTS_TEST */
