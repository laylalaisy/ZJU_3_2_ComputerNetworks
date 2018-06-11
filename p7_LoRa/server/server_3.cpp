
#define RH_RF95_MAX_MESSAGE_LEN 251
int expectID=0;

bool reliable_recv_from(){
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if(TL_LoRa.recv(buf, &len))
  {  
    TL_LoRa.setHeaderFlags(2);
    TL_LoRa.send(&buf[0], sizeof(buf[0]));
    if(expectID%256 != TL_LoRa.headerId()){
      return false;
    }
    expectID++;
    TL_Serial.println((char)buf[0]);
    return true;
  }
  else
  {
    return false;
  }
}

void setup() 
{    
  TL_Serial.begin(9600);
  if (!TL_LoRa.init())
    TL_Serial.println("init failed");  

  TL_LoRa.setThisAddress(12);
  TL_LoRa.setHeaderFrom(12);
  TL_LoRa.setHeaderTo(11);

  TL_LoRa.setTxPower(-1, true);
}

void loop()
{
  if (TL_LoRa.available())
  {
    //Should be a message for us now   
    if(reliable_recv_from())
    {
      TL_Serial.println("recv success");
    }
    else
    {
      TL_Serial.println("recv failed");
    }
  }
}


