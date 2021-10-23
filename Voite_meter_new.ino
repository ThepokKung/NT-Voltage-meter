/* Libary */
#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

/* cal ac val */
double AC_Sensor_val = 0;
int crosscount = 0;
int climb_flag = 0;
int val[100];
int max_v = 0;
double VmaxD = 0;
double VeffD = 0;
double Veff = 0;

/* Analog Pin */
#define sensor_ac_pin 26
#define sensor_dc_pin_1 27
#define sensor_dc_pin_2 14
#define sensor_dc_pin_3 12
#define sensor_dc_pin_4 13

/* Val for cal DC  */
float volt_dc_1;
float volt_dc_2;
float volt_dc_3;
float volt_dc_4;
float DCinputforcal_dc_1 = 1296.00;
float DCinputforcal_dc_2 = 1296.00;
float DCinputforcal_dc_3 = 1296.00;
float DCinputforcal_dc_4 = 1296.00;

/* MQTT Server */
//Test server
const char* mqttServer = "broker.mqttdashboard.com";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
char mqtt_name_id[] = "";
const char* pubTopic = "ntnode/batt/";

//Real Server
/*
  const char* mqttServer = "180.180.216.61";
  const int mqttPort = 1883;
  const char* mqttUser = "juub";
  const char* mqttPassword = "t0t12345";
  char mqtt_name_id[] = "";
  const char* pubTopic = "ntnode/batt/";
*/

/* Ethernet config */
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

/* Call for use */
EthernetClient Eclient;
PubSubClient mqtt(Eclient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Json config */
StaticJsonDocument<256> doc;
char buffer[256];

/* DEVICE ID */
String deviceid = "kknb0001";
/* Check Val */
int check_time;

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Needed for Leonardo only
  delay(250);
  Serial.println("Start NT Voltage meter");
  delay(250);
  /* LCD SETUP */
  lcd.begin();
  lcd.backlight();

  lcd_startup();
  delay(250);

  /* Internet connect */
  //Ethernet_connect();
  delay(250);

  /* MQTT connect */
  //MQTT_connect();

  lcd.clear();
  delay(250);
}

void loop() {
  /* MQTT Check Connect */
  /*
    if (!mqtt.connected()) {
    Serial.println("---Reconnect MQTT ---");
    MQTT_reconnect();
    }
    mqtt.loop();
  */

  cal_ac_val();

  cal_dc_val();

  debug_here();

  lcd_main();

  send_data();

  /* END LOOP */
  VmaxD = 0;
  delay(1000);
  yield();
}

void Ethernet_connect() {
  Ethernet.init(5);
  lcd_internet_connecting();
  Serial.print("Connect to Ethernet : ");

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. 🙁");
  } else if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  } else if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Ethernet cable is connected.");
  }

  /* DHCP */
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  Serial.println("Done");

  /* Fix Internet IP
    Ethernet.begin(mac, ip, gateway, subnet);
    Serial.println("fix ip internet Done");
  */

  /* Show internet IP */
  Serial.print("Ethernet IP is: ");
  Serial.println(Ethernet.localIP());
  lcd_done();
  delay(100);
}

void debug_here() {
  /* Print Value */
  Serial.println("====================================");
  /* AC val */
  Serial.print("Voltage: ");
  Serial.println(Veff);
  /* DC val 1 */
  Serial.print("Battary DC Val 1 : ");
  Serial.print(volt_dc_1);
  Serial.println(" V ");
  /* DC val 2 */
  Serial.print("Battary DC Val 2 : ");
  Serial.print(volt_dc_2);
  Serial.println(" V ");
  /* DC val 3 */
  Serial.print("Battary DC Val 3 : ");
  Serial.print(volt_dc_3);
  Serial.println(" V ");
  /* DC val 4 */
  Serial.print("Battary DC Val 4 : ");
  Serial.print(volt_dc_4);
  Serial.println(" V ");
  Serial.println("====================================");

  /* DEBUG */
  Serial.println("============DEBUG==============");
  Serial.print("Analog Read AC val : ");
  Serial.println(analogRead(26));
  Serial.print("Analog Read DC_1 val : ");
  Serial.println(analogRead(27));
  Serial.println("===================");
}

