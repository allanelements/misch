#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <U8g2lib.h>
#include <ESP32Encoder.h>

// Definindo os pinos do encoder
#define ENCODER_CLK_PIN 27  // Pino CLK do encoder
#define ENCODER_DT_PIN 26   // Pino DT do encoder

// Criando uma instância do encoder
ESP32Encoder encoder;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_MPR121 touch;
unsigned int altura = 90;
uint16_t lastTouched = 0;
bool portaTocadaAnteriormente[12] = {false};
unsigned long ultimaAlteracao = 0;
unsigned long desligaTela = 0;
int tela = 0;

long encoderValue;

void draw(void) {
    if (tela == 0){
      u8g2.setFont(u8g2_font_inb30_mr);
      // Centraliza o valor da variável no meio da tela
      String variavelStr = String(altura);
      const char* variavelChar = variavelStr.c_str();
      int x = (128 - u8g2.getStrWidth(variavelChar)) / 2;
      int y = 42;  // Altura central da tela
      u8g2.drawStr(x, y, variavelChar);

    }
    if(tela == 1){
      u8g2.setFont(u8g2_font_inb16_mr);
    // Centraliza o valor da variável no meio da tela
    String variavelStr = String(encoderValue);
    const char* variavelChar = variavelStr.c_str();
    // Combina as strings da variável e das setas
    String textoCompleto = variavelStr + "   " + altura ;
    int x = (128 - u8g2.getStrWidth(textoCompleto.c_str())) / 2;
    int y = 42;  // Altura central da tela
    u8g2.drawStr(x, y, textoCompleto.c_str());
    // Desenha uma seta para cima
    u8g2.setDrawColor(1); // Define a cor do desenho como preto (1)
    u8g2.setDrawColor(2); // Define a espessura da linha como 2 pixels
    
    if(encoderValue > altura){
      u8g2.drawLine(53, 30, 63, 40); // Linha diagonal inferior da seta
      u8g2.drawLine(63, 40, 73, 30); // Linha diagonal superior da seta
    }
     if (encoderValue < altura){
      u8g2.drawLine(53, 40, 63, 30); // Linha diagonal superior invertida da seta
      u8g2.drawLine(63, 30, 73, 40); // Linha diagonal inferior invertida da seta
    }

    
  }
    if (tela == 2) {
      u8g2.setFont(u8g2_font_inb30_mr);
      // Centraliza o valor da variável no meio da tela
      String variavelStr = String(altura);
      const char* variavelChar = variavelStr.c_str();
      int x = (128 - u8g2.getStrWidth(variavelChar)) / 2;
      int y = 42;  // Altura central da tela
      u8g2.drawStr(x, y, "  ");
      u8g2.clearBuffer();  // Limpa o buffer
      u8g2.sendBuffer();   // Envia o buffer limpo para o display
    }
}

void setup() {
    //Serial.begin(115200);
    //while (!Serial)
       // ;

    Serial.println("MPR121 Capacitive Touch Test");

    if (!touch.begin()) {
        Serial.println("Error initializing MPR121");
        while (1)
            ;
    }
    touch.setThresholds(2, 0);  // Ajuste de limiar de toque e liberação

    // Configurando os pinos do encoder
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);

    encoder.attachHalfQuad(ENCODER_CLK_PIN, ENCODER_DT_PIN);

    u8g2.begin();
    encoder.setCount(90);
    ultimaAlteracao = millis();
}

void loop() {
    // Leitura do encoder
     encoderValue = encoder.getCount();

    // Exibir valores do encoder
    //Serial.println("Encoder: " + String(encoderValue));

    // Verifica se houve uma mudança no estado de toque para qualquer porta
    uint16_t currentlyTouched = touch.touched();
    if (currentlyTouched != lastTouched) {
      if (tela == 1){
        altura = encoderValue;
      }
      if(tela == 2){
        tela = 0;
      }
        for (uint8_t i = 0; i < 12; i++) {
            // Verifica se a porta atual mudou de estado
            if ((currentlyTouched & (1 << i)) != (lastTouched & (1 << i))) {
                if (currentlyTouched & (1 << i)) {
                    tela = 0; desligaTela = millis();
                    // Porta está sendo tocada agora
                    portaTocadaAnteriormente[i] = true;

                    // Determina a direção do toque
                    if (i > 0 && i < 11) {
                        // Transição em outras portas
                        if (portaTocadaAnteriormente[i - 1]) {
                            // Transição para uma porta maior (descendo)
                            if (altura > 55)
                                altura -= 1;
                        } else if (portaTocadaAnteriormente[i + 1]) {
                            // Transição para uma porta menor (subindo)
                            if (altura < 120)
                                altura += 1;
                        }
                    }
                    // Atualiza o tempo da última alteração
                    ultimaAlteracao = millis();
                } else {
                    // Porta não está mais sendo tocada
                    portaTocadaAnteriormente[i] = false;
                }
            }
        }

        lastTouched = currentlyTouched;
    }
    u8g2.firstPage();
    do {
        draw();
    } while (u8g2.nextPage());

    // Verifica se passaram 5 segundos desde a última alteração
    if (millis() - ultimaAlteracao > 1000 && encoderValue != altura ) {
        tela = 1;
        
    }

    if(encoderValue < altura && tela == 1){
      Serial.println("Mesa subindo");
      desligaTela = millis();
    }
    if(encoderValue > altura && tela == 1){
      Serial.println("Mesa descendo");
      desligaTela = millis();
    }
    if(encoderValue == altura && tela == 1){
      tela = 0;
      desligaTela = millis();
      Serial.println("Mesa parada");
    }
    if (millis() - desligaTela > 5000 && tela == 0) {
        tela = 2;
        Serial.println("Apagando display");
    }
    Serial.print(tela);
    Serial.print("   ");
}


