/*
  Dweet.io POST client for ArduinoHttpStream library
  Connects to dweet.io once every ten seconds,
  sends a POST request and a request body.

  Shows how to use Strings to assemble path and body

  created 15 Feb 2016
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
  // assemble the path for the POST message:
  String dweetName = "scandalous-cheese-hoarder";
  String path = "/dweet/for/" + dweetName;
  String contentType = "application/json";

  // assemble the body of the POST message:
  int sensorValue = analogRead(A0);
  String postData = "{\"sensorValue\":\""; 
  postData += sensorValue;
  postData += "\"}";

  Serial.println("making POST request");

  // send the POST request
  client.post(path, contentType, postData);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  Serial.println("Wait ten seconds\n");
  delay(10000);
}
