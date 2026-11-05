#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>
#include <esp_wifi.h>


//MIMICING GATEWAY esp32, COM3, Sender_node

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
    uint8_t id;
    uint32_t elapsed;
    float value;
} struct_message;


void peerSetup();
void commands();
void processingTask(void *pvParameters);

unsigned long lastSendTime = 0;
bool dataReceived = false;

QueueHandle_t rxQueue; 

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(struct_message)) {
        struct_message data_packet;

        memcpy(&data_packet, incomingData, sizeof(data_packet));
        dataReceived = true;

        volatile int lostpackets = 0;
        if(xQueueSend(rxQueue, &data_packet, 0) != pdTRUE){
            lostpackets++;
        }
    } 
    else {
        Serial.println("Received unknown data");
    }

}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup(){
    Serial.begin(115200);

    Serial.println("Gateway ready`");

    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }   

    rxQueue = xQueueCreate(50, sizeof(struct_message));

    if(rxQueue == NULL){
        Serial.println("Queue creation failed");
    }

    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);
   

    peerSetup();
    lastSendTime = millis();

    xTaskCreatePinnedToCore(
        processingTask,   
        "Processing Task", 
        4096,  // stack size
        NULL,             
        2,  // priority of the task
        NULL,             
        1   // core where the task should run
    );               

}

void loop(){
    commands();

}

void peerSetup(){
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    memcpy(peerInfo.peer_addr, broadcastAddress, 6); //every mac address is 6 bytes
    if(esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }
}
//commands to initalize syncrhonization
void commands(){

    if (Serial.available()){
        String cmd = Serial.readStringUntil('\n');
        cmd.trim(); //remove any leading/trailing whitespace

        if (cmd == "start") {
            uint8_t start_command = 1;
            esp_now_send(broadcastAddress, (uint8_t *) &start_command, sizeof(start_command));
            Serial.println("Sending start command");
            dataReceived = false; //reset data received flag
            lastSendTime = millis(); //reset last send time
        

        } else if (cmd == "s") {
            uint8_t stop_command = 2;
            esp_now_send(broadcastAddress, (uint8_t *) &stop_command, sizeof(stop_command));
            Serial.println("Sending stop command");

        } else {
            Serial.println("Unknown command.");
            return;
        }
    }

}

void processingTask(void *pvParameters){

    struct_message packet;

    while(true){
        if(xQueueReceive(rxQueue, &packet, portMAX_DELAY)){
            Serial.printf("Processing Packet: id=%d elapsed=%u value=%.2f\n", packet.id, packet.elapsed, packet.value);

            if(packet.id == 1){
                Serial.printf("front elapsed=%u value=%.2f\n", packet.elapsed, packet.value);
            }
            else if(packet.id == 2){
                Serial.printf("rear elapsed=%u value=%.2f\n", packet.elapsed, packet.value);
            }
            else{
                Serial.println("Unknown packet id");
            }
        
        }
        
    }
}

