#include "GiandujaDecisionLoopFunc.h"

/****************************************/
/****************************************/

GiandujaDecisionLoopFunction::GiandujaDecisionLoopFunction() {
  m_fRadius = 0.3;
  m_cCoordSpot1 = CVector2(0,0);

  m_CCoordRect1Pos = CVector2(0.8,1);
  m_CCoordRect1 = CVector2(1.25,-0.6);

  m_CCoordRect2Pos = CVector2(-0.8,-1);
  m_CCoordRect2 = CVector2(-1.25,0.6);


  m_unCostI = 0;
  m_unCostO = 0;
  m_unState = 0;
  m_unTbar = 0;
  m_unDecision = 0;
  m_fObjectiveFunction = 0;

}

/****************************************/
/****************************************/

GiandujaDecisionLoopFunction::GiandujaDecisionLoopFunction(const GiandujaDecisionLoopFunction& orig) {}

/****************************************/
/****************************************/

GiandujaDecisionLoopFunction::~GiandujaDecisionLoopFunction() {}

/****************************************/
/****************************************/

void GiandujaDecisionLoopFunction::Destroy() {}

/****************************************/
/****************************************/

void GiandujaDecisionLoopFunction::Reset() {
    CNeatLoopFunctions::Reset();
    m_unCostI = 0;
    m_unCostO = 0;
    m_unState = 0;
    m_unTbar = 0;
    m_fObjectiveFunction = 0;
    //Real a = m_pcRng->Uniform(CRange<Real>(0.0f, 1.0f));
    //Real b = m_pcRng->Uniform(CRange<Real>(0.0f, 1.0f));
    m_unDecision = m_pcRng->Uniform(CRange<UInt32>(0,2));
}

/****************************************/
/****************************************/

void GiandujaDecisionLoopFunction::Init(TConfigurationNode& t_tree) {
    CNeatLoopFunctions::Init(t_tree);
    //Real a = m_pcRng->Uniform(CRange<Real>(0.0f, 1.0f));
    //Real b = m_pcRng->Uniform(CRange<Real>(0.0f, 1.0f));
    m_unDecision = m_pcRng->Uniform(CRange<UInt32>(0,2));

}

argos::CColor GiandujaDecisionLoopFunction::GetFloorColor(const argos::CVector2& c_position_on_plane) {
  CVector2 vCurrentPoint(c_position_on_plane.GetX(), c_position_on_plane.GetY());

  Real d = (m_cCoordSpot1 - vCurrentPoint).Length();

  if (d <= m_fRadius) {
    if (m_unDecision == 0) {
        return CColor::BLACK;
    }
    else if (m_unDecision == 1) {
        return CColor::WHITE;
    }
  }
  // if (d <= m_fRadius+0.01) {
  //   return CColor::GREEN;
  // }
  //
  // if ( (vCurrentPoint.GetX()<=m_CCoordRect1Pos.GetX()) && (vCurrentPoint.GetX()>=m_CCoordRect2Pos.GetX()) && (vCurrentPoint.GetY()>=m_CCoordRect2Pos.GetY()) && (vCurrentPoint.GetY()<=m_CCoordRect1Pos.GetY()) ) {
  //   return CColor::GREEN;
  // }

  // if ( vCurrentPoint.GetY()<=m_CCoordRect1.GetY() || vCurrentPoint.GetY()>=m_CCoordRect2.GetY()) {
  //   return CColor::WHITE;
  // }

  return CColor::GRAY50;
}


CVector3 GiandujaDecisionLoopFunction::GetRandomPosition() {
    Real a;
    Real b;

    a = m_pcRng->Uniform(CRange<Real>(0.0f, 1.0f));
    b = m_pcRng->Uniform(CRange<Real>(0.0f, 1.0f));

    Real fPosX = m_CCoordRect2Pos.GetX() + a*fabs(m_CCoordRect2Pos.GetX() - m_CCoordRect1Pos.GetX());
    Real fPosY = m_CCoordRect2Pos.GetY() + b*fabs(m_CCoordRect2Pos.GetY() - m_CCoordRect1Pos.GetY());

    return CVector3(fPosX, fPosY, 0);
}

/****************************************/
/****************************************/

void GiandujaDecisionLoopFunction::PostStep() {
    CSpace::TMapPerType& tEpuckMap = GetSpace().GetEntitiesByType("epuck");
    CVector2 cEpuckPosition(0,0);
    for (CSpace::TMapPerType::iterator it = tEpuckMap.begin(); it != tEpuckMap.end(); ++it) {
        CEPuckEntity* pcEpuck = any_cast<CEPuckEntity*>(it->second);
        cEpuckPosition.Set(pcEpuck->GetEmbodiedEntity().GetOriginAnchor().Position.GetX(),
                         pcEpuck->GetEmbodiedEntity().GetOriginAnchor().Position.GetY());

        //Real fDistanceSpot1 = (m_cCoordSpot1 - cEpuckPosition).Length();

        if (m_unDecision == 0) {
            if ( cEpuckPosition.GetY()<=m_CCoordRect2.GetY() ) {
                m_unCostI += 1;
            }

        }
        else if (m_unDecision == 1) {
            if ( cEpuckPosition.GetY()>=m_CCoordRect1.GetY() ) {
                m_unCostI += 1;
            }

        }

    }

}

/****************************************/
/****************************************/

void GiandujaDecisionLoopFunction::PostExperiment() {
    LOG<< "CostI :" << m_unCostI << std::endl;
    LOG<< (24000-m_unCostI) << std::endl;
    m_fObjectiveFunction = (Real) m_unCostI;

}

Real GiandujaDecisionLoopFunction::GetObjectiveFunction() {
  return (24000-m_fObjectiveFunction);
}

REGISTER_LOOP_FUNCTIONS(GiandujaDecisionLoopFunction, "gianduja_decision_loop_functions");
