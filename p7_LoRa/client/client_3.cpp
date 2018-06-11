#define DATA_LENGTH 500
#define RH_RF95_MAX_MESSAGE_LEN 251
uint8_t data[DATA_LENGTH];
int timeout = 3000;
int id=0;
unsigned long beginTime,endTime;
bool reliable_send_to(uint8_t* data, uint8_t len);
bool reliable_send_to(uint8_t* data, uint8_t len) {
	TL_LoRa.setHeaderId(id);
	TL_LoRa.send(data, len);
	TL_LoRa.waitPacketSent();
	beginTime= TL_Time.millisFromStart();
	uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t rev_len = sizeof(buf);
    while (true){
    	if(TL_LoRa.available()){
	    	if(TL_LoRa.recv(buf, &rev_len)){
	    		if(TL_LoRa.headerFlags()==2){
		    		endTime=TL_Time.millisFromStart();
					unsigned long du=endTime-beginTime;
					TL_Serial.println(du);
					break;
		    	}
	    	}
	    }
	    if(TL_Time.millisFromStart() - beginTime >timeout){	
	    	return false;
	    }
    }
    id++;
    return true;
}

void setup() 
{
	TL_Serial.begin(9600);
	if (!TL_LoRa.init()){
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
	for (int i = 0; i < DATA_LENGTH; i++)
	{
		while(!reliable_send_to(&data[i], sizeof(data[i])));
	}
	TL_Serial.println("send over!");
}

void loop()
{
}

