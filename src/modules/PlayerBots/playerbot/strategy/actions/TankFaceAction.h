#pragma once
#include "MovementActions.h"

namespace ai
{
    // Main-tank repositioning: when the raid stands in front of the boss, walk
    // to the far side so the boss turns its cleave/breath away from the group.
    // Generic across encounters — boss strategies with explicit anchors keep
    // priority over this via their higher relevance.
    class TankFaceAction : public MovementAction
    {
    public:
        TankFaceAction(PlayerbotAI* ai) : MovementAction(ai, "tank face") {}

        bool Execute(Event& event) override;
        bool isUseful() override;

        // Shared with TankFaceTrigger: true when the boss currently faces the
        // group because tank and raid stand on the same side.
        static bool NeedsReposition(PlayerbotAI* ai, Player* bot, float* outRaidX = nullptr, float* outRaidY = nullptr);
    };
}
