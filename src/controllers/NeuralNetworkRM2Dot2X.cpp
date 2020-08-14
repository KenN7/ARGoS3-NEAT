#include "NeuralNetworkRM2Dot2X.h"
#include <argos3/core/utility/logging/argos_log.h>

/****************************************/
/************* CONSTRUCTOR **************/
/****************************************/

NeuralNetworkRM2Dot2X::NeuralNetworkRM2Dot2X() {
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

NeuralNetworkRM2Dot2X::~NeuralNetworkRM2Dot2X() {
   delete m_pcRobotState;
}

/****************************************/
/**************** INIT ******************/
/****************************************/

void NeuralNetworkRM2Dot2X::Init(TConfigurationNode& t_node) {
  CEPuckNEATController::Init(t_node);

  /* Reference model */
  m_pcRobotState = new ReferenceModel2Dot2();
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

void NeuralNetworkRM2Dot2X::ControlStep() {
   // Get Proximity sensory data.
   if(m_pcProximity != NULL) {
      const CCI_EPuckProximitySensor::TReadings& cProxiReadings = m_pcProximity->GetReadings();
      // Feed readings to EpuckDAO which will process them as needed
      m_pcRobotState->SetProximityInput(cProxiReadings);
      // Collecting processed readings
      CCI_EPuckProximitySensor::TReadings cProcessedProxiReadings = m_pcRobotState->GetProximityInput();
      // Injecting processed readings as input of the NN
      for(size_t i=0; i<8; i++) {
         m_inputs[i] = cProcessedProxiReadings[i].Value;
      }
   } else {
      for(size_t i=0; i<8; i++) {
         m_inputs[i] = 0;
      }
   }

   // Get Light sensory data.
   if(m_pcLight != NULL) {
      const CCI_EPuckLightSensor::TReadings& cLightReadings = m_pcLight->GetReadings();
      m_pcRobotState->SetLightInput(cLightReadings);
      CCI_EPuckLightSensor::TReadings cProcessedLightReadings = m_pcRobotState->GetLightInput();
      for(size_t i=8; i<16; i++) {
         m_inputs[i] = cProcessedLightReadings[i-8].Value;
      }
   } else {
      for(size_t i=8; i<16; i++) {
         m_inputs[i] = 0;
      }
   }

   // Get Ground sensory data.
   if(m_pcGround != NULL) {
      const CCI_EPuckGroundSensor::SReadings& cGroundReadings = m_pcGround->GetReadings();
      m_pcRobotState->SetGroundInput(cGroundReadings);
      CCI_EPuckGroundSensor::SReadings cProcessedGroundReadings = m_pcRobotState->GetGroundInput();
      for(size_t i=16; i<19; i++) {
         m_inputs[i] = cProcessedGroundReadings[i-16];
         if(cProcessedGroundReadings[i-16] <= 0.1) { //black
            m_inputs[i] = 0;
         } else if(cProcessedGroundReadings[i-16] >= 0.95){ //white
            m_inputs[i] = 1;
         } else { //gray
            UInt32 index = m_pcRNG->Uniform(CRange<UInt32>(0, 4204));
            if(i == 16)
               m_inputs[i] = m_GraySamplesLeft[index];
            else if(i == 17)
               m_inputs[i] = m_GraySamplesCenter[index];
            else
            m_inputs[i] = m_GraySamplesRight[index];
         }
      }
   } else {
      for(size_t i=16; i<19; i++) {
         m_inputs[i] = 0;
      }
   }

   // Get RAB sensory data.
   if(m_pcRAB != NULL) {
      const CCI_EPuckRangeAndBearingSensor::TPackets& cRABReadings = m_pcRAB->GetPackets();
      m_pcRobotState->SetRangeAndBearingMessages(cRABReadings);

      CCI_EPuckRangeAndBearingSensor::SReceivedPacket cProcessedRabReading = m_pcRobotState->GetAttractionVectorToNeighbors(1.0);   // alpha = 1 (artbitrary value)
      CVector2 cRabReading = CVector2(cProcessedRabReading.Range, cProcessedRabReading.Bearing);

      Real len = cRabReading.Length();
      if(len != 0) {
         cRabReading.Normalize(); // now, sRabVectorSum.Length = 1
         cRabReading *= (2/(1+exp(-len)) - 1);
      }
      UInt8 unNumberNeighbors = m_pcRobotState->GetNumberNeighbors();

      // Set the RAB input of the NN
      m_inputs[19] = 1 - (2 / (1 + exp(unNumberNeighbors))); // Saturate at 5, and is in [0,1]
      for(int i = 20; i < 24; i++) {
         CRadians cDirection = CRadians::PI*(2*(i-20) + 1)/4;
         Real value = cRabReading.DotProduct(CVector2(1.0, cDirection));
         m_inputs[i] = (value > 0 ? value : 0); // only 2 inputs (rarely 3) will be different from 0.
      }

    // CCI_EPuckRangeAndBearingSensor::SReceivedPacket cProcessedRabReadingM = m_pcRobotState->GetAttractionVectorToMessagingNeighbors(1.0, 10);   // alpha = 1 (artbitrary value)
    // CVector2 cRabReadingM = CVector2(cProcessedRabReadingM.Range, cProcessedRabReadingM.Bearing);
    // len = cRabReadingM.Length();
    // if(len != 0) {
    //    cRabReadingM.Normalize(); // now, sRabVectorSum.Length = 1
    //    cRabReadingM *= (2/(1+exp(-len)) - 1);
    // }

    UInt8 unNumberMessagingNeighbors = m_pcRobotState->GetNumberMessagingNeighbors(10);
    //fill input 24->29 (excluded) for messaging rab inputs
    m_inputs[24] = 1 - (2 / (1 + exp(unNumberMessagingNeighbors))); // Saturate at 5, and is in [0,1]
    // for(int i = 25; i < 29; i++) {
    //     CRadians cDirection = CRadians::PI*(2*(i-25) + 1)/4;
    //     Real value = cRabReadingM.DotProduct(CVector2(1.0, cDirection));
    //     m_inputs[i] = ((value > 0) ? value : 0); // only 2 inputs (rarely 3) will be dif
    // }

    // CCI_EPuckRangeAndBearingSensor::SReceivedPacket cProcessedRabReadingM2 = m_pcRobotState->GetAttractionVectorToMessagingNeighbors(1.0, 160);   // alpha = 1 (artbitrary value)
    // CVector2 cRabReadingM2 = CVector2(cProcessedRabReadingM2.Range, cProcessedRabReadingM2.Bearing);
    // len = cRabReadingM2.Length();
    // if(len != 0) {
    //    cRabReadingM2.Normalize(); // now, sRabVectorSum.Length = 1
    //    cRabReadingM2 *= (2/(1+exp(-len)) - 1);
    // }

    UInt8 unNumberMessagingNeighbors2 = m_pcRobotState->GetNumberMessagingNeighbors(160);
    //fill input 24->29 (excluded) for messaging rab inputs
    m_inputs[25] = 1 - (2 / (1 + exp(unNumberMessagingNeighbors2))); // Saturate at 5, and is in [0,1]
    // for(int i = 30; i < 34; i++) {
    //   CRadians cDirection = CRadians::PI*(2*(i-30) + 1)/4;
    //   Real value = cRabReadingM2.DotProduct(CVector2(1.0, cDirection));
    //   m_inputs[i] = ((value > 0) ? value : 0); // only 2 inputs (rarely 3) will be dif
    // }

    } else {
      for(size_t i = 19; i<26; i++) {
         m_inputs[i] = 0;
      }
    }

   // Bias Unit
   m_inputs[26] = 1;

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
       m_pcRobotState->SetRangeAndBearingMessageToSend(0 | (m_pcRobotState->GetMessageToSend() & 0xF0 ));
   }
   else {
       m_pcRobotState->SetRangeAndBearingMessageToSend(10 | (m_pcRobotState->GetMessageToSend() & 0xF0 ));
   }

   if ((m_net->outputs[3])->activation < 0.5) {
       m_pcRobotState->SetRangeAndBearingMessageToSend(0 | (m_pcRobotState->GetMessageToSend() & 0x0F ));
   }
   else {
       m_pcRobotState->SetRangeAndBearingMessageToSend(160 | (m_pcRobotState->GetMessageToSend() & 0x0F ));
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

void NeuralNetworkRM2Dot2X::Reset() {
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

void NeuralNetworkRM2Dot2X::Destroy() {}

REGISTER_CONTROLLER(NeuralNetworkRM2Dot2X, "epuck_nn_controller")
