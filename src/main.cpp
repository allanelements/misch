#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

const char *ssid = "Elements";
const char *password = "Elements@@2024!";

const int botaoSubirPin = 2;
const int botaoDescerPin = 3;
const int fimDeCursoSuperiorPin = 5;
const int fimDeCursoInferiorPin = 4;
const int releSubirPin = 26;  // Pino GPIO para controle do relé de subida
const int releDescerPin = 27; // Pino GPIO para controle do relé de descida
const int ledIndicativoPin = 13; // Pino GPIO para o LED indicativo

AsyncWebServer server(80);

// Estados da mesa
enum EstadoMesa
{
  Parada,
  Subindo,
  Descendo
};

EstadoMesa estadoAtual = Parada;

// Tempos de acionamento disponíveis
const int temposAcionamento[] = {500, 1000, 2000, 3000};
int tempoSelecionado = 2000; // Tempo padrão: 2 segundos

// Protótipos das funções
void desligarReles();
String obterEstadoMesa();
void acionarReleSubir();
void acionarReleDescer();
void acionarLedIndicativo();
void desligarLedIndicativo();
void pararMesa();
void definirTempoAcionamento(int tempo);

unsigned long tempoInicioAcionamento = 0;
unsigned long duracaoAcionamentoPadrao = 2000; // 2 segundos

void setup()
{
  Serial.begin(9600);

  pinMode(botaoSubirPin, INPUT_PULLUP);
  pinMode(botaoDescerPin, INPUT_PULLUP);
  pinMode(fimDeCursoSuperiorPin, INPUT_PULLUP);
  pinMode(fimDeCursoInferiorPin, INPUT_PULLUP);
  pinMode(releSubirPin, OUTPUT);
  pinMode(releDescerPin, OUTPUT);
  pinMode(ledIndicativoPin, OUTPUT);

  // Inicia com os relés desligados e o LED indicativo apagado
  desligarReles();
  desligarLedIndicativo();

  Serial.println("Iniciando conexão Wi-Fi...");

  // Conectar-se à rede Wi-Fi
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando à rede Wi-Fi...");
  }

  Serial.println("Conectado à rede Wi-Fi");
  Serial.print("Endereço IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // Configura as rotas do servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<style>";
    html += "body { font-family: 'Arial', sans-serif; text-align: center; background-color: #f4f4f4; }";
    html += "h1 { color: #333; }";
    html += "button { font-size: 2em; padding: 10px 20px; margin: 10px; border: none; border-radius: 5px; cursor: pointer; }";
    html += "button.subir { background-color: #3498db; color: white; }";
    html += "button.descer { background-color: #3498db; color: white; }";
    html += "button.emergencia { background-color: #e74c3c; color: white; }";
    html += ".logo { max-width: 100%; height: auto; }";
    html += "</style></head><body>";
    html += "<img src='https://elements.com.br/cdn/shop/files/Group_1.png?v=1702562143&width=50' alt='Logo' class='logo'>";
    html += "<h1>Teste Protótipo Misch</h1>";
    html += "<p>Estado Atual: " + obterEstadoMesa() + "</p>";
    html += "<form action='/subir' method='post'><button class='subir' type='submit'>Subir Mesa</button></form>";
    html += "<form action='/descer' method='post'><button class='descer' type='submit'>Descer Mesa</button></form>";
    html += "<form action='/parar' method='post'><button class='emergencia' type='submit'>STOP</button></form>";
    html += "<p>Tempo de Acionamento: ";
    html += "<select onchange='this.form.submit()' name='tempo'>";
    for (int i = 0; i < sizeof(temposAcionamento) / sizeof(temposAcionamento[0]); i++)
    {
      html += "<option value='" + String(temposAcionamento[i]) + "' " + (temposAcionamento[i] == tempoSelecionado ? "selected" : "") + ">" + String(temposAcionamento[i] / 1000.0) + " segundos</option>";
    }
    html += "</select></p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Rota para subir a mesa
  server.on("/subir", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (digitalRead(fimDeCursoSuperiorPin) == HIGH)
    {
      acionarReleSubir();
    }
    request->redirect("/");
  });

  // Rota para descer a mesa
  server.on("/descer", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (digitalRead(fimDeCursoInferiorPin) == HIGH)
    {
      acionarReleDescer();
    }
    request->redirect("/");
  });

  // Rota para parar a mesa
  server.on("/parar", HTTP_POST, [](AsyncWebServerRequest *request) {
    pararMesa();
    request->redirect("/");
  });

  // Configura o tempo de acionamento quando a seleção é alterada
  server.on("/tempo", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("tempo"))
    {
      tempoSelecionado = request->getParam("tempo")->value().toInt();
      definirTempoAcionamento(tempoSelecionado);
    }
    request->redirect("/");
  });

  // Inicia o servidor
  server.begin();
}

void loop()
{
  // Adicione qualquer lógica adicional necessária aqui
  if (tempoInicioAcionamento > 0 && millis() - tempoInicioAcionamento >= tempoSelecionado)
  {
    // Se o tempo de acionamento expirou, desliga os relés
    desligarReles();
  }

  // Verifica o estado da mesa para ligar/desligar o LED indicativo
  switch (estadoAtual)
  {
  case Parada:
    acionarLedIndicativo();
    break;
  default:
    desligarLedIndicativo();
    break;
  }
}

String obterEstadoMesa()
{
  switch (estadoAtual)
  {
  case Parada:
    return "Mesa parada";
  case Subindo:
    return "Mesa subindo";
  case Descendo:
    return "Mesa descendo";
  default:
    return "Estado desconhecido";
  }
}

void acionarReleSubir()
{
  digitalWrite(releSubirPin, HIGH);
  digitalWrite(releDescerPin, LOW);
  tempoInicioAcionamento = millis(); // Inicia o temporizador de acionamento
  estadoAtual = Subindo;
}

void acionarReleDescer()
{
  digitalWrite(releSubirPin, LOW);
  digitalWrite(releDescerPin, HIGH);
  tempoInicioAcionamento = millis(); // Inicia o temporizador de acionamento
  estadoAtual = Descendo;
}

void pararMesa()
{
  digitalWrite(releSubirPin, LOW);
  digitalWrite(releDescerPin, LOW);
  estadoAtual = Parada;
}

void desligarReles()
{
  pararMesa();
  tempoInicioAcionamento = 0; // Reseta o temporizador de acionamento
}

void acionarLedIndicativo()
{
  if (digitalRead(releSubirPin) == LOW && digitalRead(releDescerPin) == LOW)
  {
    digitalWrite(ledIndicativoPin, HIGH);
  }
}

void desligarLedIndicativo()
{
  digitalWrite(ledIndicativoPin, LOW);
}

void definirTempoAcionamento(int tempo)
{
  duracaoAcionamentoPadrao = tempo;
}
