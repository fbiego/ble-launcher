/*
   MIT License
  Copyright (c) 2022 Felix Biego
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  ______________  _____
  ___  __/___  /_ ___(_)_____ _______ _______
  __  /_  __  __ \__  / _  _ \__  __ `/_  __ \
  _  __/  _  /_/ /_  /  /  __/_  /_/ / / /_/ /
  /_/     /_.___/ /_/   \___/ _\__, /  \____/
                              /____/
*/

#include <ESP32Time.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "fb1e4001-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_RX "fb1e4002-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_TX "fb1e4003-54ae-4a28-9f74-dfccb248601d"
static BLECharacteristic *pCharacteristicTX;
static BLECharacteristic *pCharacteristicRX;

ESP32Time rtc;

static bool deviceConnected = false;
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    pServer->startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{

  void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code)
  {
    Serial.print("Status ");
    Serial.print(s);
    Serial.print(" on characteristic ");
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(" with code ");
    Serial.println(code);
  }

  void onNotify(BLECharacteristic *pCharacteristic)
  {
    uint8_t *pData;
    std::string value = pCharacteristic->getValue();
    int len = value.length();
    pData = pCharacteristic->getData();
    if (pData != NULL)
    {
      Serial.print("Notify callback for characteristic ");
      Serial.print(pCharacteristic->getUUID().toString().c_str());
      Serial.print(" of data length ");
      Serial.println(len);
      Serial.print("TX  ");
      for (int i = 0; i < len; i++)
      {
        Serial.printf("%02X ", pData[i]);
      }
      Serial.println();
    }
  }

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    uint8_t *pData;
    std::string value = pCharacteristic->getValue();
    int len = value.length();
    pData = pCharacteristic->getData();
    if (pData != NULL)
    {
      Serial.print("Write callback for characteristic ");
      Serial.print(pCharacteristic->getUUID().toString().c_str());
      Serial.print(" of data length ");
      Serial.println(len);
      Serial.print("RX  ");
      for (int i = 0; i < len; i++)
      {
        Serial.printf("%02X ", pData[i]);
      }
      Serial.println();

      if (pData[0] == 0xAB)
      {
        switch (pData[4])
        {
        case 0x93:
          rtc.setTime(pData[13], pData[12], pData[11], pData[10], pData[9], pData[7] * 256 + pData[8]);
          break;
          
        }
      }
      
    }
  }
};

void initBLE()
{
  BLEDevice::init("Card");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicTX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristicRX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristicRX->setCallbacks(new MyCallbacks());
  pCharacteristicTX->setCallbacks(new MyCallbacks());
  pCharacteristicTX->addDescriptor(new BLE2902());
  pCharacteristicTX->setNotifyProperty(true);
  pService->start();

  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}


enum Action {
  VIEW = 0xA0,
  DIAL
};

void launchIntent(Action a, String intent){
  char s1 = a;
  String result = String(s1) + intent;
  pCharacteristicTX->setValue(result.c_str());
  pCharacteristicTX->notify();
  delay(200);
}

void setup()
{
  Serial.begin(115200);
  
  
  initBLE();


}


void loop(){

    // call the launchIntent function after BLE is connected to launch the correspinding intent on the connected phone
    // launchIntent(DIAL, "tel:+25470*****06");
    // launchIntent(VIEW, "https://felix.fbiego.com/");
    // launchIntent(VIEW, "https://twitter.com/fbiego_");

}