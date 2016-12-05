#include <Ticker.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_ADXL345_U.h>

#include <Adafruit_Sensor.h>

#include <TinyGPS.h>
#include <ArduinoJson.h>
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"

// for OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>
#include <Adafruit_MotorShield.h>

#include <ArduinoJson.h>

Ticker secondTick;
volatile int watchdogCount = 0;
void ISRWatchdog() {
  watchdogCount++;
  if (watchdogCount >= 5) {
    Serial.println("Watchdov bites!!!!!!!!!!");
    ESP.reset();
  }
}


// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x62);

// And connect 2 DC motors to port M3 & M4 !
Adafruit_DCMotor *L_MOTOR = AFMS.getMotor(2);
Adafruit_DCMotor *R_MOTOR = AFMS.getMotor(1);

TinyGPS gps;

// WiFi parameters
const char* ssid_def = "EE-4594jp";
const char* password_def = "glue-shine-few";
const char* ssid = "nexus";
const char* password = "isabelle";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           8080

// Create an instance of the server
//WiFiServer servserverer(LISTEN_PORT);
ESP8266WebServer webserver;

int displayReadings(String message);
int stop(String message);
int forward(String message);
int right(String message);
int left(String message);
int backward(String message);
void setHeading();

// constants
const int motor_speed_straight = 255;

const int motor_speed_rotate = 100;

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
/* Assign a unique ID to this sensor at the same time */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12346);

float headingDegrees;
float latitude, longitude;
float accelX, accelY, accelZ;
float magX, magY, magZ;


void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webserver.uri();
  message += "\nMethod: ";
  message += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webserver.args();
  message += "\n";
  for (uint8_t i = 0; i < webserver.args(); i++) {
    message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }
  webserver.send(404, "text/plain", message);
}
void setup(void)
{
  // Start Serial
  Serial.begin(9600);



  Serial.println("");
  Serial.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
  Serial.print( "Arduino IDE version: ");
  Serial.println( ARDUINO, DEC);

//  gpsSer.begin(9600);
  //  gps_port.begin(9600);

  // Init motor shield
  AFMS.begin();


  int attempt = 0;
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && (attempt < 20), attempt++) {
    delay(500);
    Serial.print(".");
  }
  if ( WiFi.status() != WL_CONNECTED) {
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

  webserver.on("/", []() {
    webserver.send(200, "text/plain", String(longitude));
  });
  webserver.on("/left", []() {
    left("");
    webserver.send(204,"");
  });
  webserver.on("/right", []() {
    right("");
    webserver.send(204,"");
  });
  webserver.on("/forward", []() {
    forward("");
    webserver.send(204,"");
  });
  webserver.on("/backward", []() {
    backward("");
    webserver.send(204,"");
  });
  webserver.on("/stop", []() {
    stop("");
    webserver.send(204,"");
  });
  webserver.on("/variable", HTTP_GET, []() {
    String varName = webserver.arg("name");
    if ( varName == "latitude") {
      webserver.send(200, "text/plain", String(latitude));
    }
    else if ( varName == "longitude") {
      webserver.send(200, "text/plain", String(longitude));
    }
    else if ( varName == "headingDegrees") {
      webserver.send(200, "text/plain", String(headingDegrees));
    }
    else if ( varName == "accelX") {
      webserver.send(200, "text/plain", String(accelX));
    }
    else if ( varName == "accelY") {
      webserver.send(200, "text/plain", String(accelY));
    }
    else if ( varName == "accelZ") {
      webserver.send(200, "text/plain", String(accelZ));
    }
    else {
      webserver.send(200, "text/plain", String(latitude));
    }
  });
  webserver.on("/heading", [](){
    setHeading();
    webserver.send(204,"");
    });
  webserver.onNotFound(handleNotFound);
  webserver.begin();

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
    watchdogCount = 0; 
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

  setupHMC388L();
  setupADXL345();
  secondTick.attach(1, ISRWatchdog);
}

