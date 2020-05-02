/*
  Custom request header example for the ArduinoHttpStream
  library. This example sends a GET and a POST request with a custom header every 5 seconds.

  based on SimpleGet example by Tom Igoe
  header modifications by Todd Treece
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
  Serial.println("making GET request");
  client.beginRequest();
  client.get("/");
  client.sendHeader("X-CUSTOM-HEADER", "custom_value");
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("GET Status code: ");
  Serial.println(statusCode);
  Serial.print("GET Response: ");
  Serial.println(response);

  Serial.println("Wait five seconds");
  delay(5000);

  Serial.println("making POST request");
  String postData = "name=Alice&age=12";
  client.beginRequest();
  client.post("/");
  client.sendHeader(HTTP_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");
  client.sendHeader(HTTP_HEADER_CONTENT_LENGTH, postData.length());
  client.sendHeader("X-CUSTOM-HEADER", "custom_value");
  client.endRequest();
  client.write((const byte*)postData.c_str(), postData.length());
  // note: the above line can also be achieved with the simpler line below:
  //client.print(postData);

  // read the status code and body of the response
  statusCode = client.responseStatusCode();
  response = client.responseBody();

  Serial.print("POST Status code: ");
  Serial.println(statusCode);
  Serial.print("POST Response: ");
  Serial.println(response);

  Serial.println("Wait five seconds");
  delay(5000);
}
