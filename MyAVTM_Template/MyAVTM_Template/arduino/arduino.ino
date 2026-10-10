const int BUTTON_PIN = 4; // Pino do botão no Sensor Kit (D4)
int lastState = LOW; // Guarda o estado anterior do botão (LOW e HIGH)

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  // 1: BOTÃO (D4)
  int currentState = digitalRead(BUTTON_PIN); // Estado atual do botão
  // Deteta apenas o clique (transição de LOW para HIGH)
  if (currentState == HIGH && lastState == LOW) {
    Serial.print('H'); // Envia um sinal para ativar as headlights
    delay(50); // Pequeno atraso para evitar leituras duplas (debounce)
  }
  lastState = currentState;
}