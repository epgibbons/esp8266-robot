#include <SoftwareSerial.h>

#include <TinyGPS.h>

// Import required libraries
#include "ESP8266WiFi.h"
// for OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <aREST.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x62); 
 
// And connect 2 DC motors to port M3 & M4 !
Adafruit_DCMotor *L_MOTOR = AFMS.getMotor(2);
Adafruit_DCMotor *R_MOTOR = AFMS.getMotor(1);

// Create aREST instance
aREST rest = aREST();

//gps serial 
TinyGPS gps;
// rx 14, tx 12 - brown
SoftwareSerial gpsSer(14,12, false, 256);

// WiFi parameters
const char* ssid_def = "EE-4594jp";
const char* password_def = "glue-shine-few";
const char* ssid = "nexus";
const char* password = "isabelle";

// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// aRest Functions
int stop(String message);
int forward(String message);
int right(String message);
int left(String message);
int backward(String message);


// constants
const int motor_speed_straight =255;

const int motor_speed_rotate =100;


void setup(void)
{  
  // Start Serial
  Serial.begin(115200);
  Serial.println("");
  Serial.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  Serial.print( "Arduino IDE version: ");
  Serial.println( ARDUINO, DEC);
  
  gpsSer.begin(9600);


  // Init motor shield
  AFMS.begin();  

  // Functions          
  rest.function("stop", stop);
  rest.function("forward", forward);
  rest.function("left", left);
  rest.function("right", right);
  rest.function("backward", backward);
      
  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("robot");

  int attempt=0;
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && (attempt<20), attempt++) {
    delay(500);
    Serial.print(".");
  }
  if( WiFi.status() != WL_CONNECTED){
      Serial.print("Connection failed. Trying default ssid");
      WiFi.begin(ssid_def, password_def);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }    
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  
  // Start the server
  server.begin();
  Serial.println("Server started");
  
  // Print the IP address

}

void loop() {


    
  ArduinoOTA.handle();
  
  // Handle REST calls
  // Without the delay call some messages are lost
  WiFiClient client = server.available();
  if (client) {
    while(!client.available()){
      delay(1);
    }
    rest.handle(client);
  }
 
  while (gpsSer.available()){
    if(gps.encode(gpsSer.read())){
      long lat, lon;
      unsigned long fix_age, time, date, speed, course;
      unsigned long chars;
      unsigned short sentences, failed_checksum;
       
      // retrieves +/- lat/long in 100000ths of a degree
      gps.get_position(&lat, &lon, &fix_age);
       
      // time in hhmmsscc, date in ddmmyy
      gps.get_datetime(&date, &time, &fix_age);
       
      // returns speed in 100ths of a knot
      speed = gps.speed();
       
      // course in 100ths of a degree
      course = gps.course();

      Serial.print("Latitude:");
      Serial.print( lat);
      Serial.print(" Longitude:");
      Serial.println( lon);
      
      break;
    }
  }

}



int stop(String command) {
  
  // Stop
  Serial.println("Stop");
  L_MOTOR->setSpeed(0);
  R_MOTOR->setSpeed(0);
  L_MOTOR->run( RELEASE );
  R_MOTOR->run( RELEASE );
  
}

int forward(String command) {
  Serial.println("forward");  
  // Stop
  L_MOTOR->setSpeed(motor_speed_straight);
  R_MOTOR->setSpeed(motor_speed_straight);
  L_MOTOR->run( FORWARD );
  R_MOTOR->run( FORWARD );
  
}

int left(String command) {
  
  // Stop
  L_MOTOR->setSpeed(motor_speed_rotate);
  R_MOTOR->setSpeed(motor_speed_rotate);
  L_MOTOR->run( BACKWARD );
  R_MOTOR->run( FORWARD );
  
}

int right(String command) {
  
  // Stop
  L_MOTOR->setSpeed(motor_speed_rotate);
  R_MOTOR->setSpeed(motor_speed_rotate);
  L_MOTOR->run( FORWARD );
  R_MOTOR->run( BACKWARD );
  
}

int backward(String command) {
  
  // Stop
  L_MOTOR->setSpeed(motor_speed_straight);
  R_MOTOR->setSpeed(motor_speed_straight);
  L_MOTOR->run( BACKWARD );
  R_MOTOR->run( BACKWARD );
  
}
