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
#include <argos3/demiurge/epuck-dao/ReferenceModel2Dot2.h>

#include "../NEATController.h"

#include <map>

// gianduja version of evocom2 with 3 bits (and original rm 2.2)

using namespace argos;

class NeuralNetworkRM2Dot3X : public CEPuckNEATController {

public:

   NeuralNetworkRM2Dot3X();
   virtual ~NeuralNetworkRM2Dot3X();

   virtual void Init(TConfigurationNode& t_node);
   virtual void ControlStep();
   virtual void Reset();
   virtual void Destroy();

private:

   Real m_inputs[28];

};

#endif
