void setup() {
  TL_Bluetooth.init("3100000000");
}

void loop() {
  char buf[20];
  if(TL_Bluetooth.recv(buf) == 0){
    if(strcmp(buf, "on") == 0){
      TL_LED.turnOn();
      TL_Bluetooth.send("Light On");
    }else if(strcmp(buf, "off") == 0){
      TL_LED.turnOff();
      TL_Bluetooth.send("Light Off");
    }
  }
  TL_Time.delayMillis(1000);
}