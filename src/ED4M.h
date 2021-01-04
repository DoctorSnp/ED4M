#ifndef ED4M_LOGIC_H
#define ED4M_LOGIC_H

#include <sys/timeb.h>
#include "radiostation.h"
#include "saut_datatype.h"
#include "src/private_ED4M.h"
#include "src/ED4M_datatypes/cab/section1/elements.h"


#define NEUTRAL_CONTROLLER_POS 5
#define REVERSE_NEUTRAL 2
#define SECTION_DEST_FORWARD 1
#define SECTION_DEST_BACKWARD -1

#define TUMBLERS_MAX_ID 790
PACKED_BEGIN

/*разные флаги локомотива*/
typedef struct st_Flags
{
    int EPK_Triggered;
}st_Flags;


typedef struct st_Pneumo
{
  int Arm_254;
  int Arm_395;
  int Blok_367;

}Pneumo;

typedef struct st_Electric
{
    unsigned char PantoRaised;
    float power;
    float LineVoltage;
}Electric;

typedef struct st_Tumblers
{
    int projHalf;
    int projFull;
    int AvarEPT;
}st_Tumblers;

typedef struct st_timeForDebug
{
    struct timeb prevTime;
    struct timeb currTime;
}st_timeForDebug;

struct st_ServiceInfo
{
    float currTime;
    float prevTIme;
    int LocoState;
    float AirTemperature;
};

/**
 * @brief The st_Self struct Структура с собственными параметрами для работы между функциями библиотеки.
 */
struct st_Self
{
  st_Flags flags;
  int isBackward;
  int brake395_pos;
  int secdionCabDest;
  int dest; // 1 ,0,  -1
  st_Tumblers tumblers; // Это потом удалить
  int tumblersArray[TUMBLERS_MAX_ID];
  Electric elecrto;
  Pneumo pneumo;
  float prevVelocity;
  bool SL2M_Ticked;
  int BV_STATE;
  int EPK;
  int RB;
  int MK;
  int MV_low;
  st_Radiostation radio;
  int Reverse;
  unsigned int TyagaPosition;
  unsigned int RecuperationPosition;
  int shuntNum;
  st_ALSN alsn;
  st_timeForDebug debugTime;
  st_ServiceInfo service;
  st_KLUB KLUB;

 // float IndependentBrakeValue;
 // float BrakeForce;
 // float Force ;
 // float EngineForce;
 // float EngineCurrent;
//  float Power;
};

PACKED_END

bool ED4M_init(struct st_Self *SELF, Locomotive *loco, Engine *eng);
void ED4M_set_destination(st_Self *SELF, int dest);
void ED4M_ALSN(struct st_Self *SELF, const Locomotive *loco);
int ED4M_Step(struct st_Self *SELF, const ElectricLocomotive *loco, ElectricEngine *eng, float gameTime);

#endif // VL15_LOGIC_H
