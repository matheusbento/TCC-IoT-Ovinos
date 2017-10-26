// includes
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <RDM6300.h>
#include "HX711.h"

// variaveis do hx711
#define DOUT 4
#define CLK 5

// quantidade de leitura para fazer media
#define NUMREADINGS 10


SoftwareSerial RFID(12, 33);

uint8_t Payload[6]; // used for read comparisons

// qtde de amostras
float readings[NUMREADINGS];         

// índice da leitura atual
int index2 = 0; 

// total móvel                           
float total = 0;   
   
// média                     
float average = 0;

// quantida de leituras
int quantleituras = 0;


// FATOR DE CALIBRACAO
//float calibration_factor = -429900.00; //-7050 worked for my 440lb max scale setup
float calibration_factor = -221000; //-7050 worked for my 440lb max scale setup


// login e senha
const char* ssid     = "GG";
const char* password = "123456789";

// enderecos
const char* host = "192.168.0.102";
char thingSpeakAddress[] = "192.168.0.102";

// dados thingspeak
//String writeAPIKey = "JERZ3KEJDYXVE3ZK";
const int updateThingSpeakInterval = 30 * 1000;      // Time interval in milliseconds to update ThingSpeak (number of seconds * 1000 = interval)

// Variable Setup
long lastConnectionTime = 0; 
boolean lastConnected = false;
int failedCounter = 0;

float myvalue = 1234;
// Initialize Arduino Ethernet Client
WiFiClient client;

// iniciando o modulo
HX711 scale(DOUT, CLK);


RDM6300 RDM6300(Payload);


void setup()
{
  index2=0;
  total=0;
  average=0;
  quantleituras=0;
  RFID.begin(9600);
  // Start Serial for debugging on the Serial Monitor
  Serial.begin(9600);

  // Start Ethernet on Arduino
  startEthernet();
  // resetando a array
  for (int i = 0; i < NUMREADINGS; i++)
    readings[i] = 0;                      // inicializa todas as leituras com 0

  //Informacoes iniciais
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  //Serial.println("Aguardando a TAG..\n");
}

void loop()
{
  while (RFID.available() > 0)
  {
    uint8_t c = RFID.read();
    if (RDM6300.decode(c))
    {
   

      Serial.println();
  // Read value from Analog Input Pin 0
  

float pesoAgora = 0;
  do{
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  total -= readings[index2];               // subtrair a última leitura
  readings[index2] = scale.get_units() * 0.453592; // ler do sensor
  pesoAgora = readings[index2];
  total += readings[index2];               // adicionar leitura ao total
  index2 = (index2 + 1);                    // avançar ao próximo índice

  if (index2 >= NUMREADINGS) {              // se estiver no fim do vetor...
    index2 = 0;                            // ...meia-volta ao início
    average = total / NUMREADINGS;          // calcular a média
    Serial.print("Valor da Media -> ");
    Serial.println(average,3);  
  }else{
    Serial.print("Peso Agora -> ");
    Serial.println(pesoAgora,3);  
    average = 0;
    }
  
  

  // Print Update Response to Serial Monitor
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected)
  {
    Serial.println("...disconnected");
    Serial.println();

    client.stop();
  }

  // Update ThingSpeak
  if(!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval) && (average>0))
  {
    String pesoString = String(average, 3);
    updateThingSpeak(pesoString);
    //updateThingSpeak("id="+analogValue0+"&peso="+pesoString);
  }

  // Check if Arduino Ethernet needs to be restarted
  if (failedCounter > 3 ) {startEthernet();}

  lastConnected = client.connected();
  delay(200);
      }while(pesoAgora>=-2);
    }
  }
}

void updateThingSpeak(String tsData)
{
  Serial.print("Enviou -> ");
  Serial.println(tsData);
  if (client.connect(thingSpeakAddress, 80))
  {         
    client.print("POST /peso/index.php HTTP/1.1\n");
    client.print("Host: 192.168.0.102\n");
    client.print("Connection: close\n");
    //client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");

    client.print("id=");
    for (int i = 0; i < 5; i++) {
        client.print(Payload[i], HEX);
      }
    client.print("&peso=");
    client.print(tsData);
    //updateThingSpeak("id="+analogValue0+"&peso="+pesoString);

    lastConnectionTime = millis();

    if (client.connected())
    {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();

      failedCounter = 0;
    }
    else
    {
      failedCounter++;

      Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
      Serial.println();
    }

  }
  else
  {
    failedCounter++;

    Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    Serial.println();

    lastConnectionTime = millis(); 
  }
}

void startEthernet()
{
   failedCounter = 0;

  client.stop();

  Serial.println("Connecting Arduino to network...");
  Serial.println();  

  delay(1000);

  WiFi.begin(ssid, password);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(1000);
}
