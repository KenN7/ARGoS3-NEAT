#ifndef EVOSTICK_CONTROLLER
#define EVOSTICK_CONTROLLER

#include <argos3/core/utility/math/range.h>
#include <argos3/core/utility/math/vector2.h>
#include <argos3/core/utility/math/rng.h>

#include <argos3/plugins/robots/e-puck/control_interface/ci_epuck_wheels_actuator.h>
#include <argos3/plugins/robots/e-puck/control_interface/ci_epuck_range_and_bearing_actuator.h>

#include <argos3/plugins/robots/e-puck/control_interface/ci_epuck_proximity_sensor.h>
#include <argos3/plugins/robots/e-puck/control_interface/ci_epuck_light_sensor.h>
#include <argos3/plugins/robots/e-puck/control_interface/ci_epuck_ground_sensor.h>
#include <argos3/plugins/robots/e-puck/control_interface/ci_epuck_range_and_bearing_sensor.h>

// Reference Model RM2
#include <argos3/demiurge/epuck-dao/ReferenceModel2Dot0.h>

#include "../NEATController.h"

#include <map>

using namespace argos;

class NeuralNetworkRM2Dot0 : public CEPuckNEATController {

public:

   NeuralNetworkRM2Dot0();
   virtual ~NeuralNetworkRM2Dot0();

   virtual void Init(TConfigurationNode& t_node);
   virtual void ControlStep();
   virtual void Reset();
   virtual void Destroy();

private:

   // Real m_inputs[15]; // 4x Proximity, 4x Light, 1x Ground, 5x Rab, 1x Bias
   double m_inputs[20]; //DO NOT put REAL of ARGOS on the robots


};

#endif
