// This #include statement was automatically added by the Particle IDE.
#include <neopixel.h>
#include <SparkFunBME280.h>
#include "math.h"

//SYSTEM_MODE(MANUAL); //Makes wifi/internet all manual, ie off unless turned on - for testing

//--------------------- Sensor setup --------------------------//

//Global sensor object
BME280 mySensor;

float humidity = 999;
float tempF = 999;
float tempC = 999;
float pascals = 999;
float inches = 999;
float dewptc = 999;
float dewptf = 999;

char SERVER [] = "weatherstation.wunderground.com"; //standard server - for slower send intervals
//char SERVER[] = "rtupdate.wunderground.com";        //Rapidfire update server - for multiple sends per minute
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";

//Station Identification
char ID [] = "xxx-xxx"; //Your station ID here
char PASSWORD [] = "xxx-xxx"; //your Weather Underground password here

TCPClient client;

//--------------------- LED setup --------------------------//

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 12
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

//--------------------- Setup Program, runs once --------------------------//

void setup()
{
	//***Driver settings********************************//
	mySensor.settings.commInterface = I2C_MODE;
	mySensor.settings.I2CAddress = 0x76; //default for sparkfun  is 0x77

	//***Operation settings*****************************//
	mySensor.settings.runMode = 3; //Normal mode

	//Standby can be:  0, 0.5ms - 1, 62.5ms - 2, 125ms -  3, 250ms - 4, 500ms - 5, 1000ms - 6, 10ms - 7, 20ms
	mySensor.settings.tStandby = 0;
	mySensor.settings.filter = 4;

    //*OverSample can be: 0, skipped - 1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.tempOverSample = 5;
    mySensor.settings.pressOverSample = 5;
	mySensor.settings.humidOverSample = 5;

	//***LED Setup*****************************//
    strip.begin(); //begin neo pixel strip
    strip.show(); //initialize strip

	Serial.begin(9600);
	Serial.print("Program Started\n");
	
	delay(10);              //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	mySensor.begin();       //Calling .begin() causes the settings to be loaded

}

//--------------------- Main Program, repeats --------------------------//


void loop()
{

    getData();
    confirm();
    pixelCaseF(tempF);
    delay(4000);
    pixelCaseH(humidity);
    delay(4000);
    sendToWU();
    Serial.println();
	Serial.println("---------------");

}


//----------------- Functions, Small programs that the main program can call on ---------//

void getData(){
//Each loop, take a reading.
//Start with temperature, as that data is needed for accurate compensation.
//Reading the temperature updates the compensators of the other functions
//in the background.

// Measure Temperature
tempC = mySensor.readTempC();
tempF = (tempC * 9.0)/ 5.0 + 32.0;

// Measure Relative Humidity
humidity = mySensor.readFloatHumidity();

//Measure Pressure
pascals = mySensor.readFloatPressure();
inches = pascals * 0.0002953; // Calc for converting Pa to inHg (Wunderground expects inHg)

// Caclulate Dew Point
dewptc = dewPoint(tempC, humidity); //Dew point calc(wunderground)
dewptf = (dewptc * 9.0)/ 5.0 + 32.0;

}

void confirm(){
  //Sends the sensor variables to serial to check
    Serial.print("Temperature: ");
	Serial.print(tempF);
	Serial.println(" degrees F");

	Serial.print("Pressure: ");
	Serial.print(inches);
	Serial.println(" Pa");

	Serial.print("Relative Humidity: ");
	Serial.print(humidity);
	Serial.println(" %");
	//Serial.print("NOW: ");


}

void sendToWU()
{
  Serial.println("connecting...");

  if (client.connect(SERVER, 80)) {
  Serial.println("Connected");
  client.print(WEBPAGE);
  client.print("ID=");
  client.print(ID);
  client.print("&PASSWORD=");
  client.print(PASSWORD);
  client.print("&dateutc=now");      //can use 'now' instead of time if sending in real time
  client.print("&tempf=");
  client.print(tempF);
  client.print("&humidity=");
  client.print(humidity);
  client.print("&dewptf=");
  client.print(dewptf);
  client.print("&baromin=");
  client.print(inches);
  client.print("&action=updateraw");//Standard update
  //client.print("&softwaretype=Particle-Photon&action=updateraw&realtime=1&rtfreq=5");  //Rapid Fire update rate - for sending multiple times per minute, specify frequency in seconds
  client.println();
  Serial.println("Upload complete");
  delay(200);                         // Without the delay it is less reliable (if sleeping after send)
  }else{
    Serial.println(F("Connection failed"));
  return;
  }
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double tempC, double humidity)
{
	// (1) Saturation Vapor Pressure = ESGG(T)
	double RATIO = 373.15 / (273.15 + tempC);
	double RHS = -7.90298 * (RATIO - 1);
	RHS += 5.02808 * log10(RATIO);
	RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
	RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
	RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
	double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
	double T = log(VP/0.61078);   // temp var
	return (241.88 * T) / (17.558 - T);
}

