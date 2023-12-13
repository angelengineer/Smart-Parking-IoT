#include <string>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <iostream>
#include <sstream>
#include "DHTesp.h"
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <EasyBuzzer.h>
#include <Servo.h>

//OBJETOS
Servo miServo;  // Crea un objeto de la clase Servo para controlar el servo
WiFiClient wClient;
HTTPClient https;
PubSubClient mqtt_client(wClient);

// definimos macro para indicar función y línea de código en los mensajes
#define DEBUG_STRING "["+String(__FUNCTION__)+"():"+String(__LINE__)+"] "

#define __HTTPS__

#ifdef __HTTPS__
  #include <WiFiClientSecure.h>
  WiFiClientSecure ClienteWiFi;
  const String URL_BASE = "https://iot.ac.uma.es:1880";
  // huella digital SHA-1 del servidor iot.ac.uma.es (vencimiento enero 2024)
  const char* fingerprint = "DE:3C:76:79:45:D8:F0:13:9F:22:A5:42:97:0B:F6:56:4E:E6:B8:FD";
#else
  #include <WiFiClient.h>
  WiFiClient ClienteWiFi;
  const String URL_BASE = "http://192.168.1.147:1880";
#endif

//Strings para wifi-mqtt
const String ssid     = "DIGIFIBRA-YD3Q";
const String password = "s9Ck42zKFefb";
//const String ssid     = "infind";
//const String password = "1518wifi";
const String mqtt_server = "iot.ac.uma.es";
const String mqtt_user = "II12";
const String mqtt_pass = "q4PAfzx5";

//Cadenas para topics e ID
String ID_PLACA;
String topic_CONEXION;
String topic_II12;
String topic_estado;

//PINES
const int PIN_SERVO = 4;  
const int PIN_ZUMBADOR = 16;
const int PIN_BOTON = 12;  
const int PIN_IR1 = 5; //exterior
const int PIN_IR2 = 14; //interior
const int PIN_LIBRE = 2;


//Variables globales
int angulo0 = 0;  // Ángulo inicial del servo
int angulo180 = 180;  // Ángulo final del servo
int paso = 1;  // Incremento en grados para mover el servo
bool abriendoPuerta = false;
bool cerrandoPuerta = false;
String puerta = "";
bool confirmado = false;
bool libre = false;

//JSON 
String stringII12;
DynamicJsonDocument II12(256);



