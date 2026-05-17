#pragma once
#include "ModuleConfig.h"

namespace cmangos_module
{
    class TwinkmasterModuleConfig : public ModuleConfig
    {
    public:
        TwinkmasterModuleConfig();
        bool OnLoad() override;

        bool enabled;
        uint32 targetLevel;
    };
}