// This method controls the neo-pixel for the "analog" readout on the thermometer. With every temperature group, the number of pixels lit changes along with the color
void pixelCaseF(float tempF) //float tempF
{
 int i;
 strip.setBrightness(64);
 strip.show();
  
  if (tempF >= 90.01)//if above 95 degrees, strip is red and entire strip is lit
 {
   strip.clear();
   for(int i=0;i <= 11;i++)
   {
   strip.setPixelColor(i, strip.Color(255,0,0));
   }
 }
 else if (tempF < 90 && tempF >= 85.01) //if 90 > tempF >= 80 orange and strip is partially lit up to 11th pixel
 { 
   strip.clear();
   for(int i=0;i <= 10;i++)
  {
  strip.setPixelColor(i, strip.Color(255,128,0)); 
  }
 }
 else if (tempF < 85 && tempF >= 78.01)// if 80 > tempF >= 70 yellow-green and strip is lit up to 9th pixel
 {
   strip.clear();
   for(int i = 0; i <= 8; i++)
   {
    strip.setPixelColor(i,strip.Color(204,255,0)); 
   }
 }  
 else if (tempF < 78 && tempF >= 75.01)// if 75.01 > tempF >= 60 green and strip is lit up to 7th pixel
{
  strip.clear();
  for(int i = 0; i<= 6; i++)
  {
   strip.setPixelColor(i,strip.Color(0,255,0));
  }
}
 else if (tempF < 75 && tempF >= 73.01) //if 60 > tempF >= 50 blue and strip is lit up to 5th pixel
{
  strip.clear();
  for(int i = 0; i <= 4; i++)
  {
    strip.setPixelColor(i,strip.Color(0,0,255));
  }
} 
 else if (tempF < 73 && tempF >= 70.01) //if 50 > tempF >= 40 aqua and strip is lit to 3rd pixel
 {
   strip.clear();
   for(int i = 0; i <= 2; i++)
  {
   strip.setPixelColor(i, strip.Color(0,255,255));
  }
 }
 else if (tempF < 70 && tempF >= 65.01) //if 40 > tempF >= 32 fuschia and strip is lit to 2th pixel
 {
   strip.clear();
   for(int i = 0; i <= 1; i++)
  {
   strip.setPixelColor(i, strip.Color(153, 51,255)); 
  }
 }
 else if (tempF < 65) //temp < freezing white and strip is lit to 1st pixel
 {
   strip.clear();
   for(i = 0;i <= 0; i++)
 { 
   strip.setPixelColor(i, strip.Color(255,255,255)); 
 }//end for
 }
 
strip.show(); //update color change

} //end temp


// This method controls the neo-pixel for the "analog" readout of Humidity. 
void pixelCaseH(float humidity)
{
 int i;
 strip.setBrightness(64);
 strip.show();
  
  if (humidity >= 95.01) // Max
 {
   strip.clear();
   for(int i=0;i <= 11;i++)
   {
   strip.setPixelColor(i, strip.Color(0,0,255));
   }
 }
 else if (humidity < 95 && humidity >= 90.01)
 {
   strip.clear();
   for(int i = 0; i <= 8; i++)
   {
    strip.setPixelColor(i,strip.Color(0,0,200)); 
   }
 }  
 else if (humidity < 90 && humidity >= 80.01)
{
  strip.clear();
  for(int i = 0; i<= 6; i++)
  {
   strip.setPixelColor(i,strip.Color(0,0,180));
  }
}
 else if (humidity < 80 && humidity >= 70.01) 
{
  strip.clear();
  for(int i = 0; i <= 4; i++)
  {
    strip.setPixelColor(i,strip.Color(0,0,255));
  }
} 
 else if (humidity < 70 && humidity >= 60.01) 
 {
   strip.clear();
   for(int i = 0; i <= 2; i++)
  {
   strip.setPixelColor(i, strip.Color(0,0,160));
  }
 }
 else if (humidity < 60 && humidity >= 50.01) 
 {
   strip.clear();
   for(int i = 0; i <= 1; i++)
  {
   strip.setPixelColor(i, strip.Color(0,0,140)); 
  }
 }
  else if (humidity < 50 && humidity >= 40.01) 
{
  strip.clear();
  for(int i = 0; i <= 4; i++)
  {
    strip.setPixelColor(i,strip.Color(0,0,120));
  }
} 
 else if (humidity < 40 && humidity >= 30.01) 
 {
   strip.clear();
   for(int i = 0; i <= 2; i++)
  {
   strip.setPixelColor(i, strip.Color(0,0,100));
  }
 }
 else if (humidity < 30 && humidity >= 20.1) 
 {
   strip.clear();
   for(int i = 0; i <= 1; i++)
  {
   strip.setPixelColor(i, strip.Color(0,0,80)); 
  }
 }
 else if (humidity < 20 && humidity >= 10.01) 
 {
   strip.clear();
   for(int i = 0; i <= 1; i++)
  {
   strip.setPixelColor(i, strip.Color(0,0,60)); 
  }
 }
 else if (humidity < 10) // Min
 {
   strip.clear();
   for(i = 0;i <= 0; i++)
 { 
   strip.setPixelColor(i, strip.Color(255,255,255)); 
 }
  }
 
strip.show(); //update color change

} //end humidity