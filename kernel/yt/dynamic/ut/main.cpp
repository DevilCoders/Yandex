#include <library/cpp/testing/unittest/utmain.h>
#include <yt/yt/core/misc/shutdown.h>
#include <yt/yt/core/logging/log_manager.h>

int main(int argc, char* argv[])
{
    NYT::NLogging::TLogManager::Get()->ConfigureFromEnv();
    auto retval = NUnitTest::RunMain(argc, argv);
    NYT::Shutdown();
    return retval;
}
