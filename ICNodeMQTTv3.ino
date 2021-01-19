
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h> 
#define INTERVALO_ENVIO       1000

Servo servo; 
int sensorPin = A0;
float tensaoReal,resistencia,sensorValor,corrente,ServoA;
int ultimoEnvioMQTT = 0;

const char* ssid = "Springfield";            // WiFi SSID
const char* password = "siscxrisc1";        // WiFi Password
const char* mqtt_server = "192.168.0.180";  // IP Broker MQTT
 
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {

  Serial.begin(115200);
  servo.attach(D4);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void enviaLDR(){

  char MsgLDR[5];
  char MsgCorrente[5];
  char MsgResistencia[5];
  
  sensorValor = analogRead(sensorPin);  

  tensaoReal = (sensorValor*3.3)/1024.0;

  resistencia = (1000.0*tensaoReal)/(3.3-tensaoReal);

  corrente = tensaoReal/resistencia;
  
      Serial.print(" | LDR: ");
      Serial.print(sensorValor,3);
      Serial.print(" | Resistencia: ");
      Serial.print(tensaoReal,3);
      Serial.print(" (V)");
      Serial.print(" | Corrente: ");
      Serial.println(resistencia,3);
  
    sprintf(MsgLDR,"%f",sensorValor);
    client.publish("IC/LDR", MsgLDR);
    
    sprintf(MsgResistencia,"%f",resistencia);
    client.publish("IC/Resistencia", MsgResistencia);

    sprintf(MsgCorrente,"%f",corrente);
    client.publish("IC/Corrente", MsgCorrente);
  
}

void callback(char* topic, byte* payload, unsigned int length) {
 String string;
 char Servomsg[5];
 
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 String topico = String((char*)topic);
 Serial.println(topico);

if(topico=="IC/Servo"){
   for (int i = 0; i < length; i++) {
      string+=((char)payload[i]); 
   }
   Serial.print(string);
   Serial.print(" toInt ");
  
   int pos = string.toInt(); 
   Serial.println(pos);
   
   servo.write(pos); 
  
   delay(15); 
}

if(topico=="IC/Automatico"){
  for (int i = 0; i < length; i++) {
      string+=((char)payload[i]); 
   }
  int variacao = string.toInt(); 
  servo.write(0);
   for(int j=0; j<=90; j=j+variacao){
    servo.write(j);
    ServoA = j;
    delay(1000);
    enviaLDR();
    sprintf(Servomsg,"%f",ServoA);
    client.publish("IC/lerServo", Servomsg);
   }
   servo.write(0);
   ServoA = 0;
   sprintf(Servomsg,"%f",ServoA);
   client.publish("IC/lerServo", Servomsg);
}
}

void reconnect() {
  
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP8266Client","teste","raspberry")) {
      Serial.println("connected");
      client.subscribe("IC/Servo"); 
      client.subscribe("IC/Automatico"); 
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      delay(5000);
    }
  }
}
void loop() {   
  
  if (!client.connected()) {
    reconnect();
  }
    
  if ((millis() - ultimoEnvioMQTT) > INTERVALO_ENVIO)
  {
      enviaLDR();
      ultimoEnvioMQTT = millis();
  }
  
  client.loop();
  delay(100);
}
