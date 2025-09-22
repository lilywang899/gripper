// L293D connections
#define PIN_EN 18 // Enable pin (motor speed control)
#define PIN_IN1 19 // Motor input 1
#define PIN_IN2 21 // Motor input 2
#define POT_PIN 34 // ADC

int potValue = 0;
int mappedValue = 0;
int targetPos = -1;
const int deadband = 300;
bool goingDown = true;

const int freq = 500;
const int res = 8;


void setup() {

Serial.begin(115200);


ledcAttach(PIN_IN1, freq, res);
ledcAttach(PIN_IN2, freq, res);


}

void loop() {




// read pot
potValue = analogRead(POT_PIN);
mappedValue = map(potValue, 0, 4095, 0, 180);


if(mappedValue >= 140) { // middle

ledcWrite(PIN_IN1, 255);
ledcWrite(PIN_IN2, 0);

}
else if ( mappedValue <= 130){ // closed

ledcWrite(PIN_IN1, 0);
ledcWrite(PIN_IN2, 255);

}

Serial.println("Pot: ");
Serial.print(mappedValue);


delay(200);


}

