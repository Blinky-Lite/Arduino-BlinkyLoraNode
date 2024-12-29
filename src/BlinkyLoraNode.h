#ifndef BlinkyLoraNode_h
#define BlinkyLoraNode_h
#include "Arduino.h"
#include <CRC8.h>
#include <SPI.h>
#include <LoRa.h>
 
struct GatewayDataHeader
{
  uint8_t icrc;
  uint8_t istate;
  int16_t inodeAddr;
  int16_t igatewayAddr;
  int16_t iwatchdog;
  int16_t iforceArchive;  
  int16_t irssi;  
  int16_t isnr;  
}; 
void loop();
void loop1();
void setup();
void setup1();
void setupLora();
void loopNode();
void setupNode();

class BlinkyLoraNodeClass
{
  private: 
    GatewayDataHeader    _gatewayDataHeader;
    CRC8                _crc;
    boolean             _chattyCathy = false;
    size_t              _sizeofNodeData;
    size_t              _sizeofGatewayDataHeader;
    size_t              _sizeOfTransferData;
    uint8_t*            _pnodeDataSend = nullptr;
    uint8_t*            _pgatewayDataRecv = nullptr;
    volatile boolean    _gatewayHasDataToRead = false;
    volatile boolean    _nodeHasDataToRead = false;
    boolean             _waitingForCad = false;
    void                receiveData(int packetSize);

  public:
    BlinkyLoraNodeClass(boolean chattyCathy);
    void            begin(size_t nodeDataSize, boolean chattyCathy, int16_t nodeAddress, int16_t gateWayAddress, int loraChipSelectPin, int loraResetPin, int loraIRQPin, long loraFreq, int loraSpreadingFactor, long loraSignalBandwidth);
    boolean         retrieveGatewayData(uint8_t* ploraData);
    boolean         publishNodeData(uint8_t* ploraData, boolean forceArchiveData);
    static void     onLoraTxDone();
    static void     onLoRaReceive(int packetSize);
    static void     rxMode();
    static void     txMode();
    static void     onCadDone(bool signalDetected);
    void            beginSendingLoraData();
    void            finishSendingLoraData();
    boolean         publishNodeDataInProgress(){return _nodeHasDataToRead;}
        
};
extern BlinkyLoraNodeClass BlinkyLoraNode;
#endif
