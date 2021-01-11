/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#include "src/utils/auxil_funcs.h"
#include "brake_395.h"

#define BRAKE_STR_RATE 1.8
#define TR_CURRENT_C 272.0
#define BRAKE_MR_RATIO    0.005
#define BRAKE_PIPE_RATE_CHARGE 2.5
#define BRAKE_UR_RATE_CHARGE   0.3
#define BRAKE_PIPE_RATE 0.8 // 0.4
#define BRAKE_PIPE_EMERGENCY -1.2
#define PIPE_DISCHARGE_SLOW -0.005
#define UR_DISCHARGE2     0.003

/**
 * @brief The Brake_395_Sounds enum Номера звуков в sms файле.
 */
enum Brake_395_Sounds
{
    newPos = 193,
    Kran395_Otpusk = 500,
    Kran395_Poezdnoe = 502,
    Kran395_Slugebnoe = 504,
    Kran395_Extrennoe = 508,
};

Brake_395::Brake_395()
{
    m_data.m_currPos = -1;
    m_data.m_prevPos = -1;
}

int Brake_395::init(Engine *eng, int BrakeControlState )
{
    m_eng = eng;
    m_eng->BrakeSystemsEngaged = BrakeControlState;
    return 0;
}

void Brake_395::step(int pos, const Locomotive *loco, Engine *eng, float millisec, int BrakeControlState)
{
    m_eng = eng;
    m_loco = loco;
    m_data.m_currPos = pos;
    m_eng->BrakeSystemsEngaged = BrakeControlState;
    m_checkBrake( millisec);
    m_soundBrake();
}

const st_Brake_395& Brake_395::constData()
{
    return m_data;
}

/**
 * @brief Brake_395::m_checkBrake Алгоритм расчёта значений пневматики
 */
