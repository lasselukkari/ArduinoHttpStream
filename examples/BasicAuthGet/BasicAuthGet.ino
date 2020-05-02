/*
  GET client with HTTP basic authentication for ArduinoHttpStream library
  Connects to server once every five seconds, sends a GET request

  created 14 Feb 2016
  by Tom Igoe
  modified 3 Jan 2017 to add HTTP basic authentication
  by Sandeep Mistry
  modified 22 Jan 2019
  by Tom Igoe
  
  this example is in the public domain
 */
#include <ArduinoHttpStream.h>

HttpStream client = HttpStream(Serial);

void setup() {
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  Serial.println("making GET request with HTTP basic authentication");
  client.beginRequest();
  client.get("/secure");
  client.sendBasicAuth("username", "password"); // send the username and password for authentication
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("Wait five seconds");
  delay(5000);
}
