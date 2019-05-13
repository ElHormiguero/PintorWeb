/* Control de PHPoC WiFi Shield 2 para Arduino
 * https://www.hackster.io/iot_lover/arduino-drawing-via-web-using-step-motor-controller-cb5f33#_=_
*/
#define PinLed A0

#include <Phpoc.h>
PhpocServer server(80);

#define RESOLUTION 1

void webInit() {
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  server.beginWebSocket("xy_plotter");
  pinMode(PinLed, OUTPUT);
  digitalWriteFast(PinLed, LOW);
}

void sendPositionToWeb(long x, long y, byte state) {
  char wbuf[20];
  String data = String(F("[")) + x + String(F(",")) + y + String(F(",")) + state + String(F("]\n"));
  data.toCharArray(wbuf, data.length() + 1);
  server.write(wbuf, data.length());
}

void xyCheckUpdateToWeb(long x, long y, byte state) {
  static long preX = 0;
  static long preY = 0;

  //Variaciones de posicion
  long deltaX = x - preX;
  long deltaY = y - preY;

  //Distancia mínima de resolucion
  long dist = sqrt(pow(deltaX, 2) + pow(deltaY, 2));
  boolean isUpdate = false;
  if (dist > RESOLUTION)
    isUpdate = true;

  if (isUpdate == false || dist == 0)
    return false;

  //Envia la posicion
  sendPositionToWeb(x, y, state);

  preX = x;
  preY = y;
}

boolean ReadWebData(long *xweb, long *yweb, byte *c) {

  // wait for a new client:
  PhpocClient client = server.available();

  if (client) {
    String data = client.readLine();
    if (data) {
      digitalWriteFast(PinLed, HIGH);
      byte separatorPos1 = data.indexOf(':');
      byte separatorPos2 = data.lastIndexOf(':');
      *c = data.substring(0, separatorPos1).toInt();
      *xweb = data.substring(separatorPos1 + 1, separatorPos2).toInt();
      *yweb = data.substring(separatorPos2 + 1).toInt();
      return 1;
    }
  }
  digitalWriteFast(PinLed, LOW);
  return 0;
}
