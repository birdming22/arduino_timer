#include <avr/io.h>
#include <avr/interrupt.h>

#define BAUD_RATE 9600
#define INPUT_PIN A0
#define LED_PIN 13
#define BUFFER_SIZE 8

//#define TEST_MODE                            // comment out to read analog pin, uncomment for test ramp wave
                          
volatile int j;
volatile int count = 1;
volatile unsigned char sequence = 0;
unsigned char buffer[BUFFER_SIZE * 2 + 2];
volatile int input = 0;
char ReceivedByte;

void setup()
{
  Serial.begin(BAUD_RATE);
  pinMode(LED_PIN, OUTPUT);
  
}

void loop() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    ReceivedByte = Serial.read();
    switch (ReceivedByte) //Which ASCII character was received?
    {
      case 'I': // start timer
          startTimer1();
          break;
      case 'O': // stop timer
          TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));          // clears all clock selects bits
          break;
      default:
          break;
    }
  }
}  

void startTimer1()
{
  count = 1;
  sequence = 0;
  
  cli();                                     // disable interrupts while messing with their settings
  TCCR1A = 0x00;                             // clear default timer settings, this kills the millis() funciton
  TCCR1B = 0x00;
  TCCR1B |= (1 << WGM12);                    // Configure timer 1 for CTC mode
  TCCR1B |= (1 << CS12);                     // Set timer prescaling by setting 3 bits
  TCCR1B |= (0 << CS11);                     // 001=1, 010=8, 011=64, 101=1024
  TCCR1B |= (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);                   // Enable CTC interrupt
  //
  OCR1A  = 0x3D08;                        
  // Set CTC compare value
  OCR1A  = 0x071A; // 8hz
  OCR1A  = 0x00F4; // 64hz
  OCR1A  = 0x007A; // 128hz
  
  buffer[0] = 0xFF; //sync byte
  
  sei();                                     // turn interrupts back on
}
ISR(TIMER1_COMPA_vect)                            // when timer counts down it fires this interrupt
{  
  #ifdef TEST_MODE
    Serial.write((j%64)*4);                // test mode, generate a ramp wave
    j++;
    
  #else
    if ((count % (BUFFER_SIZE+1)) == 0) {
      buffer[1] = sequence;
      Serial.write(buffer, BUFFER_SIZE * 2 + 2
      );
      if (sequence == 127)
        sequence = 0; // reset sequence
      else
        sequence++;
      count = 1;
      //Serial.write( analogRead(INPUT_PIN));   // real mode, sample analog pin
    }
    input = analogRead(INPUT_PIN);
    buffer[count * 2] = input &0x1F;
    buffer[count * 2 + 1] = input >>5;
    count++;
  #endif  
}

