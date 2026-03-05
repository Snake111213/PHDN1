// ============================================================
//  ESP32 – Control de Motores DC vía MQTT (HiveMQ Cloud TLS)
//  Recibe comandos F/B/L/R/S en el topic "zemi/motores/cmd"
// ============================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ─── WiFi ────────────────────────────────────────────────────
const char* WIFI_SSID     = "Karen";       // ← Cambia esto
const char* WIFI_PASSWORD = "Karen8318";    // ← Cambia esto

// ─── MQTT HiveMQ Cloud ──────────────────────────────────────
const char* MQTT_BROKER   = "b1439a9c1a134c51be1ef17aa3c3cd62.s1.eu.hivemq.cloud";
const int   MQTT_PORT     = 8883;
const char* MQTT_USER     = "public";
const char* MQTT_PASS     = "Admin123";
// Client ID se genera en setup() para ser único
const char* TOPIC_CMD     = "zemi/motores/cmd";
const char* TOPIC_STATUS  = "zemi/motores/status";

// ─── Pines del puente H ─────────────────────────────────────
constexpr uint8_t IN1 = 13;   // Motor A+
constexpr uint8_t IN2 = 12;   // Motor A−
constexpr uint8_t IN3 = 14;   // Motor B+
constexpr uint8_t IN4 = 27;   // Motor B−

// ─── Certificado raíz HiveMQ Cloud (ISRG Root X1) ──────────
static const char* root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// ─── Objetos globales ───────────────────────────────────────
WiFiClientSecure espClient;
PubSubClient     mqtt(espClient);
char clientId[40];

// ─── Control de motores ─────────────────────────────────────
void aplicarMotores(bool a1, bool a2, bool b1, bool b2) {
  digitalWrite(IN1, a1);
  digitalWrite(IN2, a2);
  digitalWrite(IN3, b1);
  digitalWrite(IN4, b2);
}

void adelante()   { aplicarMotores(HIGH, LOW,  HIGH, LOW);  }
void atras()      { aplicarMotores(LOW,  HIGH, LOW,  HIGH); }
void izquierda()  { aplicarMotores(LOW,  HIGH, HIGH, LOW);  }
void derecha()    { aplicarMotores(HIGH, LOW,  LOW,  HIGH); }
void detener()    { aplicarMotores(LOW,  LOW,  LOW,  LOW);  }

// ─── Callback MQTT ──────────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length == 0) return;

  char cmd = (char)payload[0];
  Serial.printf("[MQTT] Comando recibido: %c\n", cmd);

  switch (cmd) {
    case 'F': adelante();  break;
    case 'B': atras();     break;
    case 'L': izquierda(); break;
    case 'R': derecha();   break;
    case 'S': detener();   break;
    default:
      Serial.printf("[MQTT] Comando desconocido: %c\n", cmd);
      return;
  }

  // Publicar estado de vuelta
  char estado[32];
  snprintf(estado, sizeof(estado), "{\"cmd\":\"%c\",\"ok\":true}", cmd);
  mqtt.publish(TOPIC_STATUS, estado);
}

// ─── Conexión WiFi ──────────────────────────────────────────
void conectarWiFi() {
  Serial.printf("[WiFi] Conectando a %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\n[WiFi] Conectado — IP: %s\n", WiFi.localIP().toString().c_str());
}

// ─── Conexión MQTT ──────────────────────────────────────────
void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.printf("[MQTT] Conectando como '%s'...\n", clientId);
    Serial.printf("[MQTT] Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    Serial.printf("[MQTT] User: %s\n", MQTT_USER);
    Serial.printf("[MQTT] Pass length: %d\n", strlen(MQTT_PASS));

    if (mqtt.connect(clientId, MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] ¡Conectado!");
      mqtt.subscribe(TOPIC_CMD);
      Serial.printf("[MQTT] Suscrito a: %s\n", TOPIC_CMD);
      mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}");
    } else {
      int rc = mqtt.state();
      Serial.printf("[MQTT] Error rc=%d\n", rc);
      Serial.println("[MQTT] Códigos: -4=timeout, -2=fail, 4=bad_cred, 5=no_auth");
      Serial.println("[MQTT] Reintentando en 5s...");
      delay(5000);
    }
  }
}

// ─── Setup ──────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("  ESP32 Motor Controller — MQTT + TLS  ");
  Serial.println("========================================");

  // Configurar pines de motor como salida
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  detener();

  // Conectar WiFi
  conectarWiFi();

  // Generar Client ID único
  snprintf(clientId, sizeof(clientId), "ESP32_%04X", (uint16_t)random(0xFFFF));
  Serial.printf("[MQTT] Client ID: %s\n", clientId);

  // Configurar TLS — usar setInsecure() para evitar problemas de certificado
  // espClient.setCACert(root_ca);   // Comentado para debug
  espClient.setInsecure();            // Acepta cualquier certificado

  // Configurar MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setBufferSize(1024);
  mqtt.setKeepAlive(60);
  mqtt.setSocketTimeout(10);
  mqtt.setCallback(mqttCallback);

  conectarMQTT();
}

// ─── Loop ───────────────────────────────────────────────────
void loop() {
  // Reconectar si se pierde la conexión
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqtt.connected()) {
    detener();  // Seguridad: parar motores si se pierde conexión
    conectarMQTT();
  }

  mqtt.loop();
}
