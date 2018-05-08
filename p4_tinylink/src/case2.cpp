void setup() {
    TL_Serial.begin(9600);
}

void loop() {
    TL_Temperature.read();
    TL_Serial.print("Temperature data is ");
    TL_Serial.println(TL_Temperature.data());
    TL_Time.delayMillis(1000);
}