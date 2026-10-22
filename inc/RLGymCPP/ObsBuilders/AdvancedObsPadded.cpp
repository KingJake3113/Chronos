#include "AdvancedObsPadded.h"

#include <algorithm>
#include <cmath>

#include "../GameStates/StateUtil.h"

namespace {
	RLGC::Action MirrorActionX(RLGC::Action action) {
		action.steer *= -1;
		action.yaw *= -1;
		action.roll *= -1;
		return action;
	}

	int GetMirrorBoostPadIndex(int index) {
		for (int i = 0; i < RLGC::CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
			const Vec loc = RLGC::CommonValues::BOOST_LOCATIONS[index];
			const Vec other = RLGC::CommonValues::BOOST_LOCATIONS[i];
			if (fabsf(loc.x + other.x) < 3.f && fabsf(loc.y - other.y) < 3.f) {
				return i;
			}
		}

		RG_ERR_CLOSE("AdvancedObsPadded: Failed to find x-mirrored boost pad for index " << index);
		return index;
	}
}

void RLGC::AdvancedObsPadded::AddPlayerToObs(FList& obs, const Player& player, bool inv, bool mirrorX, const PhysState& ball) {
	auto phys = MirrorPhysX(InvertPhys(player, inv), mirrorX);

	obs += phys.pos * POS_COEF;
	obs += phys.rotMat.forward;
	obs += phys.rotMat.up;
	obs += phys.vel * VEL_COEF;
	obs += phys.angVel * ANG_VEL_COEF;
	obs += phys.rotMat.Dot(phys.angVel) * ANG_VEL_COEF;

	obs += phys.rotMat.Dot(ball.pos - phys.pos) * POS_COEF;
	obs += phys.rotMat.Dot(ball.vel - phys.vel) * VEL_COEF;

	obs += player.boost / 100;
	obs += player.isOnGround;
	obs += player.HasFlipOrJump();
	obs += player.isDemoed;
	obs += player.hasJumped;
}

void RLGC::AdvancedObsPadded::AddBallPredictionToObs(FList& obs, const PhysState& ball) const {
	Vec predictedPos = ball.pos;
	Vec predictedVel = ball.vel;

	const float minX = -CommonValues::SIDE_WALL_X + CommonValues::BALL_RADIUS;
	const float maxX = CommonValues::SIDE_WALL_X - CommonValues::BALL_RADIUS;
	const float minY = -CommonValues::BACK_WALL_Y + CommonValues::BALL_RADIUS;
	const float maxY = CommonValues::BACK_WALL_Y - CommonValues::BALL_RADIUS;
	const float minZ = CommonValues::BALL_RADIUS;
	const float maxZ = CommonValues::CEILING_Z - CommonValues::BALL_RADIUS;

	for (int i = 0; i < predictionSteps; i++) {
		predictedVel.z += CommonValues::GRAVITY_Z * predictionStepSeconds;
		predictedPos += predictedVel * predictionStepSeconds;

		if (predictedPos.x < minX || predictedPos.x > maxX) {
			predictedPos.x = std::clamp(predictedPos.x, minX, maxX);
			predictedVel.x *= -0.6f;
		}

		if (predictedPos.y < minY || predictedPos.y > maxY) {
			predictedPos.y = std::clamp(predictedPos.y, minY, maxY);
			predictedVel.y *= -0.6f;
		}

		if (predictedPos.z < minZ || predictedPos.z > maxZ) {
			predictedPos.z = std::clamp(predictedPos.z, minZ, maxZ);
			predictedVel.z *= -0.6f;
		}

		obs += predictedPos * POS_COEF;
		obs += predictedVel * VEL_COEF;
	}
}

RLGC::FList RLGC::AdvancedObsPadded::BuildObs(const Player& player, const GameState& state) {
	FList obs = {};

	bool inv = player.team == Team::ORANGE;
	bool mirrorX = InvertPhys(player, inv).pos.x > 0;

	auto ball = MirrorPhysX(InvertPhys(state.ball, inv), mirrorX);
	auto& pads = state.GetBoostPads(inv);
	auto& padTimers = state.GetBoostPadTimers(inv);

	obs += ball.pos * POS_COEF;
	obs += ball.vel * VEL_COEF;
	obs += ball.angVel * ANG_VEL_COEF;
	if (enableBallPrediction)
		AddBallPredictionToObs(obs, ball);

	Action prevAction = mirrorX ? MirrorActionX(player.prevAction) : player.prevAction;
	for (int i = 0; i < prevAction.ELEM_AMOUNT; i++)
		obs += prevAction[i];

	for (int i = 0; i < CommonValues::BOOST_LOCATIONS_AMOUNT; i++) {
		int padIndex = mirrorX ? GetMirrorBoostPadIndex(i) : i;
		if (pads[padIndex]) {
			obs += 1.f;
		} else {
			obs += 1.f / (1.f + padTimers[padIndex]);
		}
	}

	FList selfObs = {};
	AddPlayerToObs(selfObs, player, inv, mirrorX, ball);
	obs += selfObs;
	int playerObsSize = selfObs.size();

	std::vector<FList> teammates = {}, opponents = {};

	for (auto& otherPlayer : state.players) {
		if (otherPlayer.carId == player.carId)
			continue;

		FList playerObs = {};
		AddPlayerToObs(playerObs, otherPlayer, inv, mirrorX, ball);
		((otherPlayer.team == player.team) ? teammates : opponents).push_back(playerObs);
	}

	if (teammates.size() > maxPlayers - 1)
		RG_ERR_CLOSE("AdvancedObsPadded: Too many teammates for Obs, maximum is " << (maxPlayers - 1));

	if (opponents.size() > maxPlayers)
		RG_ERR_CLOSE("AdvancedObsPadded: Too many opponents for Obs, maximum is " << maxPlayers);

	for (int i = 0; i < 2; i++) {
		auto& playerList = i ? teammates : opponents;
		int targetCount = i ? maxPlayers - 1 : maxPlayers;

		while (playerList.size() < targetCount) {
			FList pad = FList(playerObsSize);
			playerList.push_back(pad);
		}
	}

	std::shuffle(teammates.begin(), teammates.end(), ::Math::GetRandEngine());
	std::shuffle(opponents.begin(), opponents.end(), ::Math::GetRandEngine());

	for (auto& teammate : teammates)
		obs += teammate;
	for (auto& opponent : opponents)
		obs += opponent;

	return obs;
}
