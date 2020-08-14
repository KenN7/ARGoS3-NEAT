#include "NeuralNetworkRM2Dot0.h"
#include <argos3/core/utility/logging/argos_log.h>

/****************************************/
/************* CONSTRUCTOR **************/
/****************************************/

NeuralNetworkRM2Dot0::NeuralNetworkRM2Dot0() {
    m_pcWheels = NULL;
    m_pcRABAct = NULL;
    m_pcProximity = NULL;
    m_pcLight = NULL;
    m_pcGround = NULL;
    m_pcRAB = NULL;
    m_net = NULL;
    m_nId = -1;
    m_unTimeStep = 0;
    m_mapMessages.clear();
    m_pcRNG = argos::CRandom::CreateRNG("argos");
}

/****************************************/
/************** DESTRUCTOR **************/
/****************************************/

NeuralNetworkRM2Dot0::~NeuralNetworkRM2Dot0() {
   delete m_pcRobotState;
}

/****************************************/
/**************** INIT ******************/
/****************************************/

void NeuralNetworkRM2Dot0::Init(TConfigurationNode& t_node) {
  CEPuckNEATController::Init(t_node);

  /* Reference model */
  m_pcRobotState = new ReferenceModel2Dot0();
  m_pcRobotState->SetRobotIdentifier(getRobotId());
  m_cWheelActuationRange.Set(-m_pcRobotState->GetMaxVelocity(), m_pcRobotState->GetMaxVelocity());

  m_cNeuralNetworkOutputRange.Set(0.0f, 1.0f);

  // Activate the RAB actuator (send the robot's id)
  if(m_pcRABAct != NULL) {
    UInt8 data[2];
    data[0] = getRobotId();
    data[1] = 0;
    m_pcRABAct->SetData(data);
  }
}

/****************************************/
/*************** CONTROL ****************/
/****************************************/

