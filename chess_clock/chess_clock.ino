#include <Wire.h>
#include <rgb_lcd.h>
#include <WiFi.h>
#include <SPI.h>

rgb_lcd lcd;

class Player {
  byte sensor_pin, led_pin;
  int time_left, last_check, state;
  int minutes, seconds;
  String nickname;
  bool loser, turn;

  public: Player(byte sensor_pin, byte led_pin, String nickname) {
      this->sensor_pin = sensor_pin;
      this->led_pin = led_pin;
      this->nickname = nickname;
      loser = false;
      turn = false;
      minutes = 0;
      seconds = 0;
      state = 0;
      time_left = 300000; // 5 * 60 * 1000
  }

  public: byte get_led() { return led_pin; }
  public: byte get_sensor() { return sensor_pin; }
  public: boolean get_loser() { return loser; }
  public: String get_nickname() { return nickname; }
  public: int get_state() { return state; }
  public: bool get_turn() { return turn; }

  public: void set_loser(bool loser) { this->loser = loser; }
  public: void set_state(int state) { this->state = state; }
  public: void set_turn(bool turn) { this->turn = turn; }
  public: void set_time_left(int time_left) { this->time_left = time_left; }

  void update() {
    time_left -= ((millis() - last_check));
    last_check = millis();
    
    if (time_left <= 0) {
      digitalWrite(led_pin, LOW);
      loser = true;
    }
    
    minutes = floor(time_left / 60000);
    seconds = floor(time_left / 1000) - minutes * 60;
    
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) { lcd.print(0); }
    lcd.print(seconds);
  }
  
  void start_turn () {
    if (turn) { return; }
    turn = true;
    last_check = millis();
  }
};

char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "YOUR-THINGSPEAK-WRITE-API-KEY";
const int updateThingSpeakInterval = 17 * 1000;

char ssid[] = "WIFI-NEWTORK-NAME";
char pass[] = "WIFI-PASSWORD";

int status = WL_IDLE_STATUS;
WiFiClient client;
 
long lastConnectionTime = 0;
boolean lastConnected = false;
int failedCounter = 0;

const byte player1_sensor = 6, player2_sensor = 7;
const byte player1_led = 4, player2_led = 5;
const String player1_name = "PLAYER1_NAME", player2_name = "PLAYER2_NAME";

Player player1(player1_sensor, player1_led, player1_name);
Player player2(player2_sensor, player2_led, player2_name);

bool game_over = false;

void setup() 
{
  Serial.begin(115200);
 
  while (status != WL_CONNECTED) {
    Serial.print("Connecting...");
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }
 
  Serial.println("Connection established!");
    lcd.begin(16, 2);
    lcd.setRGB(5, 0, 15);
    pinMode(player1.get_led(), OUTPUT);
    pinMode(player2.get_led(), OUTPUT);
    lcd.setCursor(0, 0);
    lcd.print(player1.get_nickname());
    lcd.setCursor(0, 1);
    lcd.print(player2.get_nickname());
}

void loop() { 
  while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
   
    if (!client.connected() && lastConnected ) {
      Serial.println("... connection closed!");
      Serial.println();
      client.stop();
    }
   
    lastConnected = client.connected();

    player1.set_state(digitalRead(player1_sensor));
    player2.set_state(digitalRead(player2_sensor));

  if ( player1.get_state() && player2.get_state() ) { restart(); }

  if (game_over) { return; }

  if (player2.get_state()) {
    player1.start_turn();
    player2.set_turn(false);
    lcd.setRGB(0, 80, 0);
    digitalWrite(player2_led, LOW);
    digitalWrite(player1_led, HIGH);
  }

  if (player1.get_state()) {
    player2.start_turn();
    player1.set_turn(false);
    lcd.setRGB(40, 0, 0);    
    digitalWrite(player1_led, LOW);
    digitalWrite(player2_led, HIGH);  
  }
  
  if (player1.get_turn()) {
    lcd.setCursor(12, 0);
    player1.update();
    if (player1.get_loser()) { display_result(player2.get_nickname()); }

  } else if (player2.get_turn()) {
    lcd.setCursor(12, 1);
    player2.update();
    if (player2.get_loser()) { display_result(player1.get_nickname()); }
  }
}

void restart() {
  game_over = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(player1.get_nickname());
  lcd.setCursor(0, 1);
  lcd.print(player2.get_nickname());
  player1.set_time_left(300000);
  player2.set_time_left(300000);
  player1.set_turn(false);
  player2.set_turn(false);
  player1.set_loser(false);
  player2.set_loser(false);
}

void display_result(String winner_name) {
  game_over = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAME OVER!");
  lcd.setCursor(0, 1);
  lcd.print("Winner: "+winner_name);
  if(!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval))
    updateThingSpeak("field1="+winner_name);
}

void updateThingSpeak(String  tsdata) {
  if (client.connect(thingSpeakAddress, 80)) {        
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: GJCZG9TXVG4TH859 \n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsdata.length());
    client.print("\n\n");
    client.print(tsdata);
    lastConnectionTime = millis();
     
    if (client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
      failedCounter = 0;
    } else {
      failedCounter++;
      Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");  
      Serial.println();
    }
    lastConnectionTime = millis();
  }
}

