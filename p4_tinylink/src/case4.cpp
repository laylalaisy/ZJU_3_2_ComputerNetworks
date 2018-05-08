String url1("http://10.214.149.119/tinylink/receiveData.php?userid=laylalaisy&nodeid=group1&temperature=");
String url2("&humidity=");
String url3("&pm25=");
TL_HTTP http;
void setup() {
    TL_WiFi.init();
    TL_WiFi.join("EmNets-301", "eagle402");
    http = TL_WiFi.fetchHTTP();
     TL_Serial.begin(9600);
}

void loop() {
    TL_Temperature.read();
    TL_Humidity.read();
    TL_PM25.read();
    http.get(url1 + String(TL_Temperature.data()) + url2 + String(TL_Humidity.data()) + url3 + String(TL_PM25.data()));
    TL_Time.delayMillis(1000);
}