void Brake_395::m_checkBrake( float gameTime)
{

    m_eng->MainResRate = 0.04*(11.0 - m_loco->MainResPressure);

    //Train brake
    m_eng->TrainPipeRate = 0.0;
    if( !(m_loco->LocoFlags&1))
        return;

    if ( !BitState(m_eng->BrakeSystemsEngaged, 0) )
        return;

    switch(m_data.m_currPos ){
    case 0: // Зарядка-отпуск
        if(m_eng->var[5]<45.0)
        {
            //if(!cab->Switches[3].SetState)
            m_eng->UR += BRAKE_UR_RATE_CHARGE * gameTime;
            if(m_eng->UR > m_loco->MainResPressure)
            {
                m_eng->UR = m_loco->MainResPressure;

            }
            if(m_loco->TrainPipePressure < m_eng->UR)
               m_eng->TrainPipeRate=( m_eng->UR - m_loco->TrainPipePressure)*1.5;
         }
        break;
    case 1: // поездное
        if(m_eng->var[5]<45.0)
        {
            if(m_eng->UR<5.0)
            {
                float rate=(m_loco->MainResPressure - m_eng->UR)*2.0;
                if(rate<0.0)rate=0.0;
                if(rate>BRAKE_UR_RATE_CHARGE)rate=BRAKE_UR_RATE_CHARGE;
                m_eng -> UR+=rate * gameTime;
            }
            else
                if(m_loco->BrakeCylinderPressure>0.0&&(m_eng->UR - m_loco->TrainPipePressure)<0.1)
                    m_eng->UR += 0.15 * gameTime;
            if(m_eng->UR>m_loco->MainResPressure)
                m_eng->UR=m_loco->MainResPressure;
            if(m_eng->UR>m_loco->TrainPipePressure)
                m_eng->UR-=0.003 * gameTime;
            if(m_loco->TrainPipePressure<m_eng->UR-0.01)
                m_eng->TrainPipeRate=(m_eng->UR-m_loco->TrainPipePressure)/BRAKE_PIPE_RATE_CHARGE;
            else if(m_loco->TrainPipePressure>m_eng->UR)
            {
                m_eng->TrainPipeRate=(m_eng->UR-m_loco->TrainPipePressure)/BRAKE_PIPE_RATE_CHARGE;
                if(m_eng->TrainPipeRate<-BRAKE_PIPE_RATE)
                    m_eng->TrainPipeRate=-BRAKE_PIPE_RATE;
            }
        }
        break;
    case 2: // Перекрыша с питанием
        if(m_eng->UR>m_loco->MainResPressure)
            m_eng->UR=m_loco->MainResPressure;
        if(m_loco->TrainPipePressure>m_eng->UR)
            m_eng->TrainPipeRate=m_eng->UR-m_loco->TrainPipePressure;
        if(m_eng->TrainPipeRate<-BRAKE_PIPE_RATE)
            m_eng->TrainPipeRate=-BRAKE_PIPE_RATE;
        if(m_eng->TrainPipeRate>PIPE_DISCHARGE_SLOW)
            m_eng->TrainPipeRate=PIPE_DISCHARGE_SLOW;

        break;
    case 3: // перекрыша без питания
        if(m_eng->UR>m_loco->MainResPressure)
            m_eng->UR=m_loco->MainResPressure;
        if(m_loco->TrainPipePressure>m_eng->UR)
            m_eng->TrainPipeRate=m_eng->UR-m_loco->TrainPipePressure;
        else if(m_eng->UR-m_loco->TrainPipePressure>0.1)
            m_eng->TrainPipeRate = 0.05;
        if(m_eng->TrainPipeRate <- BRAKE_PIPE_RATE)
            m_eng->TrainPipeRate =- BRAKE_PIPE_RATE;

        break;
    case 4: // Торможение
    case 5: // Торможение ЭП
        //if(cab->Switches[3].SetState!=4)
        // break;
        m_eng->UR-=0.3  * gameTime;
        if(m_eng->UR>m_loco->MainResPressure)
            m_eng->UR=m_loco->MainResPressure;
        if(m_eng->UR<0)
            m_eng->UR=0;
        m_eng->TrainPipeRate=-0.25;
        break;
    case 6: //Экстренное
        m_eng->UR+=BRAKE_PIPE_EMERGENCY  *1.2 * gameTime;
        if(m_eng->UR > m_loco->MainResPressure)
            m_eng->UR=m_loco->MainResPressure;
        if(m_eng->UR<0)
            m_eng->UR=0;
        m_eng->TrainPipeRate = BRAKE_PIPE_EMERGENCY;
        break;
    default:
        break;
    }

}

/**
 * @brief Brake_395::m_soundBrake Просто озвучка крана
 */
void Brake_395::m_soundBrake()
{
    if (m_data.m_prevPos != m_data.m_currPos )
    {
        m_loco->PostTriggerCab(Brake_395_Sounds::newPos);
        if (m_data.m_prevPos == 0 )
            m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Otpusk + 1);

        else if ( (m_data.m_prevPos == 4) || (m_data.m_prevPos == 5) )
           m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Slugebnoe + 1);
        else if  (m_data.m_prevPos == 6)
           m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Extrennoe + 1);

        if ( (m_data.m_currPos  == 0) && (m_eng->TrainPipeRate > 0.0))
            m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Otpusk);
        else if ( (m_data.m_currPos  == 1) && (m_eng->TrainPipeRate > 0.0))
        {
            if (m_data.m_prevPos == 0)
                m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Poezdnoe);
        }
        else if ( (m_data.m_currPos  == 4) || (m_data.m_currPos  == 5) )
        {
            if( m_eng->TrainPipeRate < 0.0)
                m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Slugebnoe);
        }
        else if  (m_data.m_currPos  == 6)
        {   if( m_eng->TrainPipeRate < 0.0)
                m_loco->PostTriggerCab(Brake_395_Sounds::Kran395_Extrennoe);
        }
        m_data.m_prevPos = m_data.m_currPos ;
    }
}
