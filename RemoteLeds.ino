#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <Crc.h>

#define NUM_LEDS 1
#define DATA_PIN 6


const char* Ssid = "Home_33"; // My WiFi-network name
const char* Password =  "2012100901"; // My WiFi-network password
const int Port = 12100;  // Port to listen incoming connectins

WiFiServer Server(Port);
WiFiClient Client;
bool Connected = false;

CRGB Leds[NUM_LEDS];


enum Command_t : uint8_t
{
	GET_LEDS,
	SET_LEDS,
};


enum Result_t : uint8_t
{
	RESULT_OK,
	RESULT_ERROR,
	RESULT_UNKNOWN_CMD,
};






const uint16_t PacketLenMax = 128;
char PacketData[PacketLenMax];

uint32_t PacketPreample;
uint16_t PacketLen;
uint16_t PacketCrc;
int Step;
int PixelIndex;
int ColorIndex;
int Index;

void setup() 
{
  // put your setup code here, to run once:

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(Leds, NUM_LEDS);

  Serial.begin(115200); // opens serial port, sets data rate to 115200 bps

  delay(2000);
 
  Serial.println();

  WiFi.begin(Ssid, Password);

  Serial.print("Connecting to: ");
  Serial.print(Ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // begin listening for incoming connections
  Server.begin();
}


void ResetProcessData()
{
    Step = 0;
    PacketPreample = 0;
}




void ProcessData(char chr)
{
    switch(Step)
    {
        case 0: // Wait preample
        {
            PacketPreample >>= 8;
            PacketPreample |= ((uint32_t)chr << 24);

            
            if (PacketPreample == 0xAABBCCDD)
            {
            	Serial.print("Preample: ");
            	Serial.println(PacketPreample);
            	PacketLen = 0;
                Index = 0;
                Step++;
            }
            break;
        }

		case 1:  // Receive packet length field
		{
			PacketLen >>= 8;
			PacketLen |= ((uint16_t)chr << 8);
			if (++Index >= sizeof(PacketLen))
			{
				if ((PacketLen > 0) && (PacketLen <= PacketLenMax))
				{
					Serial.print("PacketLen: ");
            		Serial.println(PacketLen);
					Index = 0;
					Step++;
				}
				else
				{
					ResetProcessData();
				}
			}
			break;
		}

		case 2: // Receive packet data
		{
			PacketData[Index] = chr;
			if (++Index >= PacketLen)
			{
				Serial.println("Packet received");
				Index = 0;
				PacketCrc = 0;
				Step++;
			}
			break;
		}

		case 3: // Receive packet CRC
		{
			PacketCrc >>= 8;
			PacketCrc |= ((uint16_t)chr << 8);
			if (++Index >= sizeof(PacketCrc))
			{
				Serial.print("PacketCrc: ");
            	Serial.println(PacketCrc);
				// Check packet CRC
				//uint16_t crc = CRC::CRC16(&PacketLen, sizeof(PacketLen), 0);
				uint16_t crc = CRC::CRC16(PacketData, PacketLen, 0);
				if (crc == PacketCrc)
				{
					ProcessCommand();
				}
				ResetProcessData();					
			}		
			break;
			
		}        
    }
}



void ProcessCommand()
{
//	if (PacketLen < sizeof(Command_t))
//	{
//		return;
//	}

	char* data = PacketData;	
	Command_t cmd = (Command_t)*data++;
	//int dataLen = PackenLen - sizeof(cmd);
	
	switch(cmd)
	{
//		case GET_LEDS:
//		{
//			if ((sizeof(Command_t) + sizeof(Result_t) + sizeof(Leds)) <= PacketLenMax)
//			{
//				*data++ = RESULT_OK;
//				memcpy(data, &Leds, sizeof(Leds));
//				data += sizeof(Leds);
//			}
//			else
//			{
//				*data++ = RESULT_ERROR;
//			}
//
//			break;
//		}

		case SET_LEDS:
		{
			uint16_t startLed;
			uint16_t stopLed;
			CRGB color;
			if (PacketLen == (sizeof(Command_t) + sizeof(startLed) + sizeof(stopLed) + sizeof(color)))
			{
				memcpy(&startLed, data, sizeof(startLed));
				data += sizeof(startLed);

				memcpy(&stopLed, data, sizeof(stopLed));
				data += sizeof(stopLed);

				CRGB color;
				memcpy(&color, data, sizeof(color));

				Serial.print("startLed: ");
            	Serial.println(startLed);

            	Serial.print("color: ");
            	Serial.println(color);

            	Serial.print("stopLed: ");
            	Serial.println(stopLed);

            	Serial.print("Color received: red: ");
                Serial.print(color.red);
                Serial.print(" green: ");
                Serial.print(color.green);
                Serial.print(" blue: ");
                Serial.println(color.blue);

				for (int led = startLed; led <= stopLed; led++)
				{
					Leds[led] = color;
				}
				FastLED.show();
			}

			break;
		}

		default:
		{
			//*data++ = RESULT_UNKNOWN_CMD;
			break;
		}
	}

//	PacketLen = data - PacketData;
//	SendAnswer();	
}


//void SendAnswer()
//{
//	char* preamle = (char* )&PacketPreample;
//	for (int n = 0; n < sizeof(PacketPreample); n++)
//	{
//		Client.write(preamle[n]);
//	}
//
//	char* len = (char* )&PacketLen;
//	for (int n = 0; n < sizeof(PacketLen); n++)
//	{
//		Client.write(len[n]);
//	}
//
//	for (int n = 0; n < PacketLen; n++)
//	{
//		Client.write(PacketData[n]);
//	}
//
//	uint16_t packetCrc = CRC::CRC16(&PacketPreample, sizeof(PacketPreample), 0);
//	packetCrc = CRC::CRC16(&PacketLen, sizeof(PacketLen), packetCrc);
//	packetCrc = CRC::CRC16(PacketData, PacketLen, packetCrc);
//
//	char* crc = (char* )&packetCrc;
//	for (int n = 0; n < sizeof(packetCrc); n++)
//	{
//		Client.write(crc[n]);
//	}	
//}





void loop() 
{
	// put your main code here, to run repeatedly:

  	WiFiClient client = Server.available();
	if (client)
	{
		if (!Connected)
		{
			Connected = true;
			Client = client;
			Serial.println("Client connected");
			ResetProcessData();
		}
		else
		{
			Serial.println("Client already connected");
			client.stop();
		}
	}

	if (Connected)
	{
		if (Client.connected())
		{
			 while (Client.available() > 0)
			 {
			 	char chr = Client.read();
			 	//Serial.write(chr);
			 	ProcessData(chr); 
			 }
		}
		else
		{
			Client.stop();
			Connected = false;
			Serial.println("Client disconnected");
		}
	}
}
