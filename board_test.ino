#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

#include <WiFiUdp.h>
#include <TimeLib.h>

#define WIFI_SSID "PROLiNK_PRN3009"
#define WIFI_PASSWORD "enter2surf@"
#define FIREBASE_HOST "home-rainmeter-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "88Yf9WeshLSGaL1bldvDhdVY8k0xWHFSl1Qb3gou"

FirebaseData fbdo;

const int Reader = 4;
const int LED = 5;
int n = 0;
bool lastRead = false;
const long utcOffsetInSeconds = 3600 * 5.5;
int lastTime = 0;
String rawFbStr = "";
String fbStr = "";
int fbInt;

// Define NTP Client to get time
WiFiUDP Udp;

static const char ntpServerName[] = "us.pool.ntp.org";
const float timeZone = 5.5;     // Sri Lankan Time
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);


void setup() {
  Serial.begin(9600);
  Serial.print("STARTING DEVICE...");


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(1024, 1024);
  fbdo.setResponseSize(1024);

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(1800);

  rawFbStr = String(year()) + "/" + String(month()) + "/" + String(day()) + "/" + String(hour());

}

time_t prevDisplay = 0;



void loop() {


  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      rawFbStr = String(year()) + "/" + String(month()) + "/" + String(day()) + "/" + String(hour());
    }
  }


  if (rawFbStr != fbStr) {
    n = 0;
    fbStr = rawFbStr;
    Firebase.getString(fbdo, fbStr);
    Serial.print("Firebase Response: ");
    Serial.println(fbdo.stringData());
    if(fbdo.stringData() == "null") {
      Firebase.setInt(fbdo, fbStr, n);
    }
    Serial.print("Current TimeString: ");
    Serial.println(fbStr);
  }


  if (digitalRead(Reader) != lastRead) {
    if (digitalRead(Reader) == true) {
      Serial.println("DETECTED!");
      n = n + 1;
      Serial.print("Count: ");
      Serial.println(n);
      Firebase.setInt(fbdo, fbStr, n);
      lastRead = digitalRead(Reader);
    }
    else {
      Serial.println("DROPPED!");
      lastRead = digitalRead(Reader);
    }
  }
}


void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
