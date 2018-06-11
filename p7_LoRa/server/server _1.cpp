#define RH_RF95_MAX_MESSAGE_LEN 251
int count = 0;

void setup() 
{    
  TL_Serial.begin(9600);
  if (!TL_LoRa.init())
  {
    TL_Serial.println("init failed");  
  }
  TL_LoRa.setThisAddress(12);
  
  TL_LoRa.setTxPower(-1, true);
}

void loop()
{
  if (TL_LoRa.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (TL_LoRa.recv(buf, &len))
    {
      count = count + 1;
      for (int i = 0; i < len; i++)
      {
         TL_Serial.println((char)buf[i]);
         TL_Serial.println(TL_LoRa.lastRssi()); 
         TL_Serial.println(count);
      }  
    }
    else
    {
      TL_Serial.println("recv failed");
    }
  }
}
