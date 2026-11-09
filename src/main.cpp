#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <Botao.h>

void tratarJson(String mensagem);
void aoReceberMsg(char *topico, byte *payload, unsigned int tamanho);
String pedirParaApi(String codigo);
void reconectarMQTT();

const int pinLed = 6;

const char *SSID = "SENAI EDUCACIONAL";
const char *SENHA = "info@134";

const char *enderecoApi = "https://rickandmortyapi.com/api/character/";

const char *BROKER = "broker.emqx.io";
const int PORTA = 1883;

const char *TOPICO_SUB = "senai134/esp32/Luigi/grupo1/comando";
const char *TOPICO_PUB = "senai134/esp32/Luigi/grupo1/status";

LiquidCrystal_I2C lcd(0x27, 20, 4);

Botao btn(14);

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup()
{
  btn.iniciar();
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, LOW);

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Conectando WiFi...");

  WiFi.begin(SSID, SENHA);
  Serial.printf("Conectando à rede WiFi: %s\n", SSID);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("#");
    delay(1000);
  }

  Serial.printf("\nConectado ao WiFi: %s!\n", SSID);

  mqtt.setServer(BROKER, PORTA);
  mqtt.setCallback(aoReceberMsg);

  String personagem = pedirParaApi("1");
  if (personagem.length() > 0)
  {
    JsonDocument doc;
    DeserializationError errorJson = deserializeJson(doc, personagem);

    if (!errorJson)
    {
      String nome = doc["name"].as<String>();
      String especie = doc["species"].as<String>();
      String genero = doc["gender"].as<String>();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Seu personagem: 1");
      lcd.setCursor(0, 1);
      lcd.print("Nome: " + nome);
      lcd.setCursor(0, 2);
      lcd.print("Especie: " + especie);
      lcd.setCursor(0, 3);
      lcd.print("Genero: " + genero);

      Serial.printf("Nome: %s\nEspecie: %s\nGenero: %s\n", nome.c_str(), especie.c_str(), genero.c_str());
    }
    else
    {
      Serial.println("Falha ao ler JSON da API");
    }
  }
}

void loop()
{
  btn.atualizar();
  if (!mqtt.connected())
  {
    reconectarMQTT();
  }
  mqtt.loop();

  static unsigned long ultimoEnvio = 0;
  const unsigned long tempoEntreEnvios = 5000;

  if (millis() - ultimoEnvio >= tempoEntreEnvios)
  {
    ultimoEnvio = millis();

    String mensagem = "{\"status\":\"online\"}";
    mqtt.publish(TOPICO_PUB, mensagem.c_str());
  }

  if (btn.pressionou())
  {
    Serial.print("Pressionou");
    int numeroPseudoAleatorio = random(1, 101);
    String personagem = pedirParaApi(String(numeroPseudoAleatorio));

    if (personagem.length() > 0)
    {
      JsonDocument doc;
      DeserializationError errorJson = deserializeJson(doc, personagem);

      if (!errorJson)
      {
        String nome = doc["name"].as<String>();
        String especie = doc["species"].as<String>();
        String genero = doc["gender"].as<String>();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Seu personagem: " + (String)numeroPseudoAleatorio);
        lcd.setCursor(0, 1);
        lcd.print("Nome: " + nome);
        lcd.setCursor(0, 2);
        lcd.print("Especie: " + especie);
        lcd.setCursor(0, 3);
        lcd.print("Genero: " + genero);

        Serial.printf("Nome: %s\nEspecie: %s\nGenero: %s\n", nome.c_str(), especie.c_str(), genero.c_str());
      }
      else
      {
        Serial.println("Falha ao ler JSON da API");
      }
    }
  }
}

void aoReceberMsg(char *topico, byte *payload, unsigned int tamanho)
{
  String mensagem;
  mensagem.reserve(tamanho);

  for (unsigned int i = 0; i < tamanho; i++)
  {
    mensagem += (char)payload[i];
  }

  Serial.printf("Mensagem recebida no topico [%s]: %s\n", topico, mensagem.c_str());
  tratarJson(mensagem);
}

String pedirParaApi(String codigo)
{
  HTTPClient http;
  http.begin(enderecoApi + codigo);

  int codigoHTTP = http.GET();
  String resposta = "";

  if (codigoHTTP == HTTP_CODE_OK || codigoHTTP == 200)
  {
    resposta = http.getString();
  }
  else
  {
    Serial.printf("Erro na requisição HTTP. Código: %d\n", codigoHTTP);
  }

  http.end();
  return resposta;
}

void tratarJson(String mensagem)
{
  JsonDocument doc;
  DeserializationError codigoErro = deserializeJson(doc, mensagem);

  if (codigoErro)
  {
    Serial.println("Erro ao decodificar JSON MQTT");
    return;
  }

  bool estadoLed = doc["estadoLed"].as<bool>();
  bool sortear = doc["sortear"].as<bool>();

  if (estadoLed)
  {
    digitalWrite(pinLed, HIGH);
    Serial.println("LED LIGADO");
  }
  else
  {
    digitalWrite(pinLed, LOW);
    Serial.println("LED DESLIGADO");
  }
  if (sortear)
  {
    Serial.println("Sortear personagem");
    int numeroPseudoAleatorio = random(1, 101);
    String personagem = pedirParaApi(String(numeroPseudoAleatorio));

    if (personagem.length() > 0)
    {
      JsonDocument doc;
      DeserializationError errorJson = deserializeJson(doc, personagem);

      if (!errorJson)
      {
        String nome = doc["name"].as<String>();
        String especie = doc["species"].as<String>();
        String genero = doc["gender"].as<String>();

        Serial.print(numeroPseudoAleatorio);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Seu personagem: " + (String)numeroPseudoAleatorio);
        lcd.setCursor(0, 1);
        lcd.print("Nome: " + nome);
        lcd.setCursor(0, 2);
        lcd.print("Especie: " + especie);
        lcd.setCursor(0, 3);
        lcd.print("Genero: " + genero);

        Serial.printf("Nome: %s\nEspecie: %s\nGenero: %s\n", nome.c_str(), especie.c_str(), genero.c_str());
      }
      else
      {
        Serial.println("Falha ao ler JSON da API");
      }
    }
  }
}

void reconectarMQTT()
{
  while (!mqtt.connected())
  {
    String id = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.printf("Conectando ao Broker %s como %s...\n", BROKER, id.c_str());

    if (mqtt.connect(id.c_str()))
    {
      Serial.println("Conectado ao Broker MQTT!");
      mqtt.subscribe(TOPICO_SUB);
      Serial.printf("Inscrito no tópico: %s\n", TOPICO_SUB);
    }
    else
    {
      Serial.printf("Falha MQTT, rc=%d. Tentando novamente em 5s...\n", mqtt.state());
      delay(5000);
    }
  }
}