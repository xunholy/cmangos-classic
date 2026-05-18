#pragma once
#include "ModuleConfig.h"

namespace cmangos_module
{
    // Entry numbers picked to avoid collisions with existing Emberstone NPCs:
    //   190012/190013 -> twinkmaster (Alliance/Horde)
    //   190014        -> attunement (Attuner of Paths)
    // Upstream flekz uses 190013-190015; renumbered here when vendoring.
    #define TRAINING_DUMMY_NPC_ENTRY1 190021
    #define TRAINING_DUMMY_NPC_ENTRY2 190022
    #define TRAINING_DUMMY_NPC_ENTRY3 190023

    class TrainingDummiesModuleConfig : public ModuleConfig
    {
    public:
        TrainingDummiesModuleConfig();
        bool OnLoad() override;

    public:
        bool enabled;
    };
}