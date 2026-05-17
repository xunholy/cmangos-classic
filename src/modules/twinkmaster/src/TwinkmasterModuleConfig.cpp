#include "TwinkmasterModuleConfig.h"

namespace cmangos_module
{
    TwinkmasterModuleConfig::TwinkmasterModuleConfig()
    : ModuleConfig("twinkmaster.conf")
    , enabled(false)
    , targetLevel(19)
    {
    }

    bool TwinkmasterModuleConfig::OnLoad()
    {
        enabled = config.GetBoolDefault("Twinkmaster.Enable", false);
        targetLevel = config.GetIntDefault("Twinkmaster.TargetLevel", 19);
        return true;
    }
}