void cal_ac_val() {
  for ( int i = 0; i < 100; i++ ) {
    AC_Sensor_val = analogRead(sensor_ac_pin);
    if (analogRead(sensor_ac_pin) > 3000) {
      val[i] = AC_Sensor_val;
    }
    else  if (analogRead(sensor_ac_pin) < 2850) {
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
    Veff = (((VeffD - 2103.6) / -453.6) * -210.2) + 210.2;
  }
  else {
    Veff = 0;
  }
}

void cal_dc_val() {
  /* DC check */
  float temp1;
  float temp2;
  float temp3;
  float temp4;

  /* Cal DC value */
  temp1 = analogRead(sensor_dc_pin_1);
  volt_dc_1 = 5.0 * temp1 / DCinputforcal_dc_1;

  temp2 = analogRead(sensor_dc_pin_2);
  volt_dc_2 = 5.0 * temp2 / DCinputforcal_dc_2;

  temp3 = analogRead(sensor_dc_pin_3);
  volt_dc_3 = 5.0 * temp3 / DCinputforcal_dc_3;

  temp4 = analogRead(sensor_dc_pin_4);
  volt_dc_4 = 5.0 * temp4 / DCinputforcal_dc_4;
}

void send_data() {
  /* json flie */
  for (int i = 5; i > -1; i--) {
    delay(1000);
    Serial.println("Countdown" + String(i));
    if (Veff < 145) {
      doc["deviceid"] = deviceid;
      doc["AC_1"] = 0;
      doc["DC_1"] = volt_dc_1;
      doc["DC_2"] = volt_dc_2;
      doc["DC_3"] = volt_dc_3;
      doc["DC_4"] = volt_dc_4;
      /* MQTT Send to mqtt*/
      MQTT_sendata();
      break;
    }
    else {
      check_time++;
    }
    if (check_time == 60) {
      doc["deviceid"] = deviceid;
      doc["AC_1"] = Veff;
      check_time = 0;
      /* MQTT Send to mqtt*/
      MQTT_sendata();
      break;
    }
  }
}

void MQTT_connect() {
  Serial.print("Set mqtt server :");
  lcd_mqtt_connect();
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(MQTT_callback);
  Serial.println(" Done ");
  lcd_done();
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
  size_t n = serializeJson(doc, buffer);

  lcd_mqtt_senddata();

  if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
    Serial.println("\nConnected MQTT: ");
    if (mqtt.publish(pubTopic, buffer , n) == true) {
      Serial.println("publish success");
      lcd_done();
    } else {
      Serial.println("publish Fail");
      lcd_fail();
    }
  } else {
    Serial.println("Connect Fail MQTT");
    lcd_fail();
  }
  Serial.println("====== JSON BODY =======");
  serializeJsonPretty(doc, Serial);
  Serial.println("======= JSON BODY END ======");
}

/* LCD */

void lcd_main() {
  if (volt_dc_1 > 10 or volt_dc_2 > 10 or volt_dc_3 > 10 or volt_dc_4 > 10) {
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  lcd.print(deviceid);
  lcd.setCursor(9, 0);
  lcd.print("DC");
  lcd.setCursor(12, 0);
  lcd.print(volt_dc_1);
  lcd.setCursor(1, 1);
  lcd.print(volt_dc_2);
  lcd.setCursor(5, 1);
  lcd.print("-");
  lcd.setCursor(6, 1);
  lcd.print(volt_dc_3);
  lcd.setCursor(10, 1);
  lcd.print("-");
  lcd.setCursor(11, 1);
  lcd.print(volt_dc_4);
  lcd.setCursor(15, 1);
  lcd.print("_");
}

void lcd_mqtt_reconnect() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("MQTT DOWN");
  lcd.setCursor(4, 1);
  lcd.print("Reconnect");
}

void lcd_startup() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Starting");
  lcd.setCursor(0, 1);
  lcd.print("NT Voltage Meter");
}

void lcd_internet_connecting() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ethernet connect");
}

void lcd_internet_ip_done() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("IP Address :");
  lcd.setCursor(0, 1);
  lcd.print(Ethernet.localIP());
}

void lcd_done() {
  lcd.setCursor(6, 1);
  delay(500);
  lcd.print("Done");
}

void lcd_fail() {
  lcd.setCursor(6, 1);
  delay(500);
  lcd.print("Fail");
}

void lcd_mqtt_connect() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("MQTT Connect");
}

void lcd_mqtt_senddata() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("MQTT senddata");
}
