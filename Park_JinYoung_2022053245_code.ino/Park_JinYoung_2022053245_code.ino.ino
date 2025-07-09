  #include <ESP8266WiFi.h>
  #include<LiquidCrystal_I2C.h>
    #include <ESP8266WebServer.h>

  #include <Wire.h>
  LiquidCrystal_I2C lcd(0x27,16,2); //LCD 클래스 초기화


  const char* ssid = "HY-DORM3-403";
  const char* password = "residence403";

  const char* api_key = "4b6b796843746f6f3639475a64796e";
  const char* SERVER = "swopenapi.seoul.go.kr";
  const int httpPort = 80;

  ESP8266WebServer server(80);
  WiFiClient client;
  #define BUTTON_UP  4
  #define BUTTON_DOWN  5

  boolean last_Button = HIGH;
  boolean c_Button = HIGH;

  boolean last_Button2 = HIGH;
  boolean c_Button2 = HIGH;
  bool requestDone1 = false;  // type1 버튼 요청 완료 플래그
  bool requestDone2 = false;  // type2 버튼 요청 완료 플래그

  String station = "%ED%95%9C%EB%8C%80%EC%95%9E"; // UTF-8 한글 인코딩 주의

  String payload = "";
  String trainName,sttsu,msg2,msg3 = "";
  String filteredPayload = "";


  String getNumber(String str, String tag, int from);
  void setup() {
    Serial.begin(115200);
    delay(100);
    lcd.init();
    lcd.backlight();

    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.print("push Button");
    // /type1 경로 처리
    server.on("/type1", []() {
      c_Button= LOW;
      server.send(200, "text/plain; charset=UTF-8", "하행 요청 수신됨");
    });

    // /type2 경로 처리
    server.on("/type2", []() {
      c_Button2= LOW; 
      server.send(200, "text/plain; charset=UTF-8", "상행 요청 수신됨");
    });
    server.on("/", []() {
      server.send(200, "text/plain; charset=UTF-8", filteredPayload);
    });
  server.begin();
  Serial.println("HTTP server started");
  }
void loop() {


  c_Button = digitalRead(BUTTON_DOWN);
  c_Button2 = digitalRead(BUTTON_UP);
  server.handleClient();

  // type1 버튼 눌림 감지 (HIGH->LOW) & 중복 방지
  if (last_Button == HIGH && c_Button == LOW && !requestDone1) {
    makeRequestAndUpdateMessage("<updnLine>하행</updnLine>");
    requestDone1 = true;
  }
  // 버튼 떼어짐 감지 (LOW->HIGH) 시 플래그 초기화
  if (last_Button == LOW && c_Button == HIGH) {
    requestDone1 = false;
  }

  // type2 버튼 눌림 감지 & 중복 방지
  if (last_Button2 == HIGH && c_Button2 == LOW && !requestDone2) {
    makeRequestAndUpdateMessage("<updnLine>상행</updnLine>");
    requestDone2 = true;
  }
  // 버튼 떼어짐 시 플래그 초기화
  if (last_Button2 == LOW && c_Button2 == HIGH) {
    requestDone2 = false;
  }

  last_Button = c_Button;
  last_Button2 = c_Button2;

  delay(10);  
}

  void makeRequestAndUpdateMessage(String line) {
  String url = "/api/subway/";
  url += api_key;
  url += "/xml/realtimeStationArrival/1/5/";
  url += station;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  if (!client.connect(SERVER, httpPort)) {
    Serial.println("Connection failed");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + SERVER + "\r\n" +
               "Connection: close\r\n\r\n");

  // 서버 응답 읽기
  String payload = "";
  while (client.connected()) {
    while (client.available()) {
      payload = client.readStringUntil('\n');
    }
  }

  int index = 0;
  String result = "";
  lcd.clear();

  while (true) {
    String section = extractRow(payload, index);
    if (section == "") break;

    if (section.indexOf(line) >= 0) {
      String subwayId = getNumber(section, "<subwayId>", "</subwayId>");
      if (subwayId != "1004") {
        index++;
        continue;
      }

      String trainName = getNumber(section, "<trainLineNm>", "</trainLineNm>");
      String msg2 = getNumber(section, "<arvlMsg2>", "</arvlMsg2>");
      String msg3 = getNumber(section, "<arvlMsg3>", "</arvlMsg3>");

      Serial.println(subwayId);
      Serial.println(trainName);
      Serial.println(msg2);
      Serial.println(msg3);

      if (msg2.length() <= 1) {
        lcd.print("not train");
        break;
      }
      result += subwayId + " " + trainName + "\n" + msg2 + "\n";

      if (strncmp(msg2.c_str(), "전", 3) == 0) {
        msg2.setCharAt(1, '1');
      } else if (strncmp(msg2.c_str(), "한", 3) == 0) {
        msg2.setCharAt(1, '0');
      }
      lcd.print(msg2[1]);
      lcd.print(" > ");
    }
    index++;
    delay(1000);
  }
  delay(10000);
  lcd.clear();
  lcd.print("wait");

  filteredPayload = result;
  }
  String extractRow(String xml, int index) {
      int start = 0;
      for (int i = 0; i <= index; i++) {
          start = xml.indexOf("<row>", start);
          if (start == -1) return "";
          // 다음 탐색을 위해 end 이후로 이동
          int end = xml.indexOf("</row>", start);
          if (end == -1) return "";

          if (i == index) {
              return xml.substring(start, end + 6);
          }
          // 다음 <row> 탐색을 위해 end 다음으로 이동
          start = end + 6;
      }
      return "";
  }
  String getNumber(String xml, String startTag, String endTag) {
      int start = xml.indexOf(startTag);
      if (start == -1) return "";
      start += startTag.length();
      int end = xml.indexOf(endTag, start);
      if (end == -1) return "";
      return xml.substring(start, end);
  }


