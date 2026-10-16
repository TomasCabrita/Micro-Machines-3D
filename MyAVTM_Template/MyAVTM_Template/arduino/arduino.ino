#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TCS34725.h>
#include <math.h>

// ==============================
// PINOS
// ==============================

const int BUTTON_PIN = 4; // Pino do botão (D4)
const int BUZZER_PIN = 5; // Pino do Buzzer (D5)
const int LIGHT_SENSOR_PIN = A3; // Pino do sensor de luz (A3)

const int LIGHT_THRESHOLD = 337; // Limite de luminosidade adaptado para alimentação a 3.3V (acima é dia, abaixo é noite)

// ==============================
// MPU6050 (GIROSCÓPIO / ACELERÓMETRO)
// ==============================

Adafruit_MPU6050 mpu; // Instância do sensor MPU6050
bool mpuOn = false; // Estado de conexão do MPU6050

unsigned long lastMpuCheck = 0; // Guarda o tempo da última tentativa de conexão do MPU6050
unsigned long lastMpuRead = 0; // Guarda o tempo da última leitura dos dados de inclinação

const unsigned long MPU_CHECK_INTERVAL = 3000; // Intervalo de tempo entre tentativas de reconexão (3 segundos)
const unsigned long MPU_READ_INTERVAL = 100; // Intervalo de tempo entre leituras de inclinação (100 ms)

const float limit = 20.0; // Limite de inclinação em graus (threshold) para acionar os movimentos
char lastDirection = 'X'; // Última direção enviada ('W', 'S', 'A', 'D' ou 'X'), usada para detetar mudanças

// ==============================
// TCS34725 (SENSOR DE CORES)
// ==============================

// Instância do sensor TCS34725 com tempo de integração de 50ms e ganho de 4x
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

bool tcsOn = false; // Estado de conexão do sensor de cor TCS34725

unsigned long lastTcsCheck = 0; // Guarda o tempo da última tentativa de conexão do TCS34725
unsigned long lastTcsRead = 0; // Guarda o tempo da última leitura de cor

const unsigned long TCS_CHECK_INTERVAL = 3000; // Intervalo de tempo para verificar/reconectar o sensor de cor (3 segundos)
const unsigned long TCS_READ_INTERVAL = 300; // Intervalo de tempo entre leituras do sensor de cor (300 ms)

char lastColorSent = 'O'; // Guarda a última cor confirmada e enviada via Serial ('R' = Vermelho, 'G' = Verde, 'Y' = Amarelo, 'O' = Outra/Pouca Luz)
char colorCandidate = 'O'; // Cor candidata a ser confirmada nas leituras consecutivas
int colorCount = 0; // Contador de leituras seguidas iguais para validação da cor
const int COLOR_CONFIRMATIONS = 3; // Número de leituras consecutivas necessárias para confirmar a alteração da cor

// ==============================
// BOTÃO E LUZ
// ==============================

int lastButtonState = LOW; // Guarda o estado anterior do botão (LOW e HIGH)
int lightState = 1; // Estado atual da iluminação ambiente (1 = dia, 0 = noite)

// ==============================
// NOTAS MUSICAIS E MELODIAS
// ==============================

#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define REST 0

// Melodia reproduzida no Game Over (estilo Super Mario)
int deathMelody[] = {
  NOTE_B4, NOTE_F5, REST, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_E4, REST, NOTE_E4, NOTE_C4
};

// Duração relativa de cada nota musical
int deathDurations[] = {
  8, 8, 8, 8,
  6, 6, 6, 8,
  8, 8, 8, 4
};

// ==============================
// SONS E EFETOS SONOROS
// ==============================

// Som curto de IMPACTO (reproduzido ao colidir com obstáculos)
void playHitSound() {
  tone(BUZZER_PIN, 150, 40); // Frequência baixa (150Hz) por 40ms
  delay(45);
  tone(BUZZER_PIN, 90, 60);  // Frequência mais baixa (90Hz) por 60ms
  delay(65);
  noTone(BUZZER_PIN);
}

// Som de GAME OVER (Super Mario)
void playDeathSound() {
  int totalNotes = sizeof(deathMelody) / sizeof(deathMelody[0]);

  for (int i = 0; i < totalNotes; i++) {
    int noteDuration = 1000 / deathDurations[i];

    if (deathMelody[i] != REST) {
      tone(BUZZER_PIN, deathMelody[i], noteDuration);
    }
    else {
      noTone(BUZZER_PIN);
    }

    int pauseBetweenNotes = noteDuration * 1.30; // Pausa de 30% entre as notas para articulação
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
  }
}