void setupHMC388L() {
  Serial.println("HMC5883 Magnetometer Test"); Serial.println("");

  /* Initialise the sensor */
  if (!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while (1);
  }

  /* Display some basic information on this sensor */
  mag.setMagGain(HMC5883_MAGGAIN_1_9);
//  displayHMC5883LSensorDetails();
}

void displayHMC5883LSensorDetails(void)
{
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setupADXL345() {
  Serial.println("Accelerometer Test"); Serial.println("");

  /* Initialise the sensor */
  if (!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while (1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);
  // displaySetRange(ADXL345_RANGE_8_G);
  // displaySetRange(ADXL345_RANGE_4_G);
  // displaySetRange(ADXL345_RANGE_2_G);

  /* Display some basic information on this sensor */
//  displayADXL345SensorDetails();

  /* Display additional settings (outside the scope of sensor_t) */
//  displayDataRate();
//  displayRange();
  Serial.println("");
}

void displayADXL345SensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");
  Serial.println("------------------------------------");
  Serial.println("");
}

void displayDataRate(void)
{
  Serial.print  ("Data Rate:    ");

  switch (accel.getDataRate())
  {
    case ADXL345_DATARATE_3200_HZ:
      Serial.print  ("3200 ");
      break;
    case ADXL345_DATARATE_1600_HZ:
      Serial.print  ("1600 ");
      break;
    case ADXL345_DATARATE_800_HZ:
      Serial.print  ("800 ");
      break;
    case ADXL345_DATARATE_400_HZ:
      Serial.print  ("400 ");
      break;
    case ADXL345_DATARATE_200_HZ:
      Serial.print  ("200 ");
      break;
    case ADXL345_DATARATE_100_HZ:
      Serial.print  ("100 ");
      break;
    case ADXL345_DATARATE_50_HZ:
      Serial.print  ("50 ");
      break;
    case ADXL345_DATARATE_25_HZ:
      Serial.print  ("25 ");
      break;
    case ADXL345_DATARATE_12_5_HZ:
      Serial.print  ("12.5 ");
      break;
    case ADXL345_DATARATE_6_25HZ:
      Serial.print  ("6.25 ");
      break;
    case ADXL345_DATARATE_3_13_HZ:
      Serial.print  ("3.13 ");
      break;
    case ADXL345_DATARATE_1_56_HZ:
      Serial.print  ("1.56 ");
      break;
    case ADXL345_DATARATE_0_78_HZ:
      Serial.print  ("0.78 ");
      break;
    case ADXL345_DATARATE_0_39_HZ:
      Serial.print  ("0.39 ");
      break;
    case ADXL345_DATARATE_0_20_HZ:
      Serial.print  ("0.20 ");
      break;
    case ADXL345_DATARATE_0_10_HZ:
      Serial.print  ("0.10 ");
      break;
    default:
      Serial.print  ("???? ");
      break;
  }
  Serial.println(" Hz");
}

void displayRange(void)
{
  Serial.print  ("Range:         +/- ");

  switch (accel.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print  ("16 ");
      break;
    case ADXL345_RANGE_8_G:
      Serial.print  ("8 ");
      break;
    case ADXL345_RANGE_4_G:
      Serial.print  ("4 ");
      break;
    case ADXL345_RANGE_2_G:
      Serial.print  ("2 ");
      break;
    default:
      Serial.print  ("?? ");
      break;
  }
  Serial.println(" g");
}


unsigned long msAtLastPosRead = 0;
const unsigned long  msPerPosRead = 1000;
void readADXL345();
void readHMC388L();

void loop() {
  unsigned long ms = millis();
  watchdogCount = 0;

  webserver.handleClient();
  ArduinoOTA.handle();

  if ( (ms - msAtLastPosRead) > msPerPosRead) {
    msAtLastPosRead = ms;
    readGPS();
    readADXL345();
    readHMC388L();
  }
}


void readGPS() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (gps.encode(ch)) {

      long lat, lon;
      unsigned long fix_age, time, date, speed, course;
      unsigned long chars;
      unsigned short sentences, failed_checksum;

      // retrieves +/- lat/long in 100000ths of a degree
      gps.get_position(&lat, &lon, &fix_age);

      // time in hhmmsscc, date in ddmmyy
      gps.get_datetime(&date, &time, &fix_age);

      if ( ( fix_age != TinyGPS::GPS_INVALID_AGE) && ( fix_age < 5000)) {
        latitude = lat/1000000.0;
        longitude = lon/1000000.0;
      } else {
        latitude = 0.;
        longitude = 0.;
      }
      break;
    }
  }
}

