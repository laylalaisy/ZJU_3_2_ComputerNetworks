#define DATA_LENGTH 500
uint8_t data[DATA_LENGTH];
unsigned long beginTime,endTime;

void setup() 
{
TL_Serial.begin(9600);
  if (!TL_LoRa.init())
  {
    TL_Serial.println("init failed");
  }
  TL_LoRa.setThisAddress(11);
  TL_LoRa.setHeaderFrom(11);
  TL_LoRa.setHeaderTo(12);
  TL_Serial.println("Sending to server");
  TL_LoRa.setTxPower(-1, true);
  for (int i = 0; i < DATA_LENGTH; i++) 
  {
    data[i] = i%10 + '0'; 
  }
    //dataLength表示数据的长度
  for (uint8_t dataLength = 10; dataLength <= 200; dataLength+=20) 
  {
    beginTime= TL_Time.millisFromStart();
    TL_LoRa.send(&data[0],dataLength);
    TL_LoRa.waitPacketSent();
    endTime=TL_Time.millisFromStart();
    unsigned long du=endTime-beginTime;
    TL_Serial.println(du);
  }
  TL_Serial.println("send over!");
}

void loop() 
{
}