void NeuralNetworkRM2Dot0::ControlStep() {
   // Get Proximity sensory data.
   if(m_pcProximity != NULL) {
      const CCI_EPuckProximitySensor::TReadings& cProxiReadings = m_pcProximity->GetReadings();
      // Feed readings to EpuckDAO which will process them as needed
      m_pcRobotState->SetProximityInput(cProxiReadings);
      // Collecting processed readings
      CCI_EPuckProximitySensor::SReading cProcessedProxiReading = m_pcRobotState->GetProximityReading();
      CVector2 cProxiReading = CVector2(cProcessedProxiReading.Value, cProcessedProxiReading.Angle);
      // Injecting processed readings as input of the NN
      for(int i = 0; i < 4; i++) {
         CRadians cDirection = CRadians::PI*(2*(i) + 1)/4;
         Real value = cProxiReading.DotProduct(CVector2(1.0, cDirection));
         m_inputs[i] = (value > 0 ? value : 0); // only 2 inputs (rarely 3) will be different from 0.
      }
   } else {
      for(size_t i=0; i<4; i++) {
         m_inputs[i] = 0;
      }
   }

   // Get Light sensory data.
   if(m_pcLight != NULL) {
      const CCI_EPuckLightSensor::TReadings& cLightReadings = m_pcLight->GetReadings();
      m_pcRobotState->SetLightInput(cLightReadings);
      CCI_EPuckLightSensor::SReading cProcessedLightReading = m_pcRobotState->GetLightReading();
      CVector2 cLightReading = CVector2(cProcessedLightReading.Value, cProcessedLightReading.Angle);
      for(size_t i=4; i<8; i++) {
        CRadians cDirection = CRadians::PI*(2*(i-4) + 1)/4;
        Real value = cLightReading.DotProduct(CVector2(1.0, cDirection));
        m_inputs[i] = (value > 0 ? value : 0);
      }
   } else {
      for(size_t i=4; i<8; i++) {
         m_inputs[i] = 0;
      }
   }

   // Get Ground sensory data.
   if(m_pcGround != NULL) {
      const CCI_EPuckGroundSensor::SReadings& cGroundReadings = m_pcGround->GetReadings();
      m_pcRobotState->SetGroundInput(cGroundReadings);
      Real cProcessedGroundReading = m_pcRobotState->GetGroundReading();
      if(cProcessedGroundReading <= 0.1) { //black
        m_inputs[8] = 0;
      } else if(cProcessedGroundReading >= 0.95){ //white
        m_inputs[8] = 1;
      } else { //gray
        UInt32 index = m_pcRNG->Uniform(CRange<UInt32>(0, 4204));
        m_inputs[8] = m_GraySamplesLeft[index];
      }
   } else {
      m_inputs[8] = 0;
   }

  // Get RAB sensory data.
   if(m_pcRAB != NULL) {
      const CCI_EPuckRangeAndBearingSensor::TPackets& cRABReadings = m_pcRAB->GetPackets();
      m_pcRobotState->SetRangeAndBearingMessages(cRABReadings);

      CCI_EPuckRangeAndBearingSensor::SReceivedPacket cProcessedRabReading = m_pcRobotState->GetAttractionVectorToNeighbors(1.0);   // alpha = 1 (artbitrary value)
      CVector2 cRabReading = CVector2(cProcessedRabReading.Range, cProcessedRabReading.Bearing);
      UInt8 unNumberNeighbors = m_pcRobotState->GetNumberNeighbors();

      // Set the RAB input of the NN
      m_inputs[9] = 1 - (2 / (1 + exp(unNumberNeighbors))); // Saturate at 5, and is in [0,1]
      for(int i = 10; i < 14; i++) {
         CRadians cDirection = CRadians::PI*(2*(i-10) + 1)/4;
         Real value = cRabReading.DotProduct(CVector2(1.0, cDirection));
         m_inputs[i] = (value > 0 ? value : 0); // only 2 inputs (rarely 3) will be different from 0.
      }

      CCI_EPuckRangeAndBearingSensor::SReceivedPacket cProcessedRabReadingM = m_pcRobotState->GetAttractionVectorToMessagingNeighbors(1.0, 85);   // alpha = 1 (artbitrary value)
      CVector2 cRabReadingM = CVector2(cProcessedRabReadingM.Range, cProcessedRabReadingM.Bearing);
      UInt8 unNumberMessagingNeighbors = m_pcRobotState->GetNumberMessagingNeighbors(85);
      //fill input 24->29 (excluded) for messaging rab inputs
      m_inputs[14] = 1 - (2 / (1 + exp(unNumberMessagingNeighbors))); // Saturate at 5, and is in [0,1]
      for(int i = 15; i < 19; i++) {
         CRadians cDirection = CRadians::PI*(2*(i-20) + 1)/4;
         Real value = cRabReadingM.DotProduct(CVector2(1.0, cDirection));
         m_inputs[i] = ((value > 0) ? value : 0); // only 2 inputs (rarely 3) will be dif
      }

   } else {
      for(size_t i = 9; i<19; i++) {
         m_inputs[i] = 0;
      }
   }

   // Bias Unit
   m_inputs[19] = 1;

   // Feed the network with those inputs
   m_net->load_sensors((double*)m_inputs);
   // Transmit the signals to the next layer.
   m_net->activate();

   // Apply NN outputs to actuation. The NN outputs are in the range [0,1], we remap this range into [-5:5] linearly.
   m_cNeuralNetworkOutputRange.MapValueIntoRange(m_fLeftSpeed, (m_net->outputs[0])->activation, m_cWheelActuationRange);
   m_cNeuralNetworkOutputRange.MapValueIntoRange(m_fRightSpeed, (m_net->outputs[1])->activation, m_cWheelActuationRange);

   if(m_pcWheels != NULL) {
      m_pcWheels->SetLinearVelocity(m_fLeftSpeed, m_fRightSpeed);
   }

   if ((m_net->outputs[2])->activation < 0.5) {
       m_pcRobotState->SetRangeAndBearingMessageToSend(0);
   }
   else {
       m_pcRobotState->SetRangeAndBearingMessageToSend(85);
   }

   // updates RAB actuator (send the robot's id and eventual message)
   if(m_pcRABAct != NULL) {
      UInt8 data[2];
      data[0] = getRobotId();
      data[1] = m_pcRobotState->GetMessageToSend();
      m_pcRABAct->SetData(data);
   }

   m_unTimeStep++;
   //Display(1);
}

/****************************************/
/****************************************/

void NeuralNetworkRM2Dot0::Reset() {
  // Reinit the network: Puts the network back into an inactive state
  if (m_net != NULL) {
    m_net->flush();
  }
  m_unTimeStep = 0;
  m_pcRobotState->Reset();
  // Activate the RAB actuator (send the robot's id)
  if(m_pcRABAct != NULL) {
    UInt8 data[2];
    data[0] = getRobotId();
    data[1] = 0;
    m_pcRABAct->SetData(data);
  }
}

/****************************************/
/****************************************/

void NeuralNetworkRM2Dot0::Destroy() {}

REGISTER_CONTROLLER(NeuralNetworkRM2Dot0, "epuck_nn_controller")
