/*  Control de 2 motores paso a paso y un servomotor incluidos en la estructura del plotter AxiDraw.
    Drivers de motores DRV8825
    Control con Timer 1 y Timer 2 de Arduino UNO
*/

//PINES
#define PinDIR1 4
#define PinSTEP1 2
#define PinDIR2 8
#define PinSTEP2 7
#define PinServo 9

//CONFIGURACION
//Motor paso a paso
#define Kvel 0.025f //Relacion diferencia de posicion - velocidad (control proporcional)
#define aMax 10//Aceleracion máxima (no normalizada, depende del tiempo de muestreo)
#define velMin 10 //Velocidad minima
#define angRot PI/4 //Rotacion de las coordenadas
#define DIRECCION1 HIGH //Direccion del motor 1
#define DIRECCION2 LOW  //Direccion del motor 2
//Servomotor
#define servoDown 0 //Angulo del servo en posicion de pintar
#define servoUp 180  //Angulo del servo en posicion de no pintar
#define timeservo 200 //ms de espera para bajar o subir el servo

//Timer
#include <digitalWriteFast.h> //Más rapido!

boolean dir1 = 1, dir2 = 1;
volatile long steps1, steps2 = 0;
volatile long steps1obj, steps2obj = 0;
volatile boolean timer1On, timer2On = 0;

//////////////////
//////////////////---------------- INTERRUPCIONES Y TIMER
//////////////////

void timerInit() {
  //Configuración de registros de control de timers
  TCCR1A = 0;                                              // Timer1 CTC mode 4
  TCCR1B = (1 << WGM12) | (1 << CS12);                     // Prescaler=256
  OCR1A = 255;                                             // Motor parado
  TCNT1 = 0;

  TCCR2A = (1 << WGM21);                                   // Timer2 CTC mode 4
  TCCR2B = (1 << CS22) | (1 << CS21);                      // Prescaler=256
  OCR2A = 255;                                             // Motor parado
  TCNT2 = 0;

  TIMSK1 |= (1 << OCIE1A);
  TIMSK2 |= (1 << OCIE2A);
}

