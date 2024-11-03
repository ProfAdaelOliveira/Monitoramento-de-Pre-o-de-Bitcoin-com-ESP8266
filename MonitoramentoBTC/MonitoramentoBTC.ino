#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <BearSSLHelpers.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// Configuração Wi-Fi
const char* ssid = "VIVOFIBRA-6349";
const char* password = "R3ufc2EuKm";

// Configuração de pinos
const int ledGreenPin = D4; // LED Verde
const int ledRedPin = D5;   // LED Vermelho
const int buzzerPin = D6;   // Buzzer

// Configuração do LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C e tamanho do display

// Variáveis para monitoramento de preço
float lastPrice = 0.0;
float alertPrice = 30000.0;  // Defina o valor de alerta
unsigned long previousMillis = 0;  // Armazena o tempo da última verificação
const long interval = 5000;        // Intervalo de 5 segundos

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Configuração dos pinos
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Inicialização do LCD
  lcd.begin(16, 2);  // Definindo 16 colunas e 2 linhas
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Conectando...");

  // Conectar ao Wi-Fi
  Serial.print("Conectando-se ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado!");
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Atualiza o tempo da última verificação

    if (WiFi.status() == WL_CONNECTED) {
      // Criar um cliente BearSSL WiFiClientSecure para HTTPS
      std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
      client->setInsecure();  // Ignorar verificação de certificado

      HTTPClient http;
      // Alterando a URL para obter o preço em BRL
      http.begin(*client, "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=brl");

      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {  // Verifica se a resposta HTTP é 200 (OK)
        String payload = http.getString();
        
        // Analisar JSON
        StaticJsonDocument<200> doc;
        deserializeJson(doc, payload);
        float currentPrice = doc["bitcoin"]["brl"];
        
        // Exibir no Serial e LCD
        Serial.print("Preço do Bitcoin: R$");
        Serial.println(currentPrice);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BTC: R$");
        lcd.print(currentPrice, 2);  // Exibe com duas casas decimais

        // Verificar aumento ou queda e acionar LEDs
        if (currentPrice > lastPrice) {
          digitalWrite(ledGreenPin, HIGH);
          digitalWrite(ledRedPin, LOW);
        } else if (currentPrice < lastPrice) {
          digitalWrite(ledGreenPin, LOW);
          digitalWrite(ledRedPin, HIGH);
        }

        // Verificar valor de alerta para o buzzer
        if (currentPrice >= alertPrice) {
          tone(buzzerPin, 1000);  // Aciona o som
          delay(1000);
          noTone(buzzerPin);
        }

        lastPrice = currentPrice;  // Atualiza o último preço
      } else {
        Serial.println("Falha na solicitação HTTP.");
        Serial.print("Código HTTP: ");
        Serial.println(httpCode);  // Exibe o código de erro HTTP
        // Mantém o último preço no LCD
        lcd.setCursor(0, 0);
        lcd.print("BTC: R$");
        lcd.print(lastPrice, 2);  // Exibe o último preço com duas casas decimais

        // Se o código for 429, aguarde 2 minutos antes de tentar novamente
        if (httpCode == 429) {
          Serial.println("Aguarde 1 minuto para tentar novamente.");
          delay(60000);  // Aguarde 1 minuto
        }
      }

      http.end();
    } else {
      Serial.println("WiFi desconectado.");
      // Mantém o último preço no LCD
      lcd.setCursor(0, 0);
      lcd.print("WiFi desconectado");
      delay(1000);  // Espera um pouco antes de tentar se reconectar
    }
  }
}
