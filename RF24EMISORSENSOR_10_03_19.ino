#include <SPI.h>
#include "RF24.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#define NUM_READS 1  // Number of sensor reads for filtering

const byte pindato = 2;

OneWire oneWireObjeto(pindato);
DallasTemperature sensorDS18B20(&oneWireObjeto);

typedef struct {        // Structure to be used in percentage and resistance values matrix to be filtered (have to be in pairs)
  int moisture;
  long resistance;
} values;

int activeDigitalPin = 4;         // 4 or 5 interchangeably
int supplyVoltageAnalogPin;       // 4-ON: A0, 5-ON: A1
int sensorVoltageAnalogPin;       // 4-ON: A1, 5-ON: A0

int supplyVoltage;                // Measured supply voltage
int sensorVoltage;                // Measured sensor voltage

const long TempC=24;
  // similarly check short resistance by shorting the sensor terminals and replace the value here.
const long short_CB=0;
const long open_CB=199;
values valueOf[NUM_READS];        // Calculated moisture percentages and resistances to be sorted and filtered

int i;                            // Simple index variable

const long knownResistor = 5500;

RF24 myRadio (7, 8); 

byte addresses[][6] = {"1Node", "2Node"};

int ref = 0;
int ref2 = 0;

struct package
{
  int humedad;
  long resistencia;
  long temperatura;
  
};

struct boton
{
  int estado = 0;
};

typedef struct boton Val;

Val vlv;

typedef struct package Package;
Package data;

void setup()
{
  Serial.begin(9600);
  delay(10);
  sensorDS18B20.begin();// detectando sensor temperatura por el serial
  delay(10);
  myRadio.begin();  
  myRadio.setChannel(115); 
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ; 
  myRadio.openWritingPipe(addresses[1]);              //Abre el canal de escritura
  myRadio.openReadingPipe(1, addresses[0]);           //Abre el canal de lectura

  //initialize the digital pin as an output.
  // Pin 6 is sense resistor voltage supply 1
  pinMode(3,OUTPUT);
  pinMode(4, OUTPUT);    

  // initialize the digital pin as an output.
  // Pin 7 is sense resistor voltage supply 2
  pinMode(5, OUTPUT);   

  delay(100);   
}

