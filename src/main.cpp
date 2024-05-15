#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <U8g2lib.h>
#include <EEPROM.h>

int endereco = 0; // Endereço na EEPROM para armazenar a variável
const int hallPin = 4;

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_MPR121 touch;
unsigned int altura = 90;
uint16_t lastTouched = 0;
bool portaTocadaAnteriormente[12] = {false};
unsigned long ultimaAlteracao = 0;
unsigned long desligaTela = 0;
int tela = 0;
int hallValue;

void salvarAltura() {
  EEPROM.begin(512); // Inicia a EEPROM
  EEPROM.put(endereco, hallValue); // Grava a variável no endereço na EEPROM
  EEPROM.commit(); // Finaliza o uso da EEPROM
}

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
    String variavelStr = String(hallValue);
    const char* variavelChar = variavelStr.c_str();
    // Combina as strings da variável e das setas
    String textoCompleto = variavelStr + "   " + altura ;
    int x = (128 - u8g2.getStrWidth(textoCompleto.c_str())) / 2;
    int y = 42;  // Altura central da tela
    u8g2.drawStr(x, y, textoCompleto.c_str());
    // Desenha uma seta para cima
    u8g2.setDrawColor(1); // Define a cor do desenho como preto (1)
    u8g2.setDrawColor(2); // Define a espessura da linha como 2 pixels
    
    if(hallValue > altura){
      u8g2.drawLine(53, 30, 63, 40); // Linha diagonal inferior da seta
      u8g2.drawLine(63, 40, 73, 30); // Linha diagonal superior da seta
    }
     if (hallValue < altura){
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
  EEPROM.begin(512); // Inicia a EEPROM
  EEPROM.get(endereco, hallValue); // Lê a variável do endereço na EEPROM
  EEPROM.end();
  Serial.print("Altura salva: ");
  Serial.println(hallValue);

    pinMode(12,OUTPUT);
    pinMode(14,OUTPUT);

    Serial.begin(115200);
  

    Serial.println("MPR121 Capacitive Touch Test");

    if (!touch.begin()) {
        Serial.println("Error initializing MPR121");
        while (1);
    }
    touch.setThresholds(2, 2);  // Ajuste de limiar de toque e liberação

    // Configurando os pinos do encoder
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);

    u8g2.begin();
    ultimaAlteracao = millis();
}

void loop() {

    // Verifica se houve uma mudança no estado de toque para qualquer porta
    uint16_t currentlyTouched = touch.touched();
    if (currentlyTouched != lastTouched) {
    
      if (tela == 1){
      
        altura = hallValue;
      }
      
        for (uint8_t i = 0; i < 12; i++) {
            // Verifica se a porta atual mudou de estado
            if ((currentlyTouched & (1 << i)) != (lastTouched & (1 << i))) {
             
                if (currentlyTouched & (1 << i)) {
              
                    // Porta está sendo tocada agora
                    portaTocadaAnteriormente[i] = true;

                    // Determina a direção do toque
                    if (i > 0 && i < 11) {
                        // Transição em outras portas
                        if (portaTocadaAnteriormente[i - 1] && tela == 0) {
                        
                            // Transição para uma porta maior (descendo)
                            if (altura > 55)
                                altura -= 1;
                        } else if (portaTocadaAnteriormente[i + 1] && tela == 0) {
                          
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
                    tela = 0; desligaTela = millis();
                    if(tela == 2){
                     
                      tela = 0;
                    }
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
    if (millis() - ultimaAlteracao > 1250 && hallValue != altura ) {
        tela = 1;
        
    }

    if(hallValue < altura && tela == 1){
      mesaUp();
      desligaTela = millis();
    }
    if(hallValue > altura && tela == 1){
      mesaDown();
      desligaTela = millis();
    }
    if(hallValue == altura && tela == 1){
      tela = 0;
      desligaTela = millis();
      mesaStop();
    }
    if (millis() - desligaTela > 3000 && tela == 0) {
        tela = 2;

    }
    
    
}

void mesaUp(){
  digitalWrite(12,HIGH);
  salvarAltura();
}

void mesaDown(){
  digitalWrite(14,HIGH);
  salvarAltura();
}
void mesaStop(){
  digitalWrite(12,LOW);
  digitalWrite(14,LOW);

}


