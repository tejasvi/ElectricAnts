#include <math.h> 
#include <esp8266wifi.h>
#include <esp8266httpclient.h>

#define PI 3.14159265

const float xmax = 200;
const float ymax = 100;
const float acc = 5; // deviation of position coordinate
const float range = 10; // range to prevent collision from other bot
const int n = 0; // bot number
const int rdelay = 10; // rotation delay
const int fdelay = 10; // forward delay
const int baud = 115200
const 

// Left Motor (A)
const int enA = 16;
const int in1 = 5;
const int in2 = 4;
// Right Motor (B)
const int enB = 14;
const int in3 = 0;
const int in4 = 22;

const char* ssid = "Wifi name";
const char* password = "password";
HTTPClient http;

class bot {
    float x; // x coordinate
    float y; // y coordinate
    float a; // angle
    float xt; // target x coordinate
    float yt; // target y coordinate
};

bot data[4]; // Recieved data

// connect to wifi
void connect() {
    Serial.begin(baud);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print("Connecting..");
    }
}

void setup() {
    // connect wifi
    connect();

    // Declare motor control pins to be in output
    pinMode(enA, OUTPUT);
    pinMode(enB, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(in3, OUTPUT);
    pinMode(in4, OUTPUT);

}

// segregate the recieved data into bots
float split(const String &payload) {
    
    float a[20];
    
    for (int j=0; j<20; ++j) {
        
        int found = 0;
        int strIndex[] = { 0, -1 };
        int maxIndex = payload.length() - 1;
    
        for (int i = 0; i <= maxIndex && found <= j; i++) {
            char c = payload.charAt(i);
            if (c == '-' || c == ',' || c == ',' || i == maxIndex) {
                found++;
                strIndex[0] = strIndex[1] + 1;
                strIndex[1] = (i == maxIndex) ? i+1 : i;
            }
        }
        
        a[j] = data.substring(strIndex[0], strIndex[1]).toFloat();
    }
    // bot 0
    data[0].x = x[0];
    data[0].y = x[1];
    data[0].a = x[2];
    data[0].xt = x[12];
    data[0].yt = x[13];
    // bot 1
    data[1].x = x[3];
    data[1].y = x[4];
    data[1].a = x[5];
    data[1].xt = x[14];
    data[1].yt = x[15];
    // bot 2
    data[2].x = x[6];
    data[2].y = x[7];
    data[2].a = x[8];
    data[2].xt = x[16];
    data[2].yt = x[17];
    // bot 3
    data[3].x = x[9];
    data[3].y = x[10];
    data[3].a = x[11];
    data[3].xt = x[18];
    data[3].yt = x[19];
}

// Get current distance from target
float curds(int n=n) {
    getdata();
    float xt=data[n].xt, yt=data[n].yt, xc=data[n].x, yc=data[n].y;

    return  sqrt((xt-xc)^2 + (yt-yc)^2);
}


// Get angle towards target
float tara(int n=n) {
    getdata();
    float xt=data[n].xt, yt=data[n].yt, xc=data[n].x, yc=data[n].y;

    // atan2(y, x) is atan2(y/x). Gives angle from -pi to pi
    return atan2((yt-yc), (xt-xc)) * 180 / PI + 180;
}

// check if another bot is in range, if then return angle
// else return -2
float inrange() {
    getdata();
    for (int i=0; i<4; ++i) {
        if (i == n) continue;
        
        dist = curds(i);
        
        if (dist < range) {
            bang = tara(i); //obstacle angle
            // if origin above
            bool oabove = ((data[i].y - data[0].y)*data[0].x - (data[i].x - data[0].x)*data[0].y) > 0;
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


// Move forward by x amount else by default amount
void forward(int x = 10) {

    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);

    analogWrite(enA, 255);
    analogWrite(enB, 255);

    delay(fdelay);
    
    return;
}


// stop the bot
void stop(){
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);  
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW); 
    return;
}

// Rotate to x degree else to target angle by default
void rotate(float tara=tara()) {
    stop();
    getdata();
    bool dir=0;
    if (abs(data[0].a - tara) > 180) dir = 1;
    
    while(abs(data[0].a - tara) < 1) {

        digitalWrite(in1, dir);
        digitalWrite(in2, !dir);

        digitalWrite(in3, !dir);
        digitalWrite(in4, dir);

        analogWrite(enA, 255);
        analogWrite(enB, 255);

        delay(rdelay); // Let it rotate a bit
        getdata();
    }

    stop();
    
    return;
}


// get data from webserver
void getdata() {
    http.begin(server);  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request

    if (httpCode > 0) { //Check the returning 
        String payload = http.getString();   //Get the request response payload
        Serial.println(payload);                     //Print the response 
        split(payload);
    }
    http.end();   //Close
    
    return;
}

void contain() {
    getdata();
    if ((abs(data[n].x) - xmax) < margin) {
        if (data[n].x) 
            rotate(180);
        else 
            rotate (0);
        forward;
    }
    if ((abs(data[n].y) - ymax) < margin) {
        if (data[n].y) 
            rotate(270);
        else 
            rotate (90);
        forward;
    }
    return;
}

void start() {
    contain();
    while (inrange() > -1) {
        rotate(inrange());
        forward();
    }
    
    rotate();
    forward();
    
    return;
}

void loop() {

    while (WiFi.status() != WL_CONNECTED)
        connect();
    
    // If previous bots finished
    while (1) {
        bool done=1;
        for (int i=0, i<n; ++i) {
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
    
    return;
}
