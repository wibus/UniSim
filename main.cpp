#include "universe.h"

#include "PilsCore/test/tests.h"

void initPilsLogger()
{
    pils::Logger::Settings logSettings;
    logSettings.outputToConsole = true;
    logSettings.outputToFile = true;
    logSettings.fileName = "unisim_log.txt";
#ifdef PILS_TARGET_DEBUG
    logSettings.level = pils::Logger::Debug;
#else
    logSettings.level = pils::Logger::Info;
#endif

    pils::Logger::getInstance().initialize(logSettings);
}

int main(int argc, char ** argv)
{
    initPilsLogger();
    bool allTestsPassed = runPilsCoreTests();

    if (!allTestsPassed)
    {
        PILS_ERROR("Some PilsCore tests failed, aborting UniSim");
        return -1;
    }

    return unisim::Universe::getInstance().launch(argc, argv);
}
