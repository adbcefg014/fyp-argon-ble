/*
 * Project ble
 * Description:
 * Author:
 * Date:
 */

// BLE Demonstration
#include <Particle.h>
SYSTEM_MODE(MANUAL)
SYSTEM_THREAD(ENABLED)
#define PERIPHERAL

const BleUuid serviceUuid("6057ffae-9efd-11ec-b909-0242ac120002");
const BleUuid rxUuid("6058033c-9efd-11ec-b909-0242ac120002");
const BleUuid txUuid("6058054e-9efd-11ec-b909-0242ac120002");

const size_t UART_TX_BUF_SIZE = 64;
const unsigned long MS_PERIOD = 2000;

struct customData {
uint16_t companyID;
int16_t data;
};

uint8_t txBuf[UART_TX_BUF_SIZE];

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

#if defined(CENTRAL)
const size_t SCAN_RESULT_COUNT = 10;

bool canDisconnect = false;

BleScanResult scanResults[SCAN_RESULT_COUNT];

BleCharacteristic peerTxCharacteristic;
BleCharacteristic peerRxCharacteristic;
BlePeerDevice peer;

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  BLE.on();
  BLE.setScanPhy(BlePhy::BLE_PHYS_AUTO);
  peerTxCharacteristic.onDataReceived(onDataReceived, &peerTxCharacteristic);
  BLE.selectAntenna(BleAntennaType::EXTERNAL);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  static uint32_t msLast = 0;
  if (millis() - msLast < MS_PERIOD) return;
  msLast = millis();

  size_t count = BLE.scanWithFilter(BleScanFilter().serviceUUID(serviceUuid), scanResults, SCAN_RESULT_COUNT);
  Serial.printlnf("Found %d device(s) exposing service %s", count, (const char*)serviceUuid.toString());
  for (uint8_t ii = 0; ii < count; ii++) {
    customData cd;
    scanResults[ii].advertisingData().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, (uint8_t*)&cd, sizeof(cd));
    Serial.printlnf("Device %s advertises %04X: %u", (const char*)scanResults[ii].address().toString(), cd.companyID, cd.data);
    peer = BLE.connect(scanResults[ii].address());
    if (peer.connected()) {
      peer.getCharacteristicByUUID(peerTxCharacteristic, txUuid);
      //peerTxCharacteristic.getValue(txBuf, sizeof(txBuf));
      //Serial.printlnf("txChar: %s\r\n", (char*)txBuf);
      while(!canDisconnect) Particle.process();
      peer.disconnect();
      canDisconnect = false;;
    }
  }
  Serial.println();
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  Serial.printf("receive data from %s: ", (const char*)peer.address().toString());
  for (size_t ii = 0; ii < len; ii++) {
    Serial.write(data[ii]);
  }
  Serial.println();
  canDisconnect = true;
}


#elif defined(PERIPHERAL)

void setAdvertisingData();
BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

void setup() {
  BLE.on();
  BLE.addCharacteristic(rxCharacteristic);
  BLE.addCharacteristic(txCharacteristic);
  BLE.setTxPower(8);
}

void loop() {
  static uint32_t msLast = 0;
  if (millis() - msLast < MS_PERIOD) return;
  msLast = millis();
  float temp = random(100) / 10.0;
  setAdvertisingData(temp * 10.0);
  snprintf((char*)txBuf, sizeof(txBuf), "{ \"temp\":\"%.1f °C\" }", temp);
  //if (BLE.connected())
  txCharacteristic.setValue(txBuf, strlen((char*)txBuf)+1);
}

void setAdvertisingData(int16_t data) {
  customData cd;
  BleAdvertisingData advData;
  cd.companyID = 0xFFFF;
  cd.data = data;
  advData.appendLocalName("BT");
  advData.appendCustomData((uint8_t*)&cd, sizeof(cd));
  advData.appendServiceUUID(serviceUuid);
  BLE.advertise(&advData);
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  for (size_t ii = 0; ii < len; ii++) {
    Serial.write(data[ii]);
  }
}
#endif