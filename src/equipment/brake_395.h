#ifndef BRAKE_395_H
#define BRAKE_395_H

#include "ts.h"
#include "brake_data.h"

class Brake_395
{
public:
  Brake_395();
  int init();
  void  step(int pos, const Locomotive* loco, Engine *eng, float gameTime);
  const st_Brake_395 &constData();

private:
  st_Brake_395 m_data;
  const Locomotive* m_loco;
  Engine *m_eng;
  int m_prevPos = -1;

  void m_checkBrake(int pos, const Locomotive *loco, Engine *eng, float gameTime);
  void m_soundBrake(int pos, const Locomotive *loco);
};

#endif // BRAKE_395_H