// ==============================
// INICIALIZAÇÃO DOS SENSORES I2C
// ==============================

// Inicialização e configuração do MPU6050 nos endereços I2C 0x68 ou 0x69
void initMPU() {
  if (mpu.begin(0x68, &Wire) || mpu.begin(0x69, &Wire)) {
    mpuOn = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G); // Alcance do acelerómetro até ±2G
    mpu.setGyroRange(MPU6050_RANGE_250_DEG); // Alcance do giroscópio até ±250 deg/s
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Filtro passa-baixo a 21Hz para estabilização da leitura
  }
  else {
    mpuOn = false; // Sensor não encontrado no barramento I2C
  }
}

// Inicialização do sensor de cores TCS34725
void initTCS() {
  tcsOn = tcs.begin(); // Retorna true se o sensor de cor for encontrado e iniciado com sucesso
}

// ==============================
// LEITURA E PROCESSAMENTO DA INCLINAÇÃO (MPU6050)
// ==============================

void readInclination() {
  if (!mpuOn) return;

  sensors_event_t a, g, temp;

  // Tenta obter os dados do sensor; se falhar, marca como desligado
  if (!mpu.getEvent(&a, &g, &temp)) {
    mpuOn = false;
    lastDirection = 'X';
    return;
  }

  // Cálculo do Pitch (Frente / Trás) em graus
  float pitch = atan2(
    -a.acceleration.x,
    sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)
  ) * 180.0 / PI;

  // Cálculo do Roll (Esquerda / Direita) em graus
  float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;

  // Proteção contra valores inválidos (Not a Number)
  if (isnan(pitch) || isnan(roll)) return;

  char direction = 'X'; // Direção padrão: Posição horizontal / Neutro

  // Processamento de Frente / Trás (Pitch)
  if (abs(pitch) > limit && abs(pitch) >= abs(roll)) {
    direction = (pitch > 0) ? 'W' : 'S'; // 'W' = Frente, 'S' = Trás
  }
  // Processamento de Esquerda / Direita (Roll)
  else if (abs(roll) > limit) {
    direction = (roll > 0) ? 'D' : 'A'; // 'D' = Direita, 'A' = Esquerda
  }

  // Envia quando a direção muda, e reenvia a cada 500 ms enquanto estiver inclinado
  // (assim o jogo volta a sincronizar se arrancar depois do Arduino)
  static unsigned long lastDirectionSend = 0;
  unsigned long nowMs = millis();

  bool changed = (direction != lastDirection);
  bool resend  = (direction != 'X') && (nowMs - lastDirectionSend >= 500);

  if (changed || resend) {
    Serial.print(direction);
    lastDirection = direction;
    lastDirectionSend = nowMs;
  }
}

// ==============================
// LEITURA E PROCESSAMENTO DA COR (TCS34725)
// ==============================

void readColour() {
  if (!tcsOn) return;

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c); // Obtém os valores brutos dos canais Vermelho (R), Verde (G), Azul (B) e Claro/Luminosidade (C)

  char color = 'O';  // 'O' = Outra cor / pouca luz ambiente

  // Verificação de luminosidade insuficiente no sensor
  if (c < 100) {
    color = 'O';
  }
  // Deteção da cor Vermelha (R significativamente superior a G e B)
  else if (r > g * 1.5 && r > b * 1.5) {
    color = 'R'; // 'R' = Vermelho
  }
  // Deteção da cor Amarela (R e G altos e parecidos, B bastante mais baixo)
  else if (r > b * 1.5 && g > b * 1.5 &&
           abs((int)r - (int)g) < max(r, g) * 0.25) {
    color = 'Y'; // 'Y' = Amarelo
  }
  // Deteção da cor Verde (G claramente superior a R e B)
  else if (g > r * 1.3 && g > b * 1.2) {
    color = 'G'; // 'G' = Verde
  }

  // Filtro de confirmação: Só aceita a nova cor após várias leituras consecutivas iguais
  if (color == colorCandidate) {
    if (colorCount < COLOR_CONFIRMATIONS) colorCount++;
  }
  else {
    colorCandidate = color; // Nova cor detetada, reinicia o contador
    colorCount = 1;
  }

  // Envia a cor confirmada via porta série caso seja diferente da última enviada
  if (colorCount >= COLOR_CONFIRMATIONS && colorCandidate != lastColorSent) {
    lastColorSent = colorCandidate;
    Serial.print(lastColorSent); // Envia 'R', 'G', 'Y' ou 'O' para o lightDemo.cpp
  }
}

