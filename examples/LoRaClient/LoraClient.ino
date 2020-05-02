#include <ArduinoHttpStream.h>
#include <LoRa.h>

// Server: https://github.com/lasselukkari/aWOT/tree/master/examples/LoRaServer

#define SS 18
#define RST 14
#define DIO0 26
#define FREQUENCY 433E6

HttpStream client = HttpStream(LoRa);
char body[16];
int counter = 0;

void setup() {
  LoRa.setPins(SS, RST, DIO0);
  LoRa.begin(FREQUENCY);
}

void loop() {
  itoa(counter++, body, 10);

  LoRa.beginPacket();
  client.put("/counter", "text/plain", body);
  client.resetState();
  LoRa.endPacket();

  delay(1000);
}
