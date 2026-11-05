#include <Arduino.h>
#include <CAN.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

//need to change this macaddress to the reciever mac address
uint8_t nextHopAddress[] = {0x08, 0x3A, 0xF2, 0x30, 0xC6, 0x48}; 
esp_now_peer_info_t peerInfo = {};

// CAN pins
#define TX_GPIO_NUM 5
#define RX_GPIO_NUM 4

struct SensorData {
    int16_t ax;
    int16_t ay;
    int16_t az;
    uint32_t timestamp;
};

struct WirelessPacket {
    uint8_t tireID;      // 1=front, 2=rear
    int16_t ax;
    int16_t ay;
    int16_t az;
    uint32_t timestamp;
};

//callback function for sending
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send OK" : "Send Failed");

}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("CAN Receiver");

    CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
    if (!CAN.begin(500E3)) {
        Serial.println("CAN failed!");
        while (1);
    }

    //setting esp as a station
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 

    //initalizing esp-now
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW initilization failed");
        return;
    }
    // Register send callback
    esp_now_register_send_cb(OnDataSent);

    // register the gateway as a peer
    memcpy(peerInfo.peer_addr, nextHopAddress, 6);  // Set MAC
    peerInfo.channel = 0;     // Use 0 to match current Wi-Fi channel
    peerInfo.encrypt = false;                 

    // add the peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    Serial.print("Gateway MAC: ");
    Serial.println(WiFi.macAddress());
}

void loop() {
    //check if packet received
    int packetSize = CAN.parsePacket();

    if (packetSize == sizeof(SensorData)) {
        SensorData receivedData;
    
        CAN.readBytes((uint8_t*)&receivedData, sizeof(receivedData));

        WirelessPacket packet;

        //identify sender
        if (CAN.packetId() == 0x101) {
            packet.tireID = 1; // front tire
        } 
        else if (CAN.packetId()  == 0x102) {
            packet.tireID = 2; // rear tire
        } 
        else {
            Serial.print("Error: Unknown CAN ID");
            return; 
        } 

        packet.ax = receivedData.ax;
        packet.ay = receivedData.ay;
        packet.az = receivedData.az;
        packet.timestamp = receivedData.timestamp;

    
        esp_err_t result = esp_now_send(nextHopAddress, (uint8_t *) &packet, sizeof(packet));

        if (result == ESP_OK){
            Serial.println("ESP-NOW Packet sent");
        } else{
            Serial.println("ESP-NOW Packet failed");
        }  
    }
}