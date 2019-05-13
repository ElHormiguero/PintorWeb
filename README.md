# PintorWeb
Basado en Arduino UNO.
Un rotulador acoplado a una estructura móvil con dos grados de libertad replica el movimiento trazado sobre una tablet en tiempo real. De esta forma aquello que pintamos en el móvil o tablet, queda pintado sobre un folio real.
La estructura del plotter es de AxiDraw. Dos motores paso a paso permiten el movimiento del rotulador en el plano, y un servomotor realiza la acción de bajar o subir según se quiera o no pintar durante el desplazamiento.
Una placa de conexión wifi acoplada al microcontrolador crea una red local en la cual está conectada la tablet. Desde el navegador de la tablet se ejecuta un código de formato .php que se encarga de rastrear la posición de la pantalla táctil y enviarla en tiempo real al microcontrolador que gestiona el movimiento de cada uno de los motores. Las coordenadas obtenidas se escalan y se rotan para hacerlas coincidir con los ejes que mueven los motores, y finalmente se establece una velocidad proporcional al error de posición. La posición actual del rotulador se obtiene manteniendo la cuenta de los pasos de cada motor.

Basado en proyecto de IoT_lover. [Arduino - Drawing via Web Using Step Motor Controller](https://www.hackster.io/iot_lover/arduino-drawing-via-web-using-step-motor-controller-cb5f33#_=_)

![](https://github.com/ElHormiguero/PintorWeb/blob/master/Imagenes/IMG_20190513_220616.jpg)
![](https://github.com/ElHormiguero/PintorWeb/blob/master/Imagenes/IMG_20190513_220626.jpg)
