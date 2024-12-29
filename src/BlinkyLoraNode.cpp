#include "BlinkyLoraNode.h"

BlinkyLoraNodeClass::BlinkyLoraNodeClass(boolean chattyCathy)
{
  _chattyCathy = chattyCathy;
  _sizeofGatewayDataHeader = sizeof(_gatewayDataHeader);
  

}
void BlinkyLoraNodeClass::begin(size_t nodeDataSize, boolean chattyCathy, int16_t nodeAddress, int16_t gateWayAddress, int loraChipSelectPin, int loraResetPin, int loraIRQPin, long loraFreq, int loraSpreadingFactor, long loraSignalBandwidth)
{
  _chattyCathy = chattyCathy;
  _sizeofNodeData = nodeDataSize;
  _sizeOfTransferData = _sizeofGatewayDataHeader + _sizeofNodeData;
  _pnodeDataSend = new (std::nothrow) uint8_t [_sizeOfTransferData];
  _pgatewayDataRecv = new (std::nothrow) uint8_t [_sizeOfTransferData];
  
  _gatewayDataHeader.istate = 1;
  _gatewayDataHeader.iforceArchive = 0;
  _gatewayDataHeader.inodeAddr = (int16_t) nodeAddress;
  _gatewayDataHeader.igatewayAddr = (int16_t) gateWayAddress;
  _gatewayDataHeader.iwatchdog = 0;
  _gatewayDataHeader.irssi = 0;  
  _gatewayDataHeader.isnr = 0;  
  _gatewayHasDataToRead = false;

  LoRa.setPins(loraChipSelectPin, loraResetPin, loraIRQPin);
  
  if (!LoRa.begin(loraFreq)) 
  {
    if (_chattyCathy) Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  LoRa.setSpreadingFactor(loraSpreadingFactor);
  LoRa.setSignalBandwidth(loraSignalBandwidth);
  if (_chattyCathy)
  {
    Serial.println("LoRa init succeeded.");
    Serial.println();
    Serial.println("LoRa Simple Node");
    Serial.println("Only receive messages from gateways");
    Serial.println("Tx: invertIQ disable");
    Serial.println("Rx: invertIQ enable");
    Serial.println();    
  }
  LoRa.onReceive(BlinkyLoraNodeClass::onLoRaReceive);
  LoRa.onTxDone(BlinkyLoraNodeClass::onLoraTxDone);
  LoRa.onCadDone(BlinkyLoraNodeClass::onCadDone);
  BlinkyLoraNodeClass::rxMode();

  return;
}
void BlinkyLoraNodeClass::rxMode()
{
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
}

void BlinkyLoraNodeClass::txMode()
{
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
}
boolean BlinkyLoraNodeClass::publishNodeData(uint8_t* pnodeData, boolean forceArchiveData) 
{
  if (_nodeHasDataToRead) return false;
  if (_pnodeDataSend == nullptr) return false;
  if (_chattyCathy) Serial.println("Publishing node Data");

  _gatewayDataHeader.iwatchdog = _gatewayDataHeader.iwatchdog + 1;
  if (_gatewayDataHeader.iwatchdog > 32765) _gatewayDataHeader.iwatchdog = 0;
  _gatewayDataHeader.irssi = 0;  
  _gatewayDataHeader.isnr = 0;
  _gatewayDataHeader.iforceArchive = 0;
  if (forceArchiveData) _gatewayDataHeader.iforceArchive = 1; 
  
  uint8_t* memPtr = _pnodeDataSend;
  uint8_t* datPtr = (uint8_t*) &_gatewayDataHeader;
  for (int ii = 0; ii < _sizeofGatewayDataHeader; ++ii)
  {
    *memPtr = *datPtr;
    ++memPtr;
    ++datPtr;
  }
  datPtr = pnodeData;
  for (int ii = 0; ii < _sizeofNodeData; ++ii)
  {
    *memPtr = *datPtr;
    ++memPtr;
    ++datPtr;
  }
      
  _crc.restart();
  memPtr = _pnodeDataSend;
  ++memPtr;
  for (int ii = 1; ii < _sizeOfTransferData; ii++)
  {
    _crc.add(*memPtr);
    ++memPtr;
  }
  _gatewayDataHeader.icrc = _crc.calc();
  memPtr = _pnodeDataSend;
  datPtr = (uint8_t*) &_gatewayDataHeader;
  *memPtr = *datPtr;
  _nodeHasDataToRead = true;
  beginSendingLoraData();
  return true;
}
void BlinkyLoraNodeClass::beginSendingLoraData() 
{
  if (!_nodeHasDataToRead) return;
  BlinkyLoraNodeClass::txMode();                      // set tx mode
  LoRa.beginPacket();                                 // start packet
  LoRa.channelActivityDetection();
}
void BlinkyLoraNodeClass::finishSendingLoraData()
{
  LoRa.write(_pnodeDataSend, _sizeOfTransferData);    // add payload
  LoRa.endPacket(true);                               // finish packet and send it
 _nodeHasDataToRead = false;
}
void BlinkyLoraNodeClass::receiveData(int packetSize)
{
  if (_gatewayHasDataToRead) return;;
  uint8_t numBytes = 0;
  uint8_t garbageByte = 0;
  
  if (_chattyCathy) Serial.print("Received LoRa data at: ");
  if (_chattyCathy) Serial.println(millis());
  numBytes = LoRa.available();
  if (numBytes != _sizeOfTransferData)
  {
    for (int ii = 0; ii < numBytes; ++ii) garbageByte = (uint8_t) LoRa.read();
    if (_chattyCathy)
    {
      Serial.print("LoRa bytes do not match. Bytes Received: ");
      Serial.print(numBytes);
      Serial.print(", Bytes expected: ");
      Serial.println(_sizeOfTransferData);
    }
    return;
  }
  LoRa.readBytes(_pgatewayDataRecv, _sizeOfTransferData);
  
  _crc.restart();
  uint8_t* memPtr = _pgatewayDataRecv;
  ++memPtr;
  for (int ii = 1; ii < _sizeOfTransferData; ii++)
  {
    _crc.add(*memPtr);
    ++memPtr;
  }
  uint8_t crcCalc = _crc.calc();
  if (crcCalc != *_pgatewayDataRecv) 
  {
    if (_chattyCathy)
    {
      Serial.print("LoRa CRC does not match. CRC Received: ");
      Serial.print(*_pgatewayDataRecv);
      Serial.print(", CRC expected: ");
      Serial.println(crcCalc);
    }
    return;
  }

  GatewayDataHeader* _pgatewayDataHeader = (GatewayDataHeader*)  _pgatewayDataRecv;
  if (_pgatewayDataHeader->igatewayAddr != _gatewayDataHeader.igatewayAddr) 
  {
    if (_chattyCathy)
    {
      Serial.println("LoRa Gateway address do not match. Addr Received: ");
      Serial.print(_pgatewayDataHeader->igatewayAddr);
      Serial.print(", Addr expected: ");
      Serial.println(_gatewayDataHeader.igatewayAddr);
    }
    return;
  }
  
  if (_pgatewayDataHeader->inodeAddr != _gatewayDataHeader.inodeAddr) 
  {
    if (_chattyCathy)
    {
      Serial.println("LoRa Node address do not match. Addr Received: ");
      Serial.print(_pgatewayDataHeader->inodeAddr);
      Serial.print(", Addr expected: ");
      Serial.println(_gatewayDataHeader.inodeAddr);
    }
    return;
  }
  if (_chattyCathy)
  {
    Serial.print("Node Receive: ");
    Serial.println(numBytes);
    Serial.print("icrc           : ");
    Serial.println(_pgatewayDataHeader->icrc);
  }
  _gatewayHasDataToRead = true;

}
boolean BlinkyLoraNodeClass::retrieveGatewayData(uint8_t* pnodeData)
{
  if (!_gatewayHasDataToRead) return false;
  uint8_t* memPtr = _pgatewayDataRecv + _sizeofGatewayDataHeader;
  for (int ii = 0; ii < _sizeofNodeData; ii++)
  {
    *pnodeData = *memPtr;
    ++memPtr;
    ++pnodeData;
  }
  _gatewayDataHeader.istate = 0;
  _gatewayHasDataToRead = false;
  return true;
}
void BlinkyLoraNodeClass::onLoRaReceive(int packetSize) 
{
  BlinkyLoraNode.receiveData(packetSize);
  return;
}
void BlinkyLoraNodeClass::onLoraTxDone() 
{
  if (BlinkyLoraNode._chattyCathy) Serial.println("TxDone");
  BlinkyLoraNodeClass::rxMode();
}
void BlinkyLoraNodeClass::onCadDone(bool signalDetected) 
{
  if (signalDetected)
  {
    delay(10);
    BlinkyLoraNode.beginSendingLoraData();
    return;
  }
  BlinkyLoraNode.finishSendingLoraData();
}


BlinkyLoraNodeClass BlinkyLoraNode(false);

void loop() 
{
}
void setup()
{
  setupLora();
}
void loop1() 
{
  loopNode();
}
void setup1()
{
  setupNode();
}