void loop()
{


   for (i=0; i<NUM_READS; i++) {

    setupCurrentPath();      // Prepare the digital and analog pin values

    // Read 1 pair of voltage values
    digitalWrite(activeDigitalPin, HIGH);                 // set the voltage supply on
    delay(10);
    supplyVoltage = analogRead(supplyVoltageAnalogPin);   // read the supply voltage
    sensorVoltage = analogRead(sensorVoltageAnalogPin);   // read the sensor voltage
    digitalWrite(activeDigitalPin, LOW);                  // set the voltage supply off  
    delay(10); 

    // Calculate resistance and moisture percentage without overshooting 100
    // the 0.5 add-term is used to round to the nearest integer
    // Tip: no need to transform 0-1023 voltage value to 0-5 range, due to following fraction

   valueOf[i].resistance =( long( float(knownResistor) * ( supplyVoltage - sensorVoltage ) / sensorVoltage ))/ -1;
   delay(100);

   // valueOf[i].moisture = min( int( pow( valueOf[i].resistance/31.65 , 1.0/-1.695 ) * 400 + 0.5 ) , 100 );
//  valueOf[i].moisture = min( int( pow( valueOf[i].resistance/331.55 , 1.0/-1.695 ) * 100 + 0.5 ) , 100 );

  
  Serial.print("sensor resistance = ");
  Serial.println(data.resistencia);
  Serial.print("\n");
   delay(10);
   if (valueOf[i].resistance>550.00) 
    {
      if(valueOf[i].resistance>8000.00)
      {
     // valueOf[i].moisture=-2.246-5.239*(valueOf[i].resistance/1000.00)*(1+.018*(TempC-24.00))-.06756*(valueOf[i].resistance/1000.00)*(valueOf[i].resistance/1000.00)*((1.00+0.018*(TempC-24.00))*(1.00+0.018*(TempC-24.00))); 
         //Serial.print("Centibares = ");
         //Serial.println(valueOf[i].moisture); 

      } 
     if (valueOf[i].resistance>1000.00) 
      {
        if (valueOf[i].resistance >= 5000)
        {
      valueOf[i].moisture=open_CB;
      Serial.print("Centibares = ");
       Serial.println(valueOf[i].moisture); 
     
        }
        if (valueOf[i].resistance <= 4950){
         valueOf[i].moisture=(-3.213*(valueOf[i].resistance/1000.00)-4.093)/(1-0.009733*(valueOf[i].resistance/1000.00)-0.01205*(TempC)) ;
       Serial.print("Centibares = ");
       Serial.println(valueOf[i].moisture/-1); 
          }
      }
      if (valueOf[i].resistance<1000.00)
        {
       if (valueOf[i].resistance>650.00)
        {
          
       // valueOf[i].moisture=-20.00*((valueOf[i].resistance/1000.00)*(1.00+0.018*(TempC-24.00))-0.55);
         //Serial.print("Centibares = ");
          //Serial.println(valueOf[i].moisture); 
                }
                 valueOf[i].moisture=0;
        Serial.print("Centibares = ");
         Serial.println(valueOf[i].moisture); 
        }
     }
     if(valueOf[i].resistance<650.00)
      {
        if(valueOf[i].resistance>300.00)
        {
        valueOf[i].moisture=0.00;
        Serial.print("Centibares = ");
         Serial.println(valueOf[i].moisture); 
        
        }
        if(valueOf[i].resistance<300.00)
        {
          //if(valueOf[i].resistance>=short_resistance)
         
          
          {   
          valueOf[i].moisture=short_CB; //240 is a fault code for sensor terminal short
          Serial.print("Centibares = ");
         Serial.println(valueOf[i].moisture); 
          //Serial.print("Entered Sensor Short Loop WM1 \n");
          }
         
        }
       /* if(valueOf[i].resistance <=open_resistance)
        {
        valueOf[i].moisture=open_CB; //255 is a fault code for open circuit or sensor not present 
        Serial.print("Centibares = ");
        Serial.println(valueOf[i].moisture); 
        Serial.print("Entered Open or Fault Loop for WM1 \n");
        }*/
      }

  }
 sortMoistures();
  // end of multiple read loop

  // Sort the moisture-resistance vector according to moisture

    sensorDS18B20.requestTemperatures();
  // Print out median values
  data.resistencia = valueOf[i].resistance;
  data.humedad = valueOf[i].moisture;
  data.temperatura = sensorDS18B20.getTempCByIndex(0);

/*
  Serial.print("sensor humedad = ");
  Serial.println(data.humedad);
  Serial.print("\n");
*/ /*Serial.print("sensor resistencia = ");
  Serial.println(data.resistencia);
  Serial.print("\n");*/
 /* Serial.print("sensor temperatura = ");
  Serial.println(data.temperatura);
  Serial.print("\n");*/

  
 myRadio.stopListening();  
 myRadio.write(&data, sizeof(data)); 
delay(10);

 myRadio.startListening();

 if(myRadio.available()){
 myRadio.read(&vlv, sizeof(vlv));
 Serial.print(vlv.estado);
 if (vlv.estado == 1){
  digitalWrite(3, HIGH);
  }
  else if(vlv.estado == 0){
    digitalWrite(3, LOW);  }
    
  }
  
  delay(1000); 
}

void setupCurrentPath() {
  if ( activeDigitalPin == 4 ) {
    activeDigitalPin = 5;
    supplyVoltageAnalogPin = A1;
    sensorVoltageAnalogPin = A0;
  }
  else {
    activeDigitalPin = 4;
    supplyVoltageAnalogPin = A0;
    sensorVoltageAnalogPin = A1;
  }
}

// Selection sort algorithm
void sortMoistures() {
  int j;
  values temp;
  for(i=0; i<NUM_READS-1; i++)
    for(j=i+1; j<NUM_READS; j++)
      if ( valueOf[i].moisture > valueOf[j].moisture ) {
        temp = valueOf[i];
        valueOf[i] = valueOf[j];
        valueOf[j] = temp;
      }
}
