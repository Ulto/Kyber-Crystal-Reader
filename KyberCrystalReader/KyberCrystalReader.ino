#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN     8
#define LED_COUNT   1
#define BRIGHTNESS 50

#define RFID_PWR_PIN    12
#define RFID_START_FLAG 0x02
#define RFID_END_FLAG   0x03


// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


/*
  link between the computer and the SoftSerial Shield
  at 9600 bps 8-N-1
  Computer is connected to Hardware UART
*/
SoftwareSerial SoftSerial(5, 6); //5 is Arduino RX / RFID TX


unsigned char buffer[64];       // buffer array for data receive over serial port
int count = 0;                  // counter for buffer array

unsigned long lastRFIDRestartMillis = 0;        // will store last time the RFID module was restarted
unsigned long lastLEDUpdateMillis = 0;          // will store last time an RFID color was read



unsigned long lastRestartMillis = 0;


void setup()
{
  SoftSerial.begin(9600);     // the SoftSerial baud rate
  Serial.begin(9600);         // the Serial port of Arduino baud rate.
  
  pinMode(RFID_PWR_PIN, OUTPUT);    // Set the RFID power pin to an output, then turn it on
  digitalWrite(RFID_PWR_PIN, HIGH);

  
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)
}



void loop()
{
  unsigned long currentMillis = millis();
  int gotRead = 0;  // set to 1 if a tag read was found

  if (currentMillis - lastRFIDRestartMillis >= 1000) {

    lastRFIDRestartMillis = currentMillis;

    digitalWrite(12, LOW);
    delay(5);
    digitalWrite(12, HIGH);

    lastRestartMillis = millis();
  }

  if (SoftSerial.available())          // reading data into char array
  {
    unsigned char inVal = "0";
    inVal = SoftSerial.read();
    //Serial.write(inVal);
    if (inVal == RFID_END_FLAG)
    {
      gotRead = 1;
    }
    else if (inVal == RFID_START_FLAG)
    {
      clearBufferArray();
      count = 0;
    }
    else
      buffer[count++] = inVal;

  }

  

  if(millis() - lastLEDUpdateMillis >= 3000)
  {
    strip.clear();
    strip.show();


  }

  if (gotRead)
  {
  
    unsigned char tagOnly[10];
    gotRead = 0;

    for (int i = 0; i < 10; i++)
      tagOnly[i] = buffer[i];

    //Serial.write(tagOnly, 10);     // if no data transmission ends, write buffer to hardware serial port
    //Serial.println("");

    if (strcmp(tagOnly, "0000000C03") == 0)
    {
      strip.fill(strip.Color(255, 255, 0, strip.gamma8(0)));
      strip.show();
      Serial.println("Yellow");
    }
    else if (strcmp(tagOnly, "0000000C0E") == 0)
    {
      strip.fill(strip.Color(0, 0, 255, strip.gamma8(0)));
      strip.show();
      Serial.println("Blue");
    }
    else if (strcmp(tagOnly, "0000000C00") == 0)
    {
      strip.fill(strip.Color(0, 0, 0, strip.gamma8(255)));
      strip.show();
      Serial.println("White");
    }
       else if (strcmp(tagOnly, "0000000C08") == 0)
    {
      strip.fill(strip.Color(0, 0, 0, strip.gamma8(255)));
      strip.show();
      Serial.println("White");
    }

    lastLEDUpdateMillis = millis();        // will store last time LED was updated

    clearBufferArray();             // call clearBufferArray function to clear the stored data from the array
    count = 0;                      // set counter of while loop to zero
  }
}
//if (Serial.available())             // if data is available on hardware serial port ==> data is coming from PC or notebook
//SoftSerial.write(Serial.read());    // write it to the SoftSerial shield

void clearBufferArray()                 // function to clear buffer array
{
  // clear all index of array with command NULL
  for (int i = 0; i < count; i++)
  {
    buffer[i] = NULL;
  }
}

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void whiteOverRainbow(int whiteSpeed, int whiteLength) {

  if (whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int      head          = whiteLength - 1;
  int      tail          = 0;
  int      loops         = 3;
  int      loopNum       = 0;
  uint32_t lastTime      = millis();
  uint32_t firstPixelHue = 0;

  for (;;) { // Repeat forever (or until a 'break' or 'return')
    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
      if (((i >= tail) && (i <= head)) ||     //  If between head & tail...
          ((tail > head) && ((i >= tail) || (i <= head)))) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, 255)); // Set white
      } else {                                             // else set rainbow
        int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
      }
    }

    strip.show(); // Update strip with new contents
    // There's no delay here, it just runs full-tilt until the timer and
    // counter combination below runs out.

    firstPixelHue += 40; // Advance just a little along the color wheel

    if ((millis() - lastTime) > whiteSpeed) { // Time to update head/tail?
      if (++head >= strip.numPixels()) {     // Advance head, wrap around
        head = 0;
        if (++loopNum >= loops) return;
      }
      if (++tail >= strip.numPixels()) {     // Advance tail, wrap around
        tail = 0;
      }
      lastTime = millis();                   // Save time of last movement
    }
  }
}

void pulseWhite(uint8_t wait) {
  for (int j = 0; j < 256; j++) { // Ramp up from 0 to 255
    // Fill entire strip with white at gamma-corrected brightness level 'j':
    strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
    strip.show();
    delay(wait);
  }

  for (int j = 255; j >= 0; j--) { // Ramp down from 255 to 0
    strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
    strip.show();
    delay(wait);
  }
}

void rainbowFade2White(int wait, int rainbowLoops, int whiteLoops) {
  int fadeVal = 0, fadeMax = 100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for (uint32_t firstPixelHue = 0; firstPixelHue < rainbowLoops * 65536;
       firstPixelHue += 256) {

    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());

      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,
                                           255 * fadeVal / fadeMax)));
    }

    strip.show();
    delay(wait);

    if (firstPixelHue < 65536) {                             // First loop,
      if (fadeVal < fadeMax) fadeVal++;                      // fade in
    } else if (firstPixelHue >= ((rainbowLoops - 1) * 65536)) { // Last loop,
      if (fadeVal > 0) fadeVal--;                            // fade out
    } else {
      fadeVal = fadeMax; // Interim loop, make sure fade is at max
    }
  }

  for (int k = 0; k < whiteLoops; k++) {
    for (int j = 0; j < 256; j++) { // Ramp up 0 to 255
      // Fill entire strip with white at gamma-corrected brightness level 'j':
      strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
      strip.show();
    }
    delay(1000); // Pause 1 second
    for (int j = 255; j >= 0; j--) { // Ramp down 255 to 0
      strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
      strip.show();
    }
  }

  delay(500); // Pause 1/2 second
}
