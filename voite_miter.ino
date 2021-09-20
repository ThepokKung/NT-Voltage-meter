/* NT Voltage meter  */
/* C0 = DC1 */
/* C1 = DC2 */
/* C2 = DC3 */
/* C3 = DC4 */
/* C4 = AC = Veff*

  /* Libary */
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

/* Define Pin */
#define S0 16
#define S1 5
#define S2 4
#define S3 0
#define SIG A0

/* Val for cal DC  */
int DC_val_1_1; //DC1
float DC_val_1_2;
int DC_val_2_1; //DC2
float DC_val_2_2;
int DC_val_3_1; //DC3
float DC_val_3_2;
int DC_val_4_1; //DC4
float DC_val_4_2;
float DCinputforcal = 3.10;

/* Val for cal AC */
double AC_Sensor_val = 0;
int crosscount = 0;
int climb_flag = 0;
int val[100];
int max_v = 0;
double VmaxD = 0;
double VeffD = 0;
double Veff = 0;

/* Muti analog */
int decimal = 2;
int sensor0;  //DC1
int sensor1;  //DC2
int sensor2;  //DC3
int sensor3;  //DC4
int sensor4;  //AC1

/* Check Val */
int check_time;
/* DEVICE ID */
String deviceid = "kknb0001";

/* WIFI config */
char* ssid = "RSRC2"; //Wifi Name
char* password = "043234794"; //Wifi Password

/* MQTT Connect
  const char* mqttServer = "180.180.216.61";
  const int mqttPort = 1883;
  const char* mqttUser = "juub";
  const char* mqttPassword = "t0t12345";
  char mqtt_name_id[] = "";
  const char* pubTopic = "ntnode/batt/";

  /* MQTT Connect */
const char* mqttServer = "broker.mqttdashboard.com";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
char mqtt_name_id[] = "";
const char* pubTopic = "ntnode/batt/";

/* Ethernet */
byte mac[] = {
  0xDE, 0xFD, 0xBE, 0xEF, 0xFE, 0x2F
};

IPAddress ip(10, 232, 12, 29);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(10, 232, 12, 254);

/* Json */
StaticJsonDocument<256> doc;
char buffer[256];

/* Call for use */
//EthernetClient Eclient;
WiFiClient Eclient;
PubSubClient mqtt(Eclient);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  /* Serial begin */
  Serial.begin(115200);
  delay(500);

  /* StartUp*/
  startup();

  /* Internet connect */
  //Ethernet_connect();
  setup_wifi();

  /* PinMode */
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  /* MQTT connect */
  MQTT_connect();

  Serial.println("===========================");
}

void loop() {
  /* MQTT Check Connect */
  if (!mqtt.connected()) {
    Serial.println("---Reconnect MQTT ---");
    MQTT_reconnect();
  }
  mqtt.loop();

  read_muti_analog();

  cal_dc_val();

  cal_ac_val();

  /* Serial print DC Val */
  Serial.print("Battary DC Val 1 : ");
  Serial.print(DC_val_1_2);
  Serial.println(" V ");
  Serial.print("Battary DC Val 2 : ");
  Serial.print(DC_val_2_2);
  Serial.println(" V ");
  Serial.print("Battary DC Val 3 : ");
  Serial.print(DC_val_3_2);
  Serial.println(" V ");
  Serial.print("Battary DC Val 4 : ");
  Serial.print(DC_val_4_2);
  Serial.println(" V ");

  /* Serial print AC value */
  Serial.print("AC Voltage : ");
  Serial.println(Veff);

  /* json flie */
  for (int i = 5; i > -1; i--) {
    delay(1000);
    Serial.println("Countdown" + String(i));
    if (Veff == 0) {
      doc["deviceid"] = deviceid;
      doc["AC_1"] = Veff;
      doc["DC_1"] = DC_val_1_2;
      doc["DC_2"] = DC_val_2_2;
      doc["DC_3"] = DC_val_3_2;
      doc["DC_4"] = DC_val_4_2;
      /* MQTT Send to mqtt*/
      MQTT_sendata();
      break;
    }
    else {
      doc["deviceid"] = deviceid;
      doc["AC_1"] = Veff;
      check_time++;
    }
    if (check_time == 60) {
      check_time = 0;
      /* MQTT Send to mqtt*/
      MQTT_sendata();
      break;
    }
  }

  /*  DEBUG */
  Serial.println("========== DEBUG Start ==========");
  Serial.print("Sensorn C0: ");
  Serial.println(sensor0);
  Serial.print("Sensorn C1: ");
  Serial.println(sensor1);
  Serial.print("Sensorn C2: ");
  Serial.println(sensor2);
  Serial.print("Sensorn C3: ");
  Serial.println(sensor3);
  Serial.print("Sensorn C4: ");
  Serial.println(sensor4);

  /* END LOOP */
  Ethernet.maintain();
  Serial.println("------------------------------------------");
  delay(1000);
  yield();
}