/*------------------------------------------------------------------------------------------
---------------CONECTA WIFI---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void conecta_wifi() {
  Serial.println("Connecting to " + ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected, IP address: " + WiFi.localIP().toString());
}

/*------------------------------------------------------------------------------------------
---------------CONECTA MQTT---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void conecta_mqtt() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect(ID_PLACA.c_str(), mqtt_user.c_str(), mqtt_pass.c_str(), topic_CONEXION.c_str(), 1, true, "{\"online\": false}")) {
      Serial.println(" conectado a broker: " + mqtt_server);
      //Subscripciones
      mqtt_client.subscribe(topic_estado.c_str());
      //Publicacion cuando se conecta
      mqtt_client.publish(topic_CONEXION.c_str(), "{\"online\": true}", true);

    } else {
      Serial.println("ERROR:" + String(mqtt_client.state()) + " reintento en 5s");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*------------------------------------------------------------------------------------------
---------------PETICIOENES HTTP---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
int http_GET(String URL, String* respuesta)
{
  return peticion_HTTP("GET", URL, "", respuesta);
}
//-----------------------------------------------------------
int http_POST(String URL, String body, String* respuesta)
{
  return peticion_HTTP("POST", URL, body, respuesta);
}
//-----------------------------------------------------------
int peticion_HTTP (String metodo, String URL, String body, String* respuesta)
{
  int httpCode=-1;
  unsigned long start = millis();
  if (https.begin(ClienteWiFi, URL)) {  // HTTPS

      Serial.println(DEBUG_STRING + metodo +" petición... " + URL);
      // start connection and send HTTP header
      https.addHeader("Content-Type", "application/json");
      if(metodo=="GET" )  httpCode = https.GET();
      if(metodo=="POST"){ httpCode = https.POST(body);
        Serial.println(DEBUG_STRING +"cuerpo solicitud: \n     "+ body);
      }
      // httpCode will be negative on error
      if (httpCode > 0) {
        Serial.println(DEBUG_STRING + metodo +" respuesta... código Status: "+ String(httpCode));

        // queremos la respuesta del servidor
        if (respuesta!=NULL) {
          *respuesta = https.getString();
          Serial.println(DEBUG_STRING+"cuerpo respuesta: \n     "+ *respuesta);
        }
      } else {
        Serial.println(DEBUG_STRING+ metodo +"... falló, error: "+ String(https.errorToString(httpCode).c_str()) );
      }

      https.end();
    } else {
      Serial.println(DEBUG_STRING+"No se pudo conectar");
    }
    Serial.println(DEBUG_STRING+"tiempo de respuesta: "+ String(millis()-start) +" ms\n");
    return httpCode;
}


/*------------------------------------------------------------------------------------------
---------------ZUMBADOR---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void zumbador(int gap, int freq){

  // Genera un tono con el zumbador
  tone(PIN_ZUMBADOR, freq);  // 1000 Hz
  delay(gap);  
  // Detiene el tono
  noTone(PIN_ZUMBADOR);
  delay(gap);  
 
}

/*------------------------------------------------------------------------------------------
---------------ABRIR PUERTA---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void abrirPuerta(){
    puerta = "abierta";
    zumbador(500,1000);
    miServo.write(angulo180);  // Establece el ángulo del servo
    Serial.println("PUERTA ABIERTA");
    delay(1000);  // Espera 1 segundo en la posición final

}

/*------------------------------------------------------------------------------------------
---------------CERRAR PUERTA---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void cerrarPuerta(){
    puerta = "cerrada";
    zumbador(500,1000);
    miServo.write(angulo0);
    Serial.println("PUERTA CERRADA");
    delay(1000);

}


/*------------------------------------------------------------------------------------------
---------------PARKING ESTADO--------------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/

void estado_parking(char* topic, byte* payload, unsigned int length) {
  String mensaje = String(std::string((char*)payload, length).c_str());
  Serial.println("Mensaje recibido [" + String(topic) + "] " + mensaje);
  // compruebo el topic
  if (String(topic) == topic_estado) {
    StaticJsonDocument<512> json;  // el tamaño tiene que ser adecuado para el mensaje
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(json, mensaje.c_str());

    // Compruebo si no hubo error
    if (error) {
      Serial.print("Error funcion deserializeJson(): ");
      Serial.println(error.c_str());
    } else if (json.containsKey("libre"))  // comprobar si existe el campo/clave que estamos buscando
    {
      libre = json["libre"];
      Serial.print("Mensaje OK, libre = ");
      Serial.println(libre);
      
    } 
    else if (json.containsKey("confirmado"))  // comprobar si existe el campo/clave que estamos buscando
    {
      confirmado = json["confirmado"];
      Serial.print("Mensaje OK, confirmado = ");
      Serial.println(confirmado);
    }
      else {
      Serial.print("Error : ");
      Serial.println("campo no encontrado");
    }
  }  // if topic
  else {
    Serial.println("Error: Topic desconocido");
  }
}

/*------------------------------------------------------------------------------------------
---------------SETUP---------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  int codigoStatus;
  Serial.begin(115200);
  Serial.println();
  Serial.println("Empieza setup...");
  Serial.println(DEBUG_STRING+"Placa: "+String(ARDUINO_BOARD));
  Serial.println(DEBUG_STRING+"Comienza SETUP...");

  // crea topics
  ID_PLACA = "ESP_" + String(ESP.getChipId());
  topic_II12 = "II12/ACCESO";
  topic_estado = "II12/ACCESO/ESTADO";
  topic_CONEXION="II12/ACCESO/conexion";
  

  //CONEXIONADO COMPONENTES
  miServo.attach(PIN_SERVO);  
  pinMode(PIN_ZUMBADOR, OUTPUT); 
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_IR1, INPUT); 
  pinMode(PIN_IR2, INPUT); 
  pinMode(PIN_LIBRE, OUTPUT);


  conecta_wifi();

  #ifdef __HTTPS__
  ClienteWiFi.setFingerprint(fingerprint); // se comprobará el certificado del servidor
 //ClienteWiFi.setInsecure(); // si no se quiere comprobar el certificado del servidor
  #endif

  mqtt_client.setServer(mqtt_server.c_str(), 1883);
  mqtt_client.setBufferSize(512);  // para poder enviar mensajes de hasta X bytes
  

  mqtt_client.setCallback(estado_parking);
  conecta_mqtt();


  miServo.write(angulo0);
  puerta = "cerrada";
  Serial.println("PUERTA CERRADA");

    Serial.println("Termina setup en " + String(millis()) + " ms");
}




/*------------------------------------------------------------------------------------------
---------------LOOP--------------------------------------------------------------------------
--------------------------------------------------------------------------------------------*/
void loop() {
  if (!mqtt_client.connected()) {
    conecta_mqtt();
  }
  mqtt_client.loop();  // esta llamada para que la librería recupere el control



  //Parking libre o ocupado
  if (libre)
  {
    digitalWrite(PIN_LIBRE, HIGH);
  }
  else
  {
    digitalWrite(PIN_LIBRE, LOW);
  }
  

  // Reinicializacion  variables antes de asignar nuevos valores
  II12.clear();
  stringII12 = "";
  II12["detectado"] = "0";
  II12["entrada"] = "0";
  II12["salida"] = "0";



  int statusBoton = digitalRead(PIN_BOTON);
  int statusIR1 = digitalRead(PIN_IR1);
  int statusIR2 = digitalRead(PIN_IR2);
  
  //Solicitud salida con botón
  if (statusBoton == LOW) {  // Comprueba si el botón está presionado (estado bajo)
    Serial.println("¡Botón presionado!");
    abrirPuerta();

    statusIR1 = digitalRead(PIN_IR1);
    statusIR2 = digitalRead(PIN_IR2);
    delay(15);
  } 

  //cerrar puerta
  if (statusIR1 == 1 and statusIR2 == 1 and puerta == "abierta")
  {
    delay(1500);
    Serial.println("No hay vehiculo");
    cerrarPuerta();
  }

  //Se detecta un vehiculo que quiere entrar
  bool detectado = false;
  if(statusIR1!=1 and !detectado)
  {
    Serial.println("VEHICULO EN ENTRADA");
    II12.clear();
    stringII12 = "";
    II12["detectado"] = "1";
    II12["entrada"] = "0";
    II12["salida"] = "0";
    detectado = true;
    // Publicacion ------------------------------------------
    serializeJson(II12, stringII12);
    Serial.println("Topic   : " + topic_II12);
    Serial.println("Payload : " + stringII12);
    mqtt_client.publish(topic_II12.c_str(), stringII12.c_str());
    delay(300);
    Serial.println("Confirmado = " + String(confirmado));

    if (confirmado)
    {
      Serial.println("¡Acceso confirmado!");
      abrirPuerta();
    }
  }
  else if(statusIR1==1) 
  {
    detectado = false;
  }

  //Se detecta una entrada
  bool entrada = false;
  while(statusIR1!=1) //senesor exterior se activa
  {
    //Actualizacion estado senesores
    statusIR1 = digitalRead(PIN_IR1);
    statusIR2 = digitalRead(PIN_IR2);
    Serial.println("WHILE EXTERIOR");
    delay(300);
    while(statusIR2!=1) //sensor interior se activa
    {
        //Actualizacion estado senesores
        statusIR1 = digitalRead(PIN_IR1);
        statusIR2 = digitalRead(PIN_IR2);
       Serial.println("WHILE INTERIOR");
       delay(300);
      if (!entrada)
      {
        entrada = true;
        Serial.println("VEHICULO ENTRA");
        delay(300);
        II12.clear();
        stringII12 = "";
        II12["detectado"] = "0";
        II12["entrada"] = "1";
        II12["salida"] = "0";
        // Publicacion ------------------------------------------
        serializeJson(II12, stringII12);
        Serial.println("Topic   : " + topic_II12);
        Serial.println("Payload : " + stringII12);
        mqtt_client.publish(topic_II12.c_str(), stringII12.c_str());
        delay(300);
      }
      
    }
   
  }



  //Se detecta una salida
  bool salida = false;
  while(statusIR2!=1) //senesor interior se activa
  {
    //Actualizacion estado senesores
    statusIR1 = digitalRead(PIN_IR1);
    statusIR2 = digitalRead(PIN_IR2);
    Serial.println("WHILE INTERIOR");
    delay(300);
    while(statusIR1!=1) //sensor exterior se activa
    {
        //Actualizacion estado senesores
        statusIR1 = digitalRead(PIN_IR1);
        statusIR2 = digitalRead(PIN_IR2);
       Serial.println("WHILE EXTERIOR");
       delay(300);
      if (!salida)
      {
        salida = true;
        Serial.println("VEHICULO SALE");
        delay(300);
        II12.clear();
        stringII12 = "";
        II12["detectado"] = "0";
        II12["entrada"] = "0";
        II12["salida"] = "1";
        // Publicacion ------------------------------------------
        serializeJson(II12, stringII12);
        Serial.println("Topic   : " + topic_II12);
        Serial.println("Payload : " + stringII12);
        mqtt_client.publish(topic_II12.c_str(), stringII12.c_str());
        delay(300);
      }
      
    }
   
  }


 Serial.println("LOOP FIN");
 delay(500);
}