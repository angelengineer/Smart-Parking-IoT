# Pantalla LCD

El segundo sketch del proyecto controla una pantalla LCD 16x2 y presenta el estado `LIBRE` o `COMPLETO` recibido desde el controlador de acceso.

![image](https://github.com/angelengineer/ProyectoIoT/assets/145780323/7ef3a761-05e6-460e-822c-f5c57bcca223)

## Conexiones

El sketch utiliza el constructor `LiquidCrystal(12, 11, 10, 5, 4, 3, 2)`:

| Señal | Pin de la placa LCD |
| --- | ---: |
| RS | 12 |
| RW | 11 |
| Enable | 10 |
| D4 | 5 |
| D5 | 4 |
| D6 | 3 |
| D7 | 2 |
| Estado de disponibilidad | 13 |

La entrada de disponibilidad debe compartir masa con el ESP8266. Comprueba siempre los niveles lógicos admitidos por ambas placas antes de conectarlas.
