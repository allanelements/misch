#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_MPR121 touch;
unsigned int variavel = 90;
uint16_t lastTouched = 0;
bool portaTocadaAnteriormente[12] = {false};  // Array para rastrear o estado anterior de cada porta
unsigned long ultimaAlteracao = 0;
bool tela = true;

void draw(void) {
    u8g2.setFont(u8g2_font_inb30_mr);
    
    // Centraliza o valor da variável no meio da tela
    String variavelStr = String(variavel);
    const char* variavelChar = variavelStr.c_str();

    int x = (128 - u8g2.getStrWidth(variavelChar)) / 2;
    int y = 42;  // Altura central da tela
    
    if(tela == true) u8g2.drawStr(x, y, variavelChar);
    else {
      u8g2.drawStr(x, y, "  ");
      u8g2.clearBuffer(); // Limpa o buffer
      u8g2.sendBuffer();  // Envia o buffer limpo para o display
    }
}


void setup() {
    /*Serial.begin(115200);
    while (!Serial);*/

    Serial.println("MPR121 Capacitive Touch Test");

    if (!touch.begin()) {
        Serial.println("Error initializing MPR121");
        while (1);
    }
    touch.setThresholds(2, 0);  // Ajuste de limiar de toque e liberação

    u8g2.begin();
    
        ultimaAlteracao = millis();
}

void loop() {
    uint16_t currentlyTouched = touch.touched();

    // Verifica se houve uma mudança no estado de toque para qualquer porta
    if (currentlyTouched != lastTouched) {
        Serial.print("currentlyTouched: ");
        Serial.print(currentlyTouched, BIN);
        Serial.print("  lastTouched: ");
        Serial.print(lastTouched, BIN);
        Serial.print(" -  var: ");
        Serial.println(variavel);

        for (uint8_t i = 0; i < 12; i++) {
            // Verifica se a porta atual mudou de estado
            if ((currentlyTouched & (1 << i)) != (lastTouched & (1 << i))) {
                if (currentlyTouched & (1 << i)) {
                    tela = true;
                    // Porta está sendo tocada agora
                    portaTocadaAnteriormente[i] = true;

                    // Determina a direção do toque
                    if (i > 0 && i < 11) {
                        // Transição em outras portas
                        if (portaTocadaAnteriormente[i - 1]) {
                            // Transição para uma porta maior (descendo)
                            if (variavel > 55) variavel -= 1;
                        } else if (portaTocadaAnteriormente[i + 1]) {
                            // Transição para uma porta menor (subindo)
                            if (variavel < 120) variavel += 1;
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
    if (millis() - ultimaAlteracao > 2000) {
        tela = false;
        Serial.println("Apagar display");
    }
}
