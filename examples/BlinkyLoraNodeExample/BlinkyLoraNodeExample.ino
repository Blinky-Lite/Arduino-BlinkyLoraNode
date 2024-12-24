bool core1_separate_stack = true;
#define BLINKY_DIAG         false
#define NODE_DIAG         false
#define NODEADDRESS 11
#define GATEWAYADDRESS 10
#define CHSPIN 17           // LoRa radio chip select
#define RSTPIN 14           // LoRa radio reset
#define IRQPIN 15           // LoRa radio IRQ
#define LORSBW 62e3
#define LORSPF 9
#define LORFRQ 868E6

#include <BlinkyLoraNode.h>

unsigned long lastPublishTime;

struct NodeData
{
  uint8_t led1;
  uint8_t led2;
  uint16_t publishInterval;
  float chipTemp;
}; 
NodeData nodeData;
NodeData nodeReceivedData;
    
int led1Pin = 14;
int led2Pin = 17;
int led1;
int led2;
int signLed1;
int signLed2;

void setupLora()
{
  if (BLINKY_DIAG > 0)
  {
     Serial.begin(9600);
     delay(10000);
  }
  BlinkyLoraNode.begin(sizeof(nodeData), BLINKY_DIAG, NODEADDRESS, GATEWAYADDRESS, CHSPIN, RSTPIN, IRQPIN, LORFRQ, LORSPF, LORSBW);
}

void setupNode() 
{
  if (NODE_DIAG > 0)
  {
     Serial.begin(9600);
     delay(10000);
  }
  
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  nodeData.led1 = 255;
  nodeData.led2 = 255;
  nodeData.publishInterval = 3000;

  led1 = 0;
  led2 = 255;
  signLed1 = 1;
  signLed2 = -1;
  analogWrite(led1Pin, led1);    
  analogWrite(led2Pin, led2);   
  lastPublishTime = millis(); 
}

void loopNode() 
{
  unsigned long now = millis();
  if ((now - lastPublishTime) > nodeData.publishInterval)
  {
    lastPublishTime = now;
    nodeData.chipTemp = analogReadTemp();
    boolean successful = BlinkyLoraNode.publishNodeData((uint8_t*) &nodeData, false);
  }
  led1 = led1 + signLed1;    
  led2 = led2 + signLed2;    
  analogWrite(led1Pin, led1);    
  analogWrite(led2Pin, led2);

  if (led1 >= (int) nodeData.led1) signLed1 = -1;
  if (led2 >= (int) nodeData.led2) signLed2 = -1;
  if (led1 == 0) signLed1 = 1;
  if (led2 == 0) signLed2 = 1;
  delay(5);

  if (BlinkyLoraNode.retrieveGatewayData((uint8_t*) &nodeReceivedData) )
  {
    nodeData.led1             = nodeReceivedData.led1;
    nodeData.led2             = nodeReceivedData.led2;
    nodeData.publishInterval  = nodeReceivedData.publishInterval;          
  }

}