void delay_1us() {
  __asm__ __volatile__ (
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop");
}

//Interrupciones ISR, steps 1 y 2
ISR(TIMER1_COMPA_vect) {
  if (!timer1On) return;
  if (steps1 == steps1obj) return;
  digitalWriteFast(PinSTEP1, HIGH);
  delay_1us();
  delay_1us();
  digitalWriteFast(PinSTEP1, LOW);
  if (dir1) steps1++;
  else steps1--;
}

ISR(TIMER2_COMPA_vect) {
  if (!timer2On) return;
  if (steps2 == steps2obj) return;
  digitalWriteFast(PinSTEP2, HIGH);
  delay_1us();
  delay_1us();
  digitalWriteFast(PinSTEP2, LOW);
  if (dir2) steps2++;
  else steps2--;
}

//Control de timers
void SetTimer1(int v) {
  if (!timer1On) {
    //Inicio de la interrupcion
    if (v > 0) timer1On = 1;
    else return;
  }
  //Interrupcion parada
  if (v == 0) {
    timer1On = 0;
    return;
  }
  //Valor del comparador
  v = constrain(v, 1, 1000);
  float period = 1000 / v;
  OCR1A = map(period, 1, 1000, 1, 255);
  //Reinicia la cuenta si el valor de comparación está por encima del valor actual
  if (TCNT1 > OCR1A) {
    TCNT1 = 0;
  }
}

void SetTimer2(int v) {
  if (!timer2On) {
    //Inicio de la interrupcion
    if (v > 0) timer2On = 1;
    else return;
  }
  //Interrupcion parada
  if (v == 0) {
    timer2On = 0;
    return;
  }
  //Valor del comparador
  v = constrain(v, 1, 1000);
  float period = 1000 / v;
  OCR2A = map(period, 1, 1000, 1, 255);
  //Reinicia la cuenta si el valor de comparación está por encima del valor actual
  if (TCNT2 > OCR2A) {
    TCNT2 = 0;
  }
}

//////////////////
//////////////////---------------- CONTROL DE MOTORES
//////////////////


void motorInit() {
  //PinMode
  pinMode(PinDIR1, OUTPUT);
  pinMode(PinSTEP1, OUTPUT);
  pinMode(PinDIR2, OUTPUT);
  pinMode(PinSTEP2, OUTPUT);
}

void SetVelocidad(float v1, float v2, boolean filtro) {
  static float v1_ant = 0;
  static float v2_ant = 0;

  //Limite de velocidad entre los limites de velocidad maxima
  v1 = constrain(v1, -1000, 1000);
  v2 = constrain(v2, -1000, 1000);

  //Limite de la aceleracion
  if (filtro) {
    float a1 = constrain(v1 - v1_ant, -aMax, aMax);
    float a2 = constrain(v2 - v2_ant, -aMax, aMax);
    v1 = v1_ant + a1;
    v2 = v2_ant + a2;
  }
  v1_ant = v1;
  v2_ant = v2;

  //Direccion motor 1
  if (v1 >= 0) {
    if (!dir1) {
      dir1 = 1;
      digitalWriteFast(PinDIR1, DIRECCION1);
    }
  }
  else if (dir1) {
    dir1 = 0;
    digitalWriteFast(PinDIR1, !DIRECCION1);
  }

  //Direccion motor 2
  if (v2 >= 0) {
    if (!dir2) {
      dir2 = 1;
      digitalWriteFast(PinDIR2, DIRECCION2);
    }
  }
  else if (dir2) {
    dir2 = 0;
    digitalWriteFast(PinDIR2, !DIRECCION2);
  }

  //Velocidad motor 1
  SetTimer1(abs(v1));
  //Velocidad motor 2
  SetTimer2(abs(v2));
}

boolean MotorMoving() {
  if (!timer1On && !timer2On) return 0;
  if (steps1 == steps1obj && steps2 == steps2obj) return 0;
  return 1;
}

//////////////////
//////////////////---------------- CONTROL DE MOVIMIENTO
//////////////////

void rotarCoordenadas(long x, long y, long *xout, long *yout) {
  *xout = x * cos(angRot) - y * sin(angRot);
  *yout = x * sin(angRot) + y * cos(angRot);
}

void GoTo(long x, long y) {

  long xrot, yrot = 0;
  float velX, velY = 0;

  //Rotacion de las coordenadas
  rotarCoordenadas(x, y, &xrot, &yrot);

  float diffX = (float)xrot - steps1;
  float diffY = (float)yrot - steps2;

  //Velocidad X
  if (diffX == 0) velX = 0;
  else {
    velX = Kvel * diffX;
    //Limite de velocidad
    //Velocidad minima
    if (abs(velX) < velMin) {
      if (velX < 0) velX = -velMin;
      if (velX > 0) velX = velMin;
    }
  }

  //Velocidad Y
  if (diffY == 0) velY = 0;
  else {
    velY = Kvel * diffY;
    if (abs(velY) < velMin) {
      if (velY < 0) velY = -velMin;
      if (velY > 0) velY = velMin;
    }
  }

  //Pasos objetivo
  steps1obj = xrot;
  steps2obj = yrot;
  //Velocidad
  SetVelocidad(velX, velY, 1);
}

//////////////////
//////////////////---------------- CONTROL DE SERVO
//////////////////

void servoInit() {
  pinMode(PinServo, OUTPUT);
}

void setServo(int ang) {
  //Control software de la señal PWM del servo (Timers ocupados)
  unsigned long m = 0;
  unsigned long tout = millis() + timeservo;
  //Ciclo de trabajo entre 1ms y 2ms
  int duty = map(ang, 0, 180, 1000, 2000);

  while (millis() < tout) {
    //Señal de 50 hz
    if (m != micros() / 20000) {
      m = micros() / 20000;
      digitalWriteFast(PinServo, HIGH);
      delayMicroseconds(duty);
      digitalWriteFast(PinServo, LOW);
    }
  }
}

void penUp() {
  setServo(servoUp);
  penState = PEN_STATE_UP;
}

void penDown() {
  setServo(servoDown);
  penState = PEN_STATE_DOWN;
}
