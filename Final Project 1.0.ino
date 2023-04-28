#include <Servo.h>                     // Servo Library
#include <HCSR04.h>                    // Ultrasonic sensor library
#include <ESP8266WiFi.h>               // Load Wi-Fi library
#define TWELVEHOURS 30                 //(43200) In seconds
const char* ssid = "Foster";           //WiFi name
const char* password = "Fuckthis1!2";  //WiFi password
WiFiServer server(80);                 // Set web server port number to 80
String header;                         // Variable to store the HTTP request
unsigned long currentTime = millis();  // Current time
unsigned long previousTime = 0;        // Previous time
const long timeoutTime = 500;          // Define timeout time in milliseconds (example: 2000ms = 2s)
unsigned long timer = 0, timeStamp = 0;
bool autoFeed = false;      //Toggleable bool for autoFeed
String feedNow = "Off";     //Toggleable string for feedNow
UltraSonicDistanceSensor HC_SR04(4, 5);                        // UltraSonic sensor with pins (Trig, echo)
Servo servo;                                                   // Creates servo object
String tankLevel = "Empty";

void setup() {
  servo.attach(16);                        // Initialize servo with pins
  servo.write(0);                          //Initial servo position
  Serial.begin(115200);                    //Begins serial with 115200 Baud Rate
  Serial.print("Connecting to ");          // Connect to Wi-Fi network with SSID and password
  Serial.println(ssid);                    //Prints for Wifi setup
  WiFi.begin(ssid, password);              //Wifi Connection
  while (WiFi.status() != WL_CONNECTED) {  //prints ... while connecting
    delay(500);
    Serial.print(".");
  }
  Serial.println("");                 // Print local IP address and start web server
  Serial.println("WiFi connected.");  //Prints once connected
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  //Prints IP to connect to
  server.begin();                  //Begins server
}

void loop() {
  if (autoFeed == true)  //If timer is enabled, begins countdown
    timer = TWELVEHOURS - ((millis() - timeStamp) / 1000);
  if ((timer == 0) && (autoFeed == true)) {  //If timer ends, resets countdown
    feed();                                  //Calls function to feed
    resetTimer();                            //Calls function to reset time
  }
  if (feedNow == "On") {
    feed();
    feedNow = "Off";
    Serial.println("Resetting Feed Button");
  }
  handleClient();  //Function to check for signals
}

void handleClient() {  //Function for reciving signals
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client.");  // print a message out in the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline charactedugrs in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            // turns the GPIOs On and Off
            if (header.indexOf("GET /F/On") >= 0) {          //If head input for feed is on
              feedNow = "On";                                //Changes value for feedNow for client.print
            } else if (header.indexOf("GET /F/Off") >= 0) {  //If head input for feed is off
              feedNow = "Off";
            } else if (header.indexOf("GET /t/On") >= 0) {  //If head input for time is on
              timeStamp = millis();
              autoFeed = true;                               //Changes autoFeed bool to true
              resetTimer();                                  //Calls function to start timer
            } else if (header.indexOf("GET /t/Off") >= 0) {  //If head input for timer is off
              autoFeed = false;                              //Changes autoFeed bool to false
              timer = 0;                                     //Sets timer to 0
            }
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the On/Off buttons
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #850687; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #387910;}.button3 {background-color: #7E7979;}</style></head>");
            client.println("<body><h1>Automatic Pet Feeder</h1>");  // Web Page Heading
            client.println("<body><h2>Current Tank Level: " + tankLevel + "<h2>");
            client.println("<p>Feed Now </p>");
            // Display current state
            if (feedNow == "Off") {  // If the feedNow is Off, it displays the On button
              client.println("<p><a href=\"/F/On\"><button class=\"button\">Off</button></a></p>");
            } else {
              client.println("<p><a href=\"/F/Off\"><button class=\"button button2\">On</button></a></p>");
            }
            char msg[9];
            sprintf(msg, "%02i:%02i:%02i", timer / 3600, (timer % 3600) / 60, timer % 60);
            // Display current state, and On/Off buttons for GPIO 4
            //client.println("<p>Timer " + String(timer/60) + ":" +String(timer%60) + "</p>");
            client.println("<p>Feeding Timer " + String(msg) + "</p>");
            if (timer != 0) {  //If timer is enabled, prints time
              client.println("<p><a href=\"/t/Off\"><button class=\"button button2\">Running</button></a></p>");
            } else {  //Else prints "12 Hours" and displays as off
              client.println("<p><a href=\"/t/On\"><button class=\"button\">12 Hours</button></a></p>");
            }
            client.println("<p> </p>");
            client.println("<p><a href=\"/r/1\"><button class=\"button button3\">Refresh</button></a></p>");
            client.println("</body></html>");
            client.println();  // The HTTP response ends with another blank line
            break;             // Break out of the while loop
          } else {             // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    //Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void resetTimer() {  //Function to Reset time
  timer = TWELVEHOURS;
  timeStamp = millis();
  delay(1000);
  Serial.println("Timer Reset");
}

void feed() {  //Function to feed
  servo.write(180);
  delay(2000);
  servo.write(0);
  Serial.println("Feeding Now");
  checkTankLevel();  //Function to check the current fullness of tank
}

void checkTankLevel() {
  int distance = HC_SR04.measureDistanceCm(); //Creates int for distance to be measured by Ultrasonic sensor
  if (distance > -1) {                         // If value is read
    Serial.println(distance);
    int tankDistance;     
    if (distance < 5) {                       // Value for full
      tankDistance = 0;
    } else if (distance >= 5 && distance <= 6) {  // Value for low
      tankDistance = 1;
    } else {  // Else empty
      tankDistance = 2;
    }
    switch (tankDistance) {  
      case 0:
        tankLevel = "Full";
        Serial.println(tankLevel);
        break;
      case 1:
        tankLevel = "Low";
        Serial.println(tankLevel);
        break;
      case 2:
        tankLevel = "Empty";
        Serial.println(tankLevel);
        break;
      default:  //Redundancy
        tankLevel = "Error";
        Serial.println(tankLevel);
        break;
    }
  }
}