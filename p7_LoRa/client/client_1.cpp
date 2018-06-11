
#define DATA_LENGTH 500
uint8_t data[DATA_LENGTH];

void setup() 
{
  TL_Serial.begin(9600);
  if (!TL_LoRa.init())
    TL_Serial.println("init failed");

  TL_Serial.println("Sending to server");

  TL_LoRa.setTxPower(-1, true);

  for (int i = 0; i < DATA_LENGTH; i++)
  {
    data[i] = i%10 + '0'; 
  }
  for (int i = 0; i < DATA_LENGTH; i++)
  {
    TL_LoRa.send(&data[i], sizeof(data[i]));
    TL_LoRa.waitPacketSent();
  }
}

void loop()
{
}


