TL_MQTT mqtt;

int port = 1883;
char serverName[]= "10.214.149.119";     // Server ip
char clientName[] = "3150105275";        // Device ID
char topicName[] = "clhcurry@wt";        // Topic name
char userName[] = "3150105275";          // Product ID
char password[] = "clhclh19971123";      // Authentication information

void setup() {
    TL_WiFi.init();
    bool b = TL_WiFi.join("EmNets-301","eagle402");
    mqtt = TL_WiFi.fetchMQTT();
    int a = mqtt.connect(serverName, port, clientName, userName, password);
    mqtt.subscribe("clhcurry@rt", messageArrived, 0);
}
void messageArrived(MQTT::MessageData& md){
    MQTT::Message &message = md.message;
    char res[20];
    strncpy(res, (char*)message.payload, 19);
    if(strncmp(res, "MOTOR_TURNON", strlen("MOTOR_TURNON")) == 0){
        TL_Motor.turnOn();
    }else if(strncmp(res, "MOTOR_TURNOFF", strlen("MOTOR_TURNOFF")) == 0){
        TL_Motor.turnOff();
    }
}
void loop() {
    TL_Temperature.read();
    String data = "{";
    data += "\"temperature\":";
    data += TL_Temperature.data();
    data += ",\"motor\":";
    data += TL_Motor.state();
    data += "}";
    char buf[100];
    data.toCharArray( buf,100 );
    int res = mqtt.publish(topicName, buf, strlen(buf));
    mqtt.yield(2000);
    TL_Time.delayMillis(1000);
}
