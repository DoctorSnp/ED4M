#ifndef ELEMENTS_H
#define ELEMENTS_H


typedef enum en_Lights
{
   Light_Proj_Half1 = 0,
   Light_Proj_Half2 = 6,
   Light_Proj_Half3 = 12,
   Light_Proj_Half4 = 18,

   Light_Proj_Full1 = 1,
   Light_Proj_Full2 = 7,
   Light_Proj_Full3 = 13,
   Light_Proj_Full4 = 19,
   Light_Proj_Full5= 30,
   Light_Proj_Full6= 31,
   Light_Proj_Full7= 32,
   Light_Proj_Full8= 33,

}en_Lights;

typedef enum sounds
{
    Default_Tumbler = 26,
    Kran_395 = 193,
    BV = 17,
    EPK_INIT = 129,
    //EPK_OFF = 107,
    EPK_ALARM_S = 56, // 57 не занимать!
    TP_UP = 13,
    TP_DOWN = 14,
    Revers =15,
    PesokButton = 4,
    FinalStop = 54,
    Controller = 16,
    Kran395_Otpusk = 124,
    Kran395_Poezdnoe = 126,
    Kran395_Slugebnoe = 128,
    Kran395_Extrennoe = 130,
    Svistok = 10,
    Tifon = 8,
    Avtomat = 191,
    VU = 192,
    RB = 25

}SoundsID;

typedef enum en_Tumblers
{
    Tmb_AvarEPT = 4,
    Tmb_vozvrZash = 5,
    Tmb_otpusk = 6,
    Tmb_manage_leftDoors_TCH_M = 7,
    Tmb_manage_RightDoors_TCH_M = 8,
    Tmb_manage_leftDoors_TCH_MP = 9,
    Tmb_manage_RightDoors_TCH_MP = 10,
    Tmb_PantoUP = 11,
    Tmb_PantoDown = 12,
    Tmb_vspomKompr = 13,
    Tmb_lightCab_Dimly = 18,
    Tmb_VU = 19,
    Tmb_AutomatUpr = 20,

    Tmb_leftDoors = 24,
    Tmb_rightDoors = 25,
    Key_EPK = 26,
}Tumblers;


typedef enum en_Sensors
{
    Sns_BrakeCil = 2,
    Sns_SurgeTank = 3,
    Sns_BrakeLine = 4,
    Sns_PressureLine = 5,

    SnsSpeed1 = 23,

}Sensors;

typedef enum en_Lamps
{
    Lmp_RN  = 6,
    Lmp_Doors = 7,
    Lmp_SOT_X = 8,
    Lmp_RP  = 9,
    Lmp_BV  = 10,
    Lmp_LK  = 11,
    Lmp_RB  = 12,
    Lmp_RNDK  = 13,
    Lmp_VspomCepi  = 14,
    Lmp_SOT  = 15,
    Lmp_T  = 16,
    Lmp_O  = 17,
    Lmp_K  = 18,
    Lmp_PSS  = 19,
    Lmp_Preobr  = 20,
    Lmp_LobOkna  = 21,
    Lmp_SryvAutostop = 22,

}Lamps;

typedef enum en_Buttons
{
    Btn_Svistok = 17,
    Btn_Tifon = 16,
    Btn_RB_D = 22,
    Btn_RB = 23,
    Btn_RescueBrake = 38
}Buttons;


typedef enum en_Equipment
{
  SL_2M = 300,
}Equipment;


typedef enum en_Arms
{
  Arm_Controller = 0,
  Arm_Reverse = 1,
  Arm_395 = 2,
  Arm_ManualBrake = 4,
}Arms;


#endif // ELEMENTS_H
