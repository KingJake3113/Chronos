#pragma once

#include "AdvancedObs.h"

namespace RLGC {
	// Padded AdvancedObs with a fixed-size lightweight ball trajectory sample.
	class AdvancedObsPadded : public AdvancedObs {
	public:
		int maxPlayers;
		bool enableBallPrediction;
		int predictionSteps;
		float predictionStepSeconds;

		AdvancedObsPadded(
			int maxPlayers,
			bool enableBallPrediction = false,
			int predictionSteps = 6,
			float predictionStepSeconds = 0.25f
		) : maxPlayers(maxPlayers), enableBallPrediction(enableBallPrediction), predictionSteps(predictionSteps), predictionStepSeconds(predictionStepSeconds) {

		}

		virtual void AddPlayerToObs(FList& obs, const Player& player, bool inv, bool mirrorX, const PhysState& ball);
		virtual void AddBallPredictionToObs(FList& obs, const PhysState& ball) const;
		virtual FList BuildObs(const Player& player, const GameState& state) override;
	};
}