// ==============================
// SETUP (CONFIGURAÇÃO INICIAL)
// ==============================

void setup() {
  Serial.begin(9600); // Configuração da taxa de transmissão Série a 9600 baud para comunicação com o lightDemo.cpp

  pinMode(BUTTON_PIN, INPUT);   // Configura o pino do botão como entrada
  pinMode(BUZZER_PIN, OUTPUT);  // Configura o pino do buzzer como saída

  Wire.begin(); // Inicializa o barramento de comunicação I2C
  Wire.setClock(100000); // Define a frequência do barramento I2C a 100 kHz (Standard Mode)
  Wire.setWireTimeout(25000, true); // PREVINE CONGELAMENTOS: Liberta o I2C se travar por mais de 25ms

  delay(1000); // Pequeno atraso para estabilização de energia dos módulos

  initMPU(); // Tenta iniciar o giroscópio/acelerómetro MPU6050
  initTCS(); // Tenta iniciar o sensor de cor TCS34725

  lastMpuCheck = millis(); // Inicializa os temporizadores de verificação periódica
  lastTcsCheck = millis();
}

// ==============================
// LOOP PRINCIPAL
// ==============================

void loop() {
  // 1. RECEBER COMANDOS DO JOGO (lightDemo.cpp)
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'I') playHitSound(); // Comando 'I' -> Som de impacto
    else if (cmd == 'G') playDeathSound(); // Comando 'G' -> Som de Game Over do Mario
  }

  // 2. BOTÃO MANUAL DOS FARÓIS (D4)
  int buttonState = digitalRead(BUTTON_PIN);

  // Deteta a transição de LOW para HIGH (pressionar do botão)
  if (buttonState == HIGH && lastButtonState == LOW) {
    Serial.print('H'); // Envia comando 'H' para alternar o estado dos faróis no C++
    delay(50); // Pequeno atraso para evitar leituras duplas (debounce)
  }

  lastButtonState = buttonState; // Atualiza o estado anterior do botão

  // 3. SENSOR DE LUZ A3 (Controlo automático de Dia/Noite)
  int lightValue = analogRead(LIGHT_SENSOR_PIN); // Obtém o valor analógico atual do sensor LDR

  // Transição para DIA (Luminosidade superior ao limite)
  if (lightValue > LIGHT_THRESHOLD && lightState != 1) {
    lightState = 1;
    Serial.print('T');   // Envia o comando 'T' para ativar o Modo Dia no C++
  }
  // Transição para NOITE (Luminosidade inferior ou igual ao limite)
  else if (lightValue <= LIGHT_THRESHOLD && lightState != 0) {
    lightState = 0;
    Serial.print('N');   // Envia o comando 'N' para ativar o Modo Noite no C++
  }

  // 4. GESTÃO E LEITURA DOS SENSORES I2C (MPU6050 e TCS34725) SEM BLOQUEAR O LOOP
  unsigned long now = millis(); // Obtém o tempo decorrido desde o arranque do Arduino

  // Gestão do MPU6050
  if (mpuOn) {
    if (now - lastMpuRead >= MPU_READ_INTERVAL) {
      lastMpuRead = now;
      readInclination(); // Realiza a leitura da inclinação e envio da direção
    }
  }
  else if (now - lastMpuCheck >= MPU_CHECK_INTERVAL) {
    lastMpuCheck = now;
    initMPU(); // Tenta reconectar o MPU6050 caso este tenha falhado
  }

  // Gestão do TCS34725
  if (tcsOn) {
    if (now - lastTcsRead >= TCS_READ_INTERVAL) {
      lastTcsRead = now;
      readColour(); // Realiza a leitura da cor e envio da respetiva letra
    }
  }
  else if (now - lastTcsCheck >= TCS_CHECK_INTERVAL) {
    lastTcsCheck = now;
    initTCS(); // Tenta reconectar o TCS34725 caso este tenha falhado
  }

  delay(10); // Pequeno atraso para estabilização do loop principal
}