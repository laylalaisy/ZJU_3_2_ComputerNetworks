TL_MQTT mqtt;
int port = 1883;
char serverName[] = "10.214.149.119";
char clientName[] = "laylalaisy";
char topicName[] = "group1@wt";
char userName[] = "laylalaisy";
char password[] = "817189";

void setup() {
    TL_WiFi.init();
    bool b = TL_WiFi.join("EmNets-301","eagle402");
    mqtt = TL_WiFi.fetchMQTT();
    int a = mqtt.connect(serverName, port, clientName, userName, password);
}

void loop() {
    TL_Light.read();
    TL_Soil_Humidity.read();
    TL_Temperature.read();
    String data = "{";
    data += "\"Light\":";
    data += TL_Light.data();
    data += ",\"Soil_Humidity\":";
    data += TL_Soil_Humidity.data();
    data += ",\"Temperature\":";
    data += TL_Temperature.data();
    data += "}";
    char buf[100];
    data.toCharArray( buf,100 );
    int res = mqtt.publish(topicName, buf, strlen(buf));
    TL_Time.delayMillis(1000);
}