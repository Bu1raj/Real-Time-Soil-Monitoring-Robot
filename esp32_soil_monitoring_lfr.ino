//the results of this porject can be enhanced by the use of few more sensors 
//work in progress

#include <WiFi.h>
#include <Adafruit_Sensor.h>   
#include <DHT.h>
#include <DHT_U.h>
#include <ThingSpeak.h>


char* ssid = "Redmi 9";
const char* password = "123454321";

char server[] = "api.thingspeak.com";
long unsigned int myChannelNumber = 2265999;
const char* writeAPIKey = "04I16J0U8L837ZDH";

WiFiClient client;

#define DHTPIN 4
#define DHTTYPE DHT11
#define LedPin 2
#define S0 5
#define S1 18
#define S2 19
#define S3 21
#define out 22
#define lfr_enable 23
#define pump_enable 14

int red_avg = 0;
int blue_avg = 0;
int green_avg = 0;
int write_success;
int pump_on_time;
DHT_Unified dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  pinMode(LedPin, OUTPUT);
  pinMode(S0, OUTPUT); 
  pinMode(S1, OUTPUT); 
  pinMode(S2, OUTPUT); 
  pinMode(S3, OUTPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);
  pinMode(out, INPUT);
  pinMode(lfr_enable, OUTPUT);
  pinMode(pump_enable, OUTPUT);
  digitalWrite(pump_enable, LOW);
  connectWiFi(); 
  ThingSpeak.begin(client); 
}

void loop() {
  run_lfr(5000);
  crop_test();
}

void crop_test(void)
{
  Serial.println("Starting crop test");
  Serial.println("----------------------------------------------------");
  sensors_event_t event;
  blinkX(2, 200);
  delay(10000); //insert the soil moisture sensor manually
  //we can use a rack and pinion steup and stepper motor to insert the soil moisture sensor
  
  int moisture_value = 0;
  
  //soil moisture
  for(int i = 0; i < 30; i++)
  {
    moisture_value += analogRead(32);
    delay(10);
  }
  moisture_value = moisture_value/(float)30.0;
  Serial.print("Analog Value of moisture sensor = ");
  Serial.println(moisture_value);
  Serial.print("Scaled down value");
  float moisture = ((4095 - moisture_value)/(float)2800.00)*100.00;
  Serial.println(moisture);
  
  //temperature and humidity
  dht.temperature().getEvent(&event);
  Serial.print("Temperature: ");
  Serial.print(event.temperature);
  Serial.println("°C");  

  dht.humidity().getEvent(&event);
  Serial.print("Humidity: ");
  Serial.print(event.relative_humidity);
  Serial.println(" %");

  //color sensing
  int redPW = 0;
  int bluePW = 0;
  int greenPW = 0;
  for(int i = 0; i < 10; i++)
  {
    redPW += getRedPW();
    delay(10);
    bluePW += getBluePW();
    delay(10);
    greenPW += getGreenPW();
    delay(10);
  }
  
  red_avg = redPW/(float)10.0;
  blue_avg = bluePW/(float)10.0;
  green_avg = greenPW/(float)10.0;

  Serial.print("Red = ");
  Serial.print(red_avg);
  
  Serial.print("  - Green = ");
  Serial.print(green_avg);
  
  Serial.print("  - Blue = ");
  Serial.println(blue_avg); 
   
  Serial.println("----------------------------------------------------------");
  Serial.println("");
  
  //thingspeak cloud
  ThingSpeak.setField(1, event.temperature);
  ThingSpeak.setField(2, event.relative_humidity);
  ThingSpeak.setField(6, moisture_value);
  ThingSpeak.setField(4, red_avg);
  Serial.println("Writing to Thingspeak");
  write_success = ThingSpeak.writeFields(myChannelNumber, writeAPIKey);
  if(write_success == 200)
  {
    Serial.println("Channel update successful.");
  }
  else
  {
    Serial.println("Problem updating channel. HTTP error code " + String(write_success));
  }
  Serial.println("");
  
  //nitrogen claculation
  float on_time = (130 - red_avg)/(float)4.0; //or on_time = k1(130 - red_avg)/(float)4.0 + k2(optimal_soil_moisture_value - current_soil_moisture_value)^2
  //k1 and k2 are the constants which the determine the strength of the influence of these factors soil_moisture and color value and can be adjusted
  //this relation is hypothetical on further data collecetion we can determine the actual relationships 
  
  on_time = sqrt(on_time*on_time);
  pump_on_time = (unsigned long)on_time*1000;
  Serial.print("Pump on time: ");
  Serial.print(on_time);
  Serial.println("s");
  pump(pump_on_time);

  //additional info can be printed 
  Serial.println("Finished crop testing");
  Serial.println("---------------------------------------------");
  Serial.println("");
}

//switching on the pump for a given time 
void pump(unsigned long time_interval)
{
  unsigned long start_time = millis();
  while(time_interval >= (millis() - start_time))
  {
    digitalWrite(pump_enable, HIGH);
  }
  digitalWrite(pump_enable, LOW);
}

//running lfr for a given time
void run_lfr(unsigned long time_interval)
{
  unsigned long start_time = millis();
  Serial.print("LFR is running for ");
  Serial.print(time_interval/1000);
  Serial.println("secs");
  while(time_interval >= (millis() - start_time))
  {
    digitalWrite(lfr_enable, HIGH);
  }
  digitalWrite(lfr_enable, LOW);
}

//color sensor functions 
int getRedPW() {
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  int PW;
  PW = pulseIn(out, digitalRead(out) == HIGH ? LOW : HIGH);
  return PW;
}

int getGreenPW() {
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  int PW;
  PW = pulseIn(out, digitalRead(out) == HIGH ? LOW : HIGH);
  return PW;
}

int getBluePW() {
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  int PW;
  PW = pulseIn(out, digitalRead(out) == HIGH ? LOW : HIGH);
  return PW;
}

//wifi connnection 
void connectWiFi(){
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
        Serial.println("Connecting......");
        delay(3000);
    }
    Serial.println("Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    blinkX(5,150);  
}

//LED blink function
void blinkX(int numTimes, int delayTime){ 
    for (int g=0;g < numTimes;g++){
        digitalWrite(LedPin, HIGH);  
        delay(delayTime);
        digitalWrite(LedPin, LOW);
        delay(delayTime);       
    }
}
