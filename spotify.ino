#include <math.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

#include <ESP8266WiFiMulti.h>

#include <WiFiClient.h>

#
define PI 3.14159265

const int n = 3; // bot number
const char server[] = "http://192.168.0.46:8081/data.txt";
const char * ssid = "ROBO_IITG_2.4G";
const char * password = "iamthefuture";
const float xmax = 460;
const float ymax = 620;
const float acc = 2.8; // deviation of position coordinate
const float range = 65; // range to prevent collision from other bot
const float margin = 5;
const int rdelay = 50; // rotation delay
const int fdelay = 50; // forward delay
const int baud = 74880;
const int recover = 100;

// Left Motor (A)
const int enA = D0;
const int in1 = D1;
const int in2 = D2;
// Right Motor (B)
const int enB = D5;
const int in3 = D3;
const int in4 = D4;

ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
WiFiClient client;

struct bot {
  float x; // x coordinate
  float y; // y coordinate
  float a; // angle
  float xt; // target x coordinate
  float yt; // target y coordinate
};

bot data[4]; // Recieved data

void setup() {
  // connect wifi
  Serial.begin(baud);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  // Declare motor control pins to be in output
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

}

// segregate the recieved data into bots
float split(const String & payload) {

  float a[20];

  for (int j = 0; j < 20; ++j) {

    int found = 0;
    int strIndex[] = {
      0,
      -1
    };
    int maxIndex = payload.length() - 1;

    for (int i = 0; i <= maxIndex && found <= j; i++) {
      char c = payload.charAt(i);
      if (c == '-' || c == ',' || c == ',' || i == maxIndex) {
        found++;
        strIndex[0] = strIndex[1] + 1;
        strIndex[1] = (i == maxIndex) ? i + 1 : i;
      }
    }

    String s = payload.substring(strIndex[0], strIndex[1]);
    if (s == "NA") {
      a[j] = -1;
    } else {
      a[j] = s.toFloat();
    }
  }
  // bot 0
  data[0].x = a[0];
  data[0].y = a[1];
  data[0].a = a[2];
  data[0].xt = a[12];
  data[0].yt = a[13];
  // bot 1
  data[1].x = a[3];
  data[1].y = a[4];
  data[1].a = a[5];
  data[1].xt = a[14];
  data[1].yt = a[15];
  // bot 2
  data[2].x = a[6];
  data[2].y = a[7];
  data[2].a = a[8];
  data[2].xt = a[16];
  data[2].yt = a[17];
  // bot 3
  data[3].x = a[9];
  data[3].y = a[10];
  data[3].a = a[11];
  data[3].xt = a[18];
  data[3].yt = a[19];
}

// Get current distance from target
float curds(int n = n) {
  getdata();
  float xt = data[n].xt, yt = data[n].yt, xc = data[n].x, yc = data[n].y;

  return sqrt((xt - xc) * (xt - xc) + (yt - yc) * (yt - yc));
}

// Get angle towards target
float tara(int n = n) {
  getdata();
  float xt = data[n].xt, yt = data[n].yt, xc = data[n].x, yc = data[n].y;

  // atan2(y, x) is atan2(y/x). Gives angle from -pi to pi
  return atan2((yt - yc), (xt - xc)) * 180 / PI + 180;
}

// check if another bot is in range, if then return angle
// else return -2
float inrange() {
  getdata();
  for (int i = 0; i < 4; ++i) {
    if (i == n) continue;

    float dist = curds(i);

    if (dist < range) {
      float bang = tara(i); //obstacle angle
      // if origin above
      bool oabove = ((data[i].y - data[n].y) * (data[n].x - 230) - (data[i].x - data[n].x) * (data[n].y - 310)) > 0;
      float ang;

      // Always turn bot towards origin
      if (oabove)
        ang = bang + 90;
      else
        ang = bang - 90;

      // ang 0-360
      if (ang > 360) ang -= 360;
      else if (ang < 0) ang += 360;

      return ang;
    }
  }

  return -2;
}

// Move forward
void forward(bool dir = 0) {

  Serial.print("\nForward");
  digitalWrite(in1, !dir);
  digitalWrite(in2, dir);

  digitalWrite(in3, dir);
  digitalWrite(in4, !dir);

  analogWrite(enA, 1024);
  analogWrite(enB, 1024);

  delay(fdelay);

  return;
}

// stop the bot
void stop() {
  Serial.print("\nStop");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  return;
}

// Rotate to x degree else to target angle by default
void rotate(float tara = tara()) {
  Serial.print("\nRotate");
  stop();
  getdata();
  bool dir = 0;
  if (abs(data[n].a - tara) > 180) dir = 1;

  while (abs(data[n].a - tara) > 20) {

    digitalWrite(in1, !dir);
    digitalWrite(in2, dir);

    digitalWrite(in3, !dir);
    digitalWrite(in4, dir);

    analogWrite(enA, 255);
    analogWrite(enB, 255);

    delay(rdelay); // Let it rotate a bit
    getdata();
  }

  return;
}

// get data from webserver
void getdata() {
  http.begin(client, server); //Specify request destination
  int httpCode = http.GET(); //Send the request
  Serial.printf("\n%d", httpCode);
  Serial.printf("%s\n", server);
  if (httpCode > 0) { //Check the returning 
    String payload = http.getString(); //Get the request response payload
    Serial.print(payload); //Print the response 
    split(payload);
  }
  http.end(); //Close

  return;
}

void start() {
  //    contain(); 
  getdata();
  // Check if other bots are in range
  while (inrange() > -1) {
    rotate(inrange());
    forward();
  }

  // Check if outbound
  while (data[n].x < 0) {
    forward(1);
    delay(recover);
    getdata();
  }

  rotate();
  forward();

  return;
}

void loop() {

  stop();

  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.print("\nWifiConnected\n");
    getdata();
  } else
    Serial.print("Unable to connect\n");

  // If previous bots finished
  while (1) {
    bool done = 1;
    for (int i = 0; i < n; ++i) {
      if (curds(i) > acc) {
        done = 0;
        break;
      }
    }
    if (done) break;
  }

  while (curds(n) > acc) {
    start();
  }

  stop();
}
