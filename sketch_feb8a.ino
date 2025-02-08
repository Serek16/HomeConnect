#include <WiFi.h>
#include <WiFiCredentials.h>

void setup() {
  Serial.begin(115200);
  printf("\n");

  WiFi.begin(SSID, PASS);
  printf("Connecting to WiFi \"%s\"", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".");
  }
  printf("\nSuccessfully connected\n");
}

void loop() {

}
