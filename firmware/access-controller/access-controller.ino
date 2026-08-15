#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include "config.h"

//OBJETOS
Servo miServo;  // Crea un objeto de la clase Servo para controlar el servo
WiFiClient wClient;
PubSubClient mqtt_client(wClient);

//Strings para wifi-mqtt
const String ssid = WIFI_SSID;
const String password = WIFI_PASSWORD;
const String mqtt_server = MQTT_HOST;
const String mqtt_user = MQTT_USERNAME;
const String mqtt_pass = MQTT_PASSWORD;

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
  String mensaje;
  mensaje.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    mensaje += static_cast<char>(payload[i]);
  }
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
  Serial.println();
  Serial.println("Empieza setup...");
  Serial.println("Placa: " + String(ARDUINO_BOARD));

  // crea topics
  ID_PLACA = "ESP_" + String(ESP.getChipId());
  topic_II12 = String(MQTT_TOPIC_PREFIX) + "/ACCESO";
  topic_estado = topic_II12 + "/ESTADO";
  topic_CONEXION = topic_II12 + "/conexion";
  

  //CONEXIONADO COMPONENTES
  miServo.attach(PIN_SERVO);  
  pinMode(PIN_ZUMBADOR, OUTPUT); 
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_IR1, INPUT); 
  pinMode(PIN_IR2, INPUT); 
  pinMode(PIN_LIBRE, OUTPUT);


  conecta_wifi();

  mqtt_client.setServer(mqtt_server.c_str(), MQTT_PORT);
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
  if(statusIR1 != 1)
  {
    Serial.println("VEHICULO EN ENTRADA");
    II12.clear();
    stringII12 = "";
    II12["detectado"] = "1";
    II12["entrada"] = "0";
    II12["salida"] = "0";
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
