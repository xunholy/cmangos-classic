
#include "playerbot/playerbot.h"
#include "Entities/Player.h"
#include "Globals/ObjectAccessor.h"
#include "Event.h"


using namespace ai;

ObjectGuid Event::getObject()
{
    if (packet.empty())
        return ObjectGuid();

    WorldPacket p(packet);
    p.rpos(0);
    
    ObjectGuid guid;
    p >> guid;

    return guid;
}

ObjectGuid Event::GuidOf(Player* owner)
{
    return owner ? owner->GetObjectGuid() : ObjectGuid();
}

Player* Event::getOwner()
{
    if (!ownerGuid)
        return nullptr;

    // inWorld=false: a player still finishing login is valid, but a player removed
    // from ObjectAccessor (RemoveObject runs before delete pl in Map::DeleteFromWorld)
    // resolves to nullptr instead of a dangling pointer.
    return ObjectAccessor::FindPlayer(ownerGuid, false);
}