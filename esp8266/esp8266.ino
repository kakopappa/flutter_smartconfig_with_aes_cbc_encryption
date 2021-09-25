
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SmartConfigProv.h"

SmartConfigProv sc;
 
void setup()
{
    Serial.begin(115200);
  
    Serial.println("Begin SmartConfig...");  
    sc.beginSmartConfig();
     
    while(1){
       delay(500);
              
       if(sc.smartConfigDone()){
         Serial.println("SmartConfig Success");
         break;
       }
    }    
}
 

void loop()
{ 
}
