/* Robot pintor web, control de un plotter en tiempo real desde un móvil o tablet. Plataforma de Arduino UNO.

   Basado en proyecto de IoT_lover. Arduino - Drawing via Web Using Step Motor Controller
   https://www.hackster.io/iot_lover/arduino-drawing-via-web-using-step-motor-controller-cb5f33#_=_

   Autor: Javier Vargas. El Hormiguero.
   https://creativecommons.org/licenses/by/4.0/
*/

//CONFIGURACION GENERAL
//Comandos
#define PEN_STATE_UP  0
#define PEN_STATE_DOWN  1
#define CMD_PEN_UP    0
#define CMD_PEN_DOWN  1
#define CMD_MOVE  2
//Muestreo
#define MuestreoMOT 10 //ms de muestreo para el control de motores
#define MuestreoWEB 50 //ms de muestreo para la recepción de coordenadas web
//Coordenadas web
#define xwebMax 54000
#define ywebMax 66000
#define xwebMin -2500
#define ywebMin -2100
//Coordenadas motores (0 posición minima)
#define xmotMax 45000
#define ymotMax 65000

byte penState = PEN_STATE_UP;
byte cmd = 0;

long x_web = 0;
long y_web = 0;
long x_mot = 0;
long y_mot = 0;
unsigned long mm, mw = 0;

//Ecuaciones de la recta para relacion de coordenadas web -> coordenadas del motor (pasos del motor)
const float x_a = (float)xmotMax / (xwebMax - xwebMin);
const float x_b = -(float)xwebMin * x_a;
const float y_a = (float)ymotMax / (ywebMax - ywebMin);
const float y_b = -(float)ywebMin * y_a;

#include "motor.h"
#include "web.h"

void webtomot(long xweb, long yweb, long *xmot, long *ymot) {
  *xmot = xweb * x_a + x_b;
  *ymot = yweb * y_a + y_b;
  *xmot = constrain(*xmot, 0, xmotMax);
  *ymot = constrain(*ymot, 0, ymotMax);
}


void setup() {
  Serial.begin(9600);
  webInit();
  motorInit();
  servoInit();
  timerInit();
  Serial.print("WebSocket server address : ");
  Serial.println(Phpoc.localIP());
}

void loop() {
  //MOTOR LOOP
  if (mm != millis() / MuestreoMOT) {
    mm = millis() / MuestreoMOT;

    //Movimiento de motores
    GoTo(x_mot, y_mot);
  }

  //WEB LOOP
  if (mw != millis() / MuestreoWEB) {
    mw = millis() / MuestreoWEB;

    //Dato disponible
    if (ReadWebData(&x_web, &y_web, &cmd)) {

      //Mapeo en las coordenadas de los motores
      webtomot(x_web, y_web, &x_mot, &y_mot);

      switch (cmd) {
        case CMD_PEN_DOWN:
          //Espera a llegar a la ultima posicion indicada
          while (1) {
            GoTo(x_mot, y_mot);
            if (!MotorMoving()) break;
            delay(MuestreoMOT);
          }
          sendPositionToWeb(x_web, y_web, penState);
          penDown();
          break;

        case CMD_PEN_UP:
          //Espera a llegar a la ultima posicion indicada
          while (1) {
            GoTo(x_mot, y_mot);
            if (!MotorMoving()) break;
            delay(MuestreoMOT);
          }
          sendPositionToWeb(x_web, y_web, penState);
          penUp();
          break;

        case CMD_MOVE:
          //Devuelve la posicion recibida
          sendPositionToWeb(x_web, y_web, penState);
          break;
      }
    }
  }

}