void Ethernet_connect() {
  digitalWrite(LED_BUILTIN, LOW);
  Ethernet.init(15);
  /*
    if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
    }
  */
  /*
    Serial.print("Connect to Ethernet : ");
    Ethernet.begin(mac, ip, gateway, subnet);
    Serial.println("fix ip internet Done");
    Serial.print("Ethernet IP is: ");
    Serial.println(Ethernet.localIP());
    }
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. 🙁");
    } else if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    } else if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Ethernet cable is connected.");
    };
  */
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
}

void cal_dc_val() {
  /* DC check */
  float temp1;
  float temp2;
  float temp3;
  float temp4;

  /* Cal DC value */
  temp1 = sensor0 / DCinputforcal;
  DC_val_1_1 = (int)temp1;
  DC_val_1_2 = ((DC_val_1_1 % 100) / 10.0);

  temp2 = sensor1 / DCinputforcal;
  DC_val_2_1 = (int)temp2;
  DC_val_2_2 = ((DC_val_2_1 % 100) / 10.0);

  temp3 = sensor2 / DCinputforcal;
  DC_val_3_1 = (int)temp3;
  DC_val_3_2 = ((DC_val_3_1 % 100) / 10.0);

  temp4 = sensor3 / DCinputforcal;
  DC_val_4_1 = (int)temp4;
  DC_val_4_2 = ((DC_val_4_1 % 100) / 10.0);
}

void cal_ac_val() {
  /* Cal AC value */
  for ( int i = 0; i < 100; i++ ) {
    AC_Sensor_val = sensor4;
    if (sensor4 > 560) {
      val[i] = AC_Sensor_val;
    }
    else {
      val[i] = 0;
    }
    delay(1);
  }
  max_v = 0;
  for ( int i = 0; i < 100; i++ )
  {
    if ( val[i] > max_v )
    {
      max_v = val[i];
    }
    val[i] = 0;
  }
  if (max_v != 0) {
    VmaxD = max_v;
    VeffD = VmaxD / sqrt(2);
    Veff = (((VeffD - 420.76) / -90.24) * -200.2) + 200.2;
  }
  else {
    Veff = 0;
  }

  /* Reset */
  VmaxD = 0;
}

void read_muti_analog() {
  /* Read form muti analog */
  // Channel 0 (C0 pin - binary output 0,0,0,0)
  digitalWrite(S0, LOW); digitalWrite(S1, LOW); digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  sensor0 = analogRead(SIG);

  // Channel 1 (C1 pin - binary output 1,0,0,0)
  digitalWrite(S0, HIGH); digitalWrite(S1, LOW); digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  sensor1 = analogRead(SIG);

  // Channel 2 (C2 pin - binary output 0,1,0,0)
  digitalWrite(S0, LOW); digitalWrite(S1, HIGH); digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  sensor2 = analogRead(SIG);

  // Channel 3 (C3 pin - binary output 1,1,0,0)
  digitalWrite(S0, HIGH); digitalWrite(S1, HIGH); digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  sensor3 = analogRead(SIG);

  // Channel 4 (C4 pin - binary output 0,0,1,0)
  digitalWrite(S0, LOW); digitalWrite(S1, LOW); digitalWrite(S2, HIGH); digitalWrite(S3, LOW);
  sensor4 = analogRead(SIG);
}

void startup() {
  Serial.println("================================");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Start NT Voltage meter");

  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
}

void MQTT_connect() {
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Set mqtt server :");
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(MQTT_callback);
  Serial.println(" Done ");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
}

void MQTT_callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void MQTT_reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
      Serial.println("-> MQTT mqtt connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println("-> try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void MQTT_sendata() {
  digitalWrite(LED_BUILTIN, LOW);
  size_t n = serializeJson(doc, buffer);

  if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
    Serial.println("\nConnected MQTT: ");
    if (mqtt.publish(pubTopic, buffer , n) == true) {
      Serial.println("publish success");
    } else {
      Serial.println("publish Fail");
    }
  } else {
    Serial.println("Connect Fail MQTT");
  }
  Serial.println("====== JSON BODY =======");
  serializeJsonPretty(doc, Serial);
  Serial.println("======= JSON BODY END ======");
  digitalWrite(LED_BUILTIN, HIGH);
}
