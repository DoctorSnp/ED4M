/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef BRAKE_395_H
#define BRAKE_395_H

#include "brake_data.h"

/**
 * @brief The Brake_395 class Кран машиниста усл. номер 395 для RTrainSim
 * @version 0.01
 * @date Январь 2021
 */

class Brake_395
{
public:
  Brake_395();

  /**
   * @brief init Инициализация крана. (пока просто возвращает 0 - что означает успех)
   * @param BrakeControlState Флаги работы тормозной системы : каждый бит - отдельный флаг
   *                                1 бит - Включен контроль ТМ.
   *                                2 бит - Включено ЭПТ .
   *                                3 бит - Включен контроль НМ.
   * @return Возвращает 0, если инициализация удалась и -1 в случае ошибки.
   */
  int init(Engine *eng, int BrakeControlState);

  /**
   * @brief step Шаг работы крана
   * @param pos Позиция крана машиниста
   * @param loco Указатель на локомотив
   * @param eng Указатель на ходовую часть
   * @param millisec Тек. значение миллисекунд (float time) библиотечной функции Run
   * @param BrakeControlState Флаги работы тормозной системы : каждый бит - отдельный флаг
   *                                1 бит - Включен контроль ТМ.
   *                                2 бит - Включено ЭПТ .
   *                                3 бит - Включен контроль НМ.
   */
  void  step(int pos, const Locomotive* loco, Engine *eng, float millisec, int BrakeControlState);

  /**
   * @brief constData Получение состояния крана машиниста
   * @return структура-описатель крана машиниста
   */
  const st_Brake_395 &constData();

private:
  st_Brake_395 m_data = {};
  const Locomotive* m_loco;
  Engine *m_eng;

  /**
   * @brief m_checkBrake Установка тормозного эффекта
   * @param gameTime  Тек. значение миллисекунд (float time) библиотечной функции Run
   */
  void m_checkBrake(float gameTime);

  /**
   * @brief m_soundBrake Озвучка крана.
   */
  void m_soundBrake();
};

#endif // BRAKE_395_H
