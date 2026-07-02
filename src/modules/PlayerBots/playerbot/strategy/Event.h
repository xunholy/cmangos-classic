#pragma once

#include "Entities/ObjectGuid.h"
#include "Server/WorldPacket.h"

class Player;

namespace ai
{
    class Event
	{
	public:
        Event(Event const& other)
        {
            source = other.source;
            param = other.param;
            packet = other.packet;
            ownerGuid = other.ownerGuid;
        }
        Event() {}
        Event(std::string source) : source(source) {}
        Event(std::string source, std::string param, Player* owner = NULL) : source(source), param(param), ownerGuid(GuidOf(owner)) {}
        Event(std::string source, WorldPacket &packet, Player* owner = NULL) : source(source), packet(packet), ownerGuid(GuidOf(owner)) {}
        Event(std::string source, ObjectGuid object, Player* owner = NULL) : source(source), ownerGuid(GuidOf(owner)) { packet << object; }
        virtual ~Event() {}

	public:
        std::string getSource() const { return source; }
        std::string getParam() { return param; }
        WorldPacket& getPacket() { return packet; }
        ObjectGuid getObject();
        // Re-resolve the owner from its guid at every use. Never store/return a raw
        // Player* that could outlive the player (UAF via ReactionEngine-retained events).
        // Returns nullptr once the owner has logged out.
        Player* getOwner();
        ObjectGuid getOwnerGuid() const { return ownerGuid; }
        bool operator! () const { return source.empty(); }

    protected:
        // Capture a player's guid without needing the full Player definition here.
        static ObjectGuid GuidOf(Player* owner);

        std::string source;
        std::string param;
        WorldPacket packet;
        ObjectGuid ownerGuid;
	};
}
