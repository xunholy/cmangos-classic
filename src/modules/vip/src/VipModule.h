#pragma once
#include "Module.h"
#include "VipModuleConfig.h"

#include <string>
#include <vector>

class WorldSession;

namespace cmangos_module
{
    class VipModule : public Module
    {
    public:
        VipModule();

        const VipModuleConfig* GetConfig() const;
        bool IsEnabled() const;

        // GM chat commands: .vip <subcommand>
        const char* GetChatCommandPrefix() const override { return "vip"; }
        std::vector<ModuleChatCommand>* GetCommandTable() override;

    private:
        bool HandleGrantCommand(WorldSession* session, const std::string& args);
        bool HandleRevokeCommand(WorldSession* session, const std::string& args);
    };
}
