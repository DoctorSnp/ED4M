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

   Light_BufferRight = 6,
   Light_BufferLeft = 7,
   Light_SigUp1 = 12,
   Light_SigUp2 = 13,
   //Light_SigDown1 = ,

}en_Lights;

typedef enum sounds
{
    Default_Tumbler = 26,
    Kran_395 = 193,
    BV = 17,
    EPK_INIT = 129,
    //EPK_OFF = 107,
    EPK_ALARM_S = 56, // 57 не занимать!
    TP_UP = 200,
    TP_DOWN = 201,
    Revers =15,
    PesokButton = 4,
    FinalStop = 54,
    Controller = 16,
    Kran395_Otpusk = 500,
    Kran395_Poezdnoe = 502,
    Kran395_Slugebnoe = 504,
    Kran395_Extrennoe = 508,
    Svistok = 10,
    Tifon = 8,
    Avtomat = 191,
    VU = 192,
    RB = 25,
    Switch = 113
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


    /* Задняя панель */
    Switch_LightSalon = 30,
    Switch_LightCab = 31,
    Switch_SigUp = 32,
    Switch_SigDown = 33,
    Switch_BufLeft = 34,
    Switch_EPT = 35,
    Packetnik_ObogrevMaslo = 36,
    Switch_Vent_I_Otoplenie = 37, // +
    Switch_ObogrevZerkal = 38, // +
    Tumbler_VspomCompressos = 39, // +
    Tumbler_ObogrevCab = 40,
    Tumbler_ObogrevCabIntensiv = 41,
    Switch_BufRight = 42,
    Packetnik_ObogrevCabDop = 43,
    Switch_SvetPult = 44,
    Switch_XZ = 45,
    Switch_VentCab = 46, // +
    Switch_Radio = 47, //+
    Switch_XZ_2 = 48, //+

    Switch_StekloobogrOkon_Lob = 51,
    Switch_StekloobogrOkon_Marshr = 52,
    Switch_StekloobogrevOkon_Bok = 53,
    Tmb_Tormozhenie = 55, //+
    Tmb_Dvornik_Mashinist = 56, //+
    Tmb_Dvornik_Pomoshnik = 57, //+

    Switch_PitALSN_1 = 58,
    Switch_PitALSN_2 = 59,
    Switch_Osveshenie = 60,
    Switch_ObogrevMasla = 61,
    Switch_Panto = 62,
    Switch_Pitanie_Dverey = 63,
    Switch_Projector_I_signaly = 64,
    Switch_BufferFonar_I_Dvorniki = 65,
    Switch_PitanieStekloobogrevLob = 66,  // +
    Switch_PitanieStekloobogrevMarshrut = 67, // +


}Tumblers;


typedef enum en_Sensors
{
    Sns_Voltage = 1,
    Sns_BrakeCil = 2,
    Sns_SurgeTank = 3,
    Sns_BrakeLine = 4,
    Sns_PressureLine = 5,

    /*КЛУБ*/
    Sns_KLUB_Speed1 = 23,
    Sns_KLUB_Time = 24,
    Sns_KLUB_KM = 25,
    Sns_KLUB_Station = 26,
    Sns_KLUB_SigName = 30,
    Sns_KLUB_RassDoCeli = 31,
    Sns_KLUB_Manevr = 34,
    Sns_KLUB_Green4 = 40,
    Sns_KLUB_Green3 = 41,
    Sns_KLUB_Green2 = 42,
    Sns_KLUB_Green1 = 43,
    Sns_KLUB_Yellow = 44,
    Sns_KLUB_RedYellow = 45,
    Sns_KLUB_Red = 46,
    Sns_KLUB_White = 47,

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
    Btn_SvistokMash = 17,
    Btn_Svistok2 = 70,
    Btn_Svistok3 = 71,
    Btn_TifonMash = 16,
    Btn_Tifon2 = 69,
    Btn_RB_D = 22,
    Btn_RB = 23,
    Btn_RescueBrake = 38,

    Btn_Zapros = 49,
    Btn_Povtor = 50,
    Btn_Zvonok = 54,
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

  Arm_PressureLine = 75,
  Arm_DoubleForce = 76,

}Arms;


#endif // ELEMENTS_H
