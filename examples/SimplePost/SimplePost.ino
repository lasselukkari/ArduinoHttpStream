/*
  Simple POST client for ArduinoHttpStream library
  Connects to server once every five seconds, sends a POST request
  and a request body

  created 14 Feb 2016
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
  Serial.println("making POST request");
  String contentType = "application/x-www-form-urlencoded";
  String postData = "name=Alice&age=12";

  client.post("/", contentType, postData);

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