void readADXL345() {
  /* Get a new sensor event */
  sensors_event_t event;
  accel.getEvent(&event);

  accelX = event.acceleration.x;
  accelY = event.acceleration.y;
  accelZ = event.acceleration.z;
}

void readHMC388L() {
  /* Get a new sensor event */
  sensors_event_t event;
  mag.getEvent(&event);

  /* Display the results (magnetic vector values are in micro-Tesla (uT)) */

  // Hold the module so that Z is pointing 'up' and you can measure the heading with x&y
  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(event.magnetic.y, event.magnetic.x); // in radians
//  String msg;
//  msg = "Compass x:" + String(event.magnetic.x,6) + " y:" + String(event.magnetic.y,6) + " z:"+ String(event.magnetic.z,6);
//  Serial.println(msg);

  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Abingdon is -1degree 2minutes. 360 degreees is 2pi radians. 1 degree = 2*3.14/360 radians = 0.017 radians
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  float declinationAngle = 0.017;
  heading += declinationAngle;

  // Correct for when signs are reversed.
  if (heading < 0)
    heading += 2 * PI;

  // Check for wrap due to addition of declination.
  if (heading > 2 * PI)
    heading -= 2 * PI;

  // Convert radians to degrees for readability.
  headingDegrees = heading * 180 / M_PI;
  magX = event.magnetic.x;
  magY = event.magnetic.y;
  magZ = event.magnetic.z;
}


int displayReadings(String command) {
  Serial.print("Latitude:");
  Serial.print( latitude);
  Serial.print(" Longitude:");
  Serial.println( longitude);
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(accelX); Serial.print("  ");
  Serial.print("Y: "); Serial.print(accelY); Serial.print("  ");
  Serial.print("Z: "); Serial.print(accelZ); Serial.print("  "); Serial.println("m/s^2 ");
  Serial.print("X: "); Serial.print(magX); Serial.print("  ");
  Serial.print("Y: "); Serial.print(magY); Serial.print("  ");
  Serial.print("Z: "); Serial.print(magZ); Serial.print("  "); Serial.println("uT");
  Serial.print("Heading (degrees): "); Serial.println(headingDegrees);
}

int stop(String command) {

  L_MOTOR->setSpeed(0);
  R_MOTOR->setSpeed(0);
  L_MOTOR->run( RELEASE );
  R_MOTOR->run( RELEASE );

}

int forward(String command) {
  L_MOTOR->setSpeed(motor_speed_straight);
  R_MOTOR->setSpeed(motor_speed_straight);
  L_MOTOR->run( FORWARD );
  R_MOTOR->run( FORWARD );

}

int left(String command) {

  L_MOTOR->setSpeed(motor_speed_rotate);
  R_MOTOR->setSpeed(motor_speed_rotate);
  L_MOTOR->run( BACKWARD );
  R_MOTOR->run( FORWARD );

}

int right(String command) {

  L_MOTOR->setSpeed(motor_speed_rotate);
  R_MOTOR->setSpeed(motor_speed_rotate);
  L_MOTOR->run( FORWARD );
  R_MOTOR->run( BACKWARD );

}

int backward(String command) {

  L_MOTOR->setSpeed(motor_speed_straight);
  R_MOTOR->setSpeed(motor_speed_straight);
  L_MOTOR->run( BACKWARD );
  R_MOTOR->run( BACKWARD );

}


void setHeading(){
  String data = webserver.arg("plain");
  StaticJsonBuffer<200> jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);
  String headingToSet = jObject["heading"];
  Serial.println(headingToSet);
}


