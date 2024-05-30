#include "examplebot.h"

#include <ctime>
#include <math.h>
#include <string>

#include "rlbot/bot.h"
#include "rlbot/color.h"
#include "rlbot/interface.h"
#include "rlbot/rlbot_generated.h"
#include "rlbot/scopedrenderer.h"
#include "rlbot/statesetting.h"

#define PI 3.1415

ExampleBot::ExampleBot(int _index, int _team, std::string _name)
    : Bot(_index, _team, _name) {
  rlbot::GameState gamestate = rlbot::GameState();

  gamestate.ballState.physicsState.location = {0, 0, 1000};
  gamestate.ballState.physicsState.velocity = {0, 0, 5000};

  rlbot::CarState carstate = rlbot::CarState();
  carstate.physicsState.location = {0, 500, 1000};
  carstate.physicsState.velocity = {500, 1000, 1000};
  carstate.physicsState.angularVelocity = {1, 2, 3};

  carstate.boostAmount = 50;

  gamestate.carStates[_index] = carstate;

  rlbot::Interface::SetGameState(gamestate);
}

ExampleBot::~ExampleBot() {
  // Free your allocated memory here.
}

double findTargetAngle(float x, float y, float yaw) {
    double originToTargetangle = atan2(y, x);
    double frontToTargetAngle = originToTargetangle - yaw;
    
    if (frontToTargetAngle < -PI)
        frontToTargetAngle += 2 * PI;
    if (frontToTargetAngle > PI)
        frontToTargetAngle -= 2 * PI;

    return frontToTargetAngle;
}

void print_pad_locs(std::vector<const rlbot::flat::BoostPad*>& pads, rlbot::ScopedRenderer& renderer) {
    for (uint32_t i = 0; i < pads.size(); ++i) {
        std::string x = std::to_string(static_cast<int>(pads[i]->location()->x()));
        std::string y = std::to_string(static_cast<int>(pads[i]->location()->y()));
        std::string z = std::to_string(static_cast<int>(pads[i]->location()->z()));
        std::string active = pads[i]->isFullBoost() ? "true" : "false";
        renderer.DrawString2D(x + " " + y + " " + z + " " + active, rlbot::Color::red,
            rlbot::flat::Vector3{10, static_cast<float>(i) * 20, 0}, 1, 1);
    }
}

rlbot::Controller ExampleBot::GetOutput(rlbot::GameTickPacket gametickpacket) {

  rlbot::flat::Vector3 ballLocation =
      *gametickpacket->ball()->physics()->location();
  rlbot::flat::Vector3 ballVelocity =
      *gametickpacket->ball()->physics()->velocity();
  rlbot::flat::Vector3 carLocation =
      *gametickpacket->players()->Get(index)->physics()->location();
  rlbot::flat::Rotator carRotation =
      *gametickpacket->players()->Get(index)->physics()->rotation();

  // Calculate the velocity of the ball.
  float velocity = sqrt(ballVelocity.x() * ballVelocity.x() +
                        ballVelocity.y() * ballVelocity.y() +
                        ballVelocity.z() * ballVelocity.z());

  // This renderer will build and send the packet once it goes out of scope.
  rlbot::ScopedRenderer renderer("test");

  // Load the ballprediction into a vector to use for rendering.
  std::vector<const rlbot::flat::Vector3 *> points;

  rlbot::BallPrediction ballprediction = GetBallPrediction();
  rlbot::FieldInfo fieldinfo = GetFieldInfo();

  // find nearest full big boost pad
  const rlbot::flat::Vector3 *closest_full_big_pad = nullptr;
  std::vector<const rlbot::flat::BoostPad*> pad_locs;

  for (uint32_t i = 0; i < fieldinfo->boostPads()->size(); ++i) {
      float min_dist = 99999;
      const rlbot::flat::BoostPad* cur = fieldinfo->boostPads()->Get(i);
      bool is_active = gametickpacket->boostPadStates()->Get(i)->isActive();
      //pad_locs.push_back(cur);
      float dist = sqrt(cur->location()->x() * cur->location()->x() +
                      cur->location()->y() * cur->location()->y());

      if (cur->isFullBoost() && is_active && (dist < min_dist)) {
          closest_full_big_pad = cur->location();
      }
  }

  //print_pad_locs(pad_locs, renderer);

  for (uint32_t i = 0; i < ballprediction->slices()->size(); i++) {
    points.push_back(ballprediction->slices()->Get(i)->physics()->location());
  }

  renderer.DrawPolyLine3D(rlbot::Color::red, points);

  /*renderer.DrawString3D(std::to_string(velocity), rlbot::Color::magenta,
                        ballLocation, 2, 2);*/

  // Calculate to get the angle from the front of the bot's car to the ball.
  int32_t cur_boost = gametickpacket->players()->Get(index)->boost();
  double targetAngle = 0.0;

  if (cur_boost < 20 && closest_full_big_pad != nullptr) {
      targetAngle = findTargetAngle(closest_full_big_pad->x() - carLocation.x(),
                                    closest_full_big_pad->y() - carLocation.y(),
                                    carRotation.yaw());
      renderer.DrawString3D("HERE", rlbot::Color::red, *closest_full_big_pad, 2, 2);
  }
  else {
      targetAngle = findTargetAngle(ballLocation.x() - carLocation.x(),
                                    ballLocation.y() - carLocation.y(),
                                    carRotation.yaw());
      renderer.DrawString3D("HERE", rlbot::Color::red, ballLocation, 2, 2);
  }

  /*renderer.DrawString3D(std::to_string(targetAngle * (180 / PI)), rlbot::Color::green,
      carLocation, 2, 2);*/

  rlbot::Controller controller{0};

  // Decide which way to steer in order to get to the ball.
  if (targetAngle > 0)
    controller.steer = 1;
  else
    controller.steer = -1;

  controller.throttle = 1.0f;

  controller.boost = true;

  return controller;
}
