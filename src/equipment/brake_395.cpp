
#include "src/elements.h"
#include "brake_395.h"

#define BRAKE_STR_RATE 1.8
#define TR_CURRENT_C 272.0
#define BRAKE_MR_RATIO    0.005
#define BRAKE_PIPE_RATE_CHARGE 2.5
#define BRAKE_UR_RATE_CHARGE   0.3
#define BRAKE_PIPE_RATE 0.4
#define BRAKE_PIPE_EMERGENCY -1.2
#define PIPE_DISCHARGE_SLOW -0.005
#define UR_DISCHARGE2     0.003

Brake_395::Brake_395()
{}

int Brake_395::init()
{
    return 0;
}

void Brake_395::step(int pos, const Locomotive *loco, Engine *eng, float gameTime)
{
    m_checkBrake(pos, loco, eng, gameTime);
    m_soundBrake(pos, loco);
}

const st_Brake_395& Brake_395::constData()
{
    return m_data;
}

void Brake_395::m_checkBrake(int pos, const Locomotive *loco, Engine *eng, float gameTime)
{

    eng->MainResRate = 0.04*(11.0-loco->MainResPressure);

    //Train brake
    eng->TrainPipeRate = 0.0;
    if( !(loco->LocoFlags&1))
        return;

    //Printer_print(eng, GMM_POST, L"Braking: %d\n GameTime %f", self->pneumo.Arm_395, self->service.gameTime);
    switch(pos){
    case 0:
        if(eng->var[5]<45.0)
        {
            //if(!cab->Switches[3].SetState)
            eng->UR += BRAKE_UR_RATE_CHARGE * gameTime;
            if(eng->UR > loco->MainResPressure)
            {
                eng->UR=loco->MainResPressure;

            }
            if(loco->TrainPipePressure < eng->UR)
                eng->TrainPipeRate=(eng->UR-loco->TrainPipePressure)*1.5;
         }
        break;
    case 1:
        if(eng->var[5]<45.0)
        {
            if(eng->UR<5.0)
            {
                float rate=(loco->MainResPressure-eng->UR)*2.0;
                if(rate<0.0)rate=0.0;
                if(rate>BRAKE_UR_RATE_CHARGE)rate=BRAKE_UR_RATE_CHARGE;
                eng->UR+=rate * gameTime;
            }
            else
                if(loco->BrakeCylinderPressure>0.0&&(eng->UR-loco->TrainPipePressure)<0.1)
                    eng->UR+=0.15 * gameTime;
            if(eng->UR>loco->MainResPressure)
                eng->UR=loco->MainResPressure;
            if(eng->UR>loco->TrainPipePressure)
                eng->UR-=0.003 * gameTime;
            if(loco->TrainPipePressure<eng->UR-0.01)
                eng->TrainPipeRate=(eng->UR-loco->TrainPipePressure)/BRAKE_PIPE_RATE_CHARGE;
            else if(loco->TrainPipePressure>eng->UR)
            {
                eng->TrainPipeRate=(eng->UR-loco->TrainPipePressure)/BRAKE_PIPE_RATE_CHARGE;
                if(eng->TrainPipeRate<-BRAKE_PIPE_RATE)
                    eng->TrainPipeRate=-BRAKE_PIPE_RATE;
            }
        }
        break;
    case 2:
        if(eng->UR>loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(loco->TrainPipePressure>eng->UR)
            eng->TrainPipeRate=eng->UR-loco->TrainPipePressure;
        if(eng->TrainPipeRate<-BRAKE_PIPE_RATE)
            eng->TrainPipeRate=-BRAKE_PIPE_RATE;
        if(eng->TrainPipeRate>PIPE_DISCHARGE_SLOW)
            eng->TrainPipeRate=PIPE_DISCHARGE_SLOW;

        break;
    case 3:
        if(eng->UR>loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(loco->TrainPipePressure>eng->UR)
            eng->TrainPipeRate=eng->UR-loco->TrainPipePressure;
        else if(eng->UR-loco->TrainPipePressure>0.1)
            eng->TrainPipeRate=0.05;
        if(eng->TrainPipeRate<-BRAKE_PIPE_RATE)
            eng->TrainPipeRate=-BRAKE_PIPE_RATE;

        break;
    case 4:
        //if(cab->Switches[3].SetState!=4)
        // break;
        eng->UR-=0.3  * gameTime;
        if(eng->UR>loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(eng->UR<0)
            eng->UR=0;
        eng->TrainPipeRate=-0.25;


        break;
    case 5:
        eng->UR+=BRAKE_PIPE_EMERGENCY  *1.2 * gameTime;
        if(eng->UR > loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(eng->UR<0)
            eng->UR=0;
        eng->TrainPipeRate = BRAKE_PIPE_EMERGENCY;
        break;
    default:
        break;
    }

}

void Brake_395::m_soundBrake(int pos, const Locomotive *loco)
{
    if (m_prevPos != pos)
    {
        if (m_prevPos == 0)
            loco->PostTriggerCab(SoundsID::Kran395_Otpusk + 1);

        else if ( (m_prevPos == 4) || (m_prevPos == 5) )
           loco->PostTriggerCab(SoundsID::Kran395_Slugebnoe + 1);
        else if  (m_prevPos == 6)
           loco->PostTriggerCab(SoundsID::Kran395_Extrennoe + 1);

        if (pos == 0)
            loco->PostTriggerCab(SoundsID::Kran395_Otpusk);
        else if (pos == 1)
        {
            if (m_prevPos == 0)
                loco->PostTriggerCab(SoundsID::Kran395_Poezdnoe);
        }
        else if ( (pos == 4) || (pos == 5) )
            loco->PostTriggerCab(SoundsID::Kran395_Slugebnoe);
        else if  (pos == 6)
            loco->PostTriggerCab(SoundsID::Kran395_Extrennoe);
        m_prevPos = pos;
    }
}
