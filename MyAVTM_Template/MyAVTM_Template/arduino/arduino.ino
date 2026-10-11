const int BUTTON_PIN = 4; // Pino do botão (D4)
const int BUZZER_PIN = 5; // Pino do Buzzer (D5)
const int LIGHT_SENSOR_PIN = A3; // Pino do sensor de luz (A3)

// Limite de luminosidade (acima é dia, abaixo é noite)
const int LIGHT_THRESHOLD = 511; 

int lastButtonState = LOW; // Guarda o estado anterior do botão (LOW e HIGH) 
bool isDay = true; // Guarda se atualmente é dia ou noite (serve para não mandar várias vezes a mensagem para o lightDemo.cpp)

// Notas musicais
#define NOTE_C4  262
#define NOTE_E4  330
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define REST     0

int deathMelody[] = {
  NOTE_B4, NOTE_F5, REST, NOTE_F5, 
  NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, 
  NOTE_E4, REST, NOTE_E4, NOTE_C4
};

int deathDurations[] = {
  8, 8, 8, 8,
  6, 6, 6, 8,
  8, 8, 8, 4
};

// Som curto de IMPACTO
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
    } else {
      noTone(BUZZER_PIN);
    }
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // 1. Receber comandos do lightDemo.cpp
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'I') {
      playHitSound();   // Comando 'I' -> Som de impacto
    }
    else if (cmd == 'G') {
      playDeathSound(); // Comando 'G' -> Som de Game Over do Mario
    }
  }

  // 2. BOTÃO (D4) - Controlo manual dos faróis
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == HIGH && lastButtonState == LOW) {
    Serial.print('H');
    delay(50); // Pequeno atraso para evitar leituras duplas (debounce) 
  }
  lastButtonState = buttonState;

  // 3. SENSOR DE LUZ (A3) - Controlo automático de Dia/Noite e Faróis
  int lightValue = analogRead(LIGHT_SENSOR_PIN); // Obtém o valor atual do sensor

  // Transição para DIA (Mundança de escuro -> claro)
  if (lightValue > LIGHT_THRESHOLD && !isDay) {
    isDay = true;
    Serial.print('D'); // Envia comando de Dia
  } 
  // Transição para NOITE (Mudança de claro -> escuro)
  else if (lightValue <= LIGHT_THRESHOLD && isDay) {
    isDay = false;
    Serial.print('N'); // Envia comando de Noite
  }
  delay(50); // Pequeno atraso para evitar leituras duplas (debounce) 
}