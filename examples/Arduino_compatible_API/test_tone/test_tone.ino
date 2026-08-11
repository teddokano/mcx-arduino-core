#include <Arduino.h>

#define BUZZER_PIN  D13

// "Twinkle Twinkle Little Star" (first phrase)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440

int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4,
  NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4,
  NOTE_D4, NOTE_D4, NOTE_C4,
};

// note length: 4 = quarter note, 2 = half note (matches classic Arduino toneMelody example)
int noteDurations[] = {
  4, 4, 4, 4,
  4, 4, 2,
  4, 4, 4, 4,
  4, 4, 2,
};

const int numNotes = sizeof(melody) / sizeof(melody[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("tone / noTone melody test");
}

void loop() {
  for (int i = 0; i < numNotes; i++) {
    int noteDuration = 1000 / noteDurations[i];

    tone(BUZZER_PIN, melody[i], noteDuration);

    Serial.print("note ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(melody[i]);

    // pause between notes, ~30% longer than the note itself
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    noTone(BUZZER_PIN);
  }

  delay(1000); // pause before repeating
}
