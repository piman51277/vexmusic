#define clk_pin 2   // ADI A
#define data1_pin 3 // ADI B
#define data2_pin 4 // ADI C
#define data3_pin 5 // ADI D
#define data4_pin 6 // ADI E
#define data5_pin 7 // ADI F
#define data6_pin 8 // ADI G
#define data7_pin 9 // ADI H
#define buzzer_pin 10

/**
 * Musical Note frequencies (starting from C1)
 * C1 = 32.70 Hz
 * C#1 = 34.65 Hz
 * D1 = 36.71 Hz
 * D#1 = 38.89 Hz
 * E1 = 41.20 Hz
 * F1 = 43.65 Hz
 * F#1 = 46.25 Hz
 * G1 = 49.00 Hz
 * G#1 = 51.91 Hz
 * A1 = 55.00 Hz
 * A#1 = 58.27 Hz
 * B1 = 61.74 Hz
 */

// The frequencies of C1 -> B1
const float frequencyBase[12] = {32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 55.00, 58.27, 61.74};

/**
 * Plays a note.
 * the ID is number of half-steps above C1
 */
void playNote(uint8_t id)
{
  // if id is 60, stop playing
  if (id == 60)
  {
    Serial.println("Rest");
    noTone(buzzer_pin);
    return;
  }

  int note = id % 12;
  int octave = id / 12;

  float frequency = frequencyBase[note] * pow(2.0, (double)octave);
  Serial.print("Playing note ");
  Serial.print(id);
  Serial.print(" (");
  Serial.print(frequency);
  Serial.println(" Hz)");

  tone(buzzer_pin, frequency);
}

void setup()
{
  // Serial to 115200
  Serial.begin(115200);

  // Clock
  pinMode(clk_pin, INPUT);

  // Data in
  pinMode(data1_pin, INPUT); // new note
  pinMode(data2_pin, INPUT);
  pinMode(data3_pin, INPUT);
  pinMode(data4_pin, INPUT);
  pinMode(data5_pin, INPUT);
  pinMode(data6_pin, INPUT);
  pinMode(data7_pin, INPUT);

  // Buzzer
  pinMode(buzzer_pin, OUTPUT);
}

uint8_t decodeInput()
{
  uint8_t result = 0;
  result |= digitalRead(data2_pin) << 0;
  result |= digitalRead(data3_pin) << 1;
  result |= digitalRead(data4_pin) << 2;
  result |= digitalRead(data5_pin) << 3;
  result |= digitalRead(data6_pin) << 4;
  result |= digitalRead(data7_pin) << 5;

  return result;
}

bool clockState = false;
long long lastInputTime = millis();

void loop()
{
  // has the clock signal changed?
  if (digitalRead(clk_pin) != clockState)
  {

    lastInputTime = millis();
    // update clock state
    clockState = !clockState;

    // read input
    uint8_t input = decodeInput();

    // is it a new note?
    if (digitalRead(data1_pin) == HIGH)
    {
      // add a short delay
      noTone(buzzer_pin);
      delay(5);
    }

    // play note
    playNote(input);
  }

  // mute it if no input for 1 second
  if (millis() - lastInputTime > 1000)
  {
    noTone(buzzer_pin);
  }

  // delay
  delay(1);
}
