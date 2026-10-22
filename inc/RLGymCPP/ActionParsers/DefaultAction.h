#pragma once
#include "RLGymCPP/ActionParsers/ActionParser.h"

namespace RLGC {

	// Actions match DiscreteAction in RLGymPPO_CPP and lookup_act.py in Python RLGym, but also has action masking
	class DefaultAction : public ActionParser {
	public:

		std::vector<Action> actions;
		std::vector<uint8_t> groundMask, airMask, jumpMask, boostMask;
		bool mirrorActionsX;

		DefaultAction(bool mirrorActionsX = false);

		// Returns a permutation where result[i] = the action index whose raw controls
		// are the X-mirror (steer/yaw/roll *= -1) of actions[i].
		static std::vector<int> GetMirrorActionMap();

		virtual Action ParseAction(int index, const Player& player, const GameState& state) override;

		virtual int GetActionAmount() override {
			return actions.size();
		}

		virtual std::vector<uint8_t> GetActionMask(const Player& player, const GameState& state) override;

	private:
		bool ShouldMirrorPlayerX(const Player& player) const;
	};
}
