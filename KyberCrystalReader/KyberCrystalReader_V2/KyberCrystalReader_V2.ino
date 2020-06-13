// EM4095.ino: Base firmware
//
// Copyright 2018 Alexandro Todeschini
// inspiraded from RFIDino and others library
// library is free to use
// released under GNU (see file)
//https://github.com/asetyde/EM4095

#include <Adafruit_NeoPixel.h>

#define DELAYVAL    384   //384 //standard delay for manchster decode
#define TIMEOUT     10000 //standard timeout for manchester decode at  160mhz

// pin configuration
#define LED_PIN     2
#define DEMODOUT  5
#define SHD  13
#define MOD  15
#define RDYCLK  12


#define BRIGHTNESS 100
#define LED_COUNT   3


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




byte tagData[5]; //Holds the ID numbers from the tag


//Manchester decode. Supply the function an array to store the tags ID in
bool decodeTag(unsigned char *buf)
{
  unsigned char i = 0;
  unsigned short timeCount;
  unsigned char timeOutFlag = 0;
  unsigned char row, col;
  unsigned char row_parity;
  unsigned char col_parity[5];
  unsigned char dat;
  unsigned char j;

  while (1)
  {
    timeCount = 0;
    while (0 == digitalRead(DEMODOUT)) //watch for DEMODOUT to go low
    {

      if (timeCount >= TIMEOUT) //if we pass TIMEOUT milliseconds, break out of the loop
      {
        break;
      }
      else
      {
        timeCount++;
      }
    }

    if (timeCount >= 600)
    {
      return false;
    }
    timeCount = 0;

    delayMicroseconds(DELAYVAL);
    if (digitalRead(DEMODOUT))
    {


      for (i = 0; i < 8; i++) // 9 header bits
      {
        timeCount = 0; //restart counting
        while (1 == digitalRead(DEMODOUT)) //while DEMOD out is high
        {
          if (timeCount == TIMEOUT)
          {
            timeOutFlag = 1;
            break;
          }
          else
          {
            timeCount++;
          }
        }

        if (timeOutFlag)
        {
          break;
        }
        else
        {
          delayMicroseconds(DELAYVAL);
          if ( 0 == digitalRead(DEMODOUT) )
          {
            break;
          }
        }
      }//end for loop

      if (timeOutFlag)
      {
        timeOutFlag = 0;
        return false;
      }

      if (i == 8) //Receive the data
      {

        timeOutFlag = 0;
        timeCount = 0;
        while (1 == digitalRead(DEMODOUT))
        {
          if (timeCount == TIMEOUT)
          {
            timeOutFlag = 1;
            break;
          }
          else
          {
            timeCount++;
          }

          if (timeOutFlag)
          {
            timeOutFlag = 0;
            return false;
          }
        }

        col_parity[0] = col_parity[1] = col_parity[2] = col_parity[3] = col_parity[4] = 0;
        for (row = 0; row < 11; row++)
        {
          row_parity = 0;
          j = row >> 1;

          for (col = 0, row_parity = 0; col < 5; col++)
          {
            delayMicroseconds(DELAYVAL);
            if (digitalRead(DEMODOUT))
            {
              dat = 1;
            }
            else
            {
              dat = 0;
            }

            if (col < 4 && row < 10)
            {
              buf[j] <<= 1;
              buf[j] |= dat;
            }

            row_parity += dat;
            col_parity[col] += dat;
            timeCount = 0;
            while (digitalRead(DEMODOUT) == dat)
            {
              if (timeCount == TIMEOUT)
              {
                timeOutFlag = 1;
                break;
              }
              else
              {
                timeCount++;
              }
            }
            if (timeOutFlag)
            {
              break;
            }
          }

          if (row < 10)
          {
            if ((row_parity & 0x01) || timeOutFlag) //Row parity
            {
              timeOutFlag = 1;
              break;
            }
          }
        }

        if ( timeOutFlag || (col_parity[0] & 0x01) || (col_parity[1] & 0x01) || (col_parity[2] & 0x01) || (col_parity[3] & 0x01) ) //Column parity
        {
          timeOutFlag = 0;
          return false;
        }
        else
        {
          return true;
        }

      }//end if(i==8)

      return false;

    }//if(digitalRead(DEMODOUT))
  } //while(1)
}


//function to compare 2 byte arrays. Returns true if the two arrays match, false of any numbers do not match
bool compareTagData(byte *tagData1, byte *tagData2)
{
  for (int j = 0; j < 5; j++)
  {
    if (tagData1[j] != tagData2[j])
    {
      return false; //if any of the ID numbers are not the same, return a false
    }
  }

  return true;  //all id numbers have been verified
}

//function to transfer one byte array to a secondary byte array.
//source -> tagData
//destination -> tagDataBuffer
void transferToBuffer(byte *tagData, byte *tagDataBuffer)
{
  for (int j = 0; j < 5; j++)
  {
    tagDataBuffer[j] = tagData[j];
  }
}

bool scanForTag(byte *tagData)
{
  static byte tagDataBuffer[5];      //A Buffer for verifying the tag data. 'static' so that the data is maintained the next time the loop is called
  static int readCount = 0;          //the number of times a tag has been read. 'static' so that the data is maintained the next time the loop is called
  boolean verifyRead = false; //true when a tag's ID matches a previous read, false otherwise
  boolean tagCheck = false;   //true when a tag has been read, false otherwise

  tagCheck = decodeTag(tagData); //run the decodetag to check for the tag
  if (tagCheck == true) //if 'true' is returned from the decodetag function, a tag was succesfully scanned
  {

    readCount++;      //increase count since we've seen a tag

    if (readCount == 1) //if have read a tag only one time, proceed
    {
      transferToBuffer(tagData, tagDataBuffer);  //place the data from the current tag read into the buffer for the next read
    }
    else if (readCount == 2) //if we see a tag a second time, proceed
    {
      verifyRead = compareTagData(tagData, tagDataBuffer); //run the checkBuffer function to compare the data in the buffer (the last read) with the data from the current read

      if (verifyRead == true) //if a 'true' is returned by compareTagData, the current read matches the last read
      {
        readCount = 0; //because a tag has been succesfully verified, reset the readCount to '0' for the next tag
        return true;
      }
    }
  }
  else
  {
    return false;
  }
  return true;
}



void  setup() {

  //set pin MODes on RFID pins
  pinMode(MOD, OUTPUT);
  pinMode(SHD, OUTPUT);
  pinMode(DEMODOUT, INPUT);
  pinMode(RDYCLK, INPUT);
  //set SHD and MOD low to prepare for reading
  digitalWrite(SHD, LOW);
  digitalWrite(MOD, LOW);

  Serial.begin(115200);
  Serial.println("Welcome. Please swipe your RFID Tag.");

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)

}

byte tagDataTest[5] = {0x00, 0x00, 0x00, 0x0C, 0x0E};

typedef enum
{
  blue1,
  blue2,
  purple1,
  purple2,
  red1,
  red2,
  red3,
  red4,
  red5,
  yellow1,
  yellow2,
  white1,
  white2,
  green1,
  green2,
  green3,
  black,
  last

} color_t;

const char* colorNames[] = 
{
  "blue1",
  "blue2",
  "purple1",
  "purple2",
  "red1",
 "red2",
  "red3",
  "red4",
  "red5",
  "yellow1",
  "yellow2",
 " white1",
 " white2",
  "green1",
  "green2",
 " green3",
  "black",
  "last"
};


byte KyberCrystalUID [][5] =
{
  {0x00, 0x00, 0x00, 0x0C, 0x0E},
  {0x00, 0x00, 0x00, 0x0C, 0x06},
  {0x00, 0x00, 0x00, 0x0C, 0x0F},
  {0x00, 0x00, 0x00, 0x0C, 0x07},
  {0x00, 0x00, 0x00, 0x0C, 0x0D},
  {0x00, 0x00, 0x00, 0x0C, 0x0A},
  {0x00, 0x00, 0x00, 0x0C, 0x09},
  {0x00, 0x00, 0x00, 0x0C, 0x01},
  {0x00, 0x00, 0x00, 0x0C, 0x31},
  {0x00, 0x00, 0x00, 0x0C, 0x0B},
  {0x00, 0x00, 0x00, 0x0C, 0x03},
  {0x00, 0x00, 0x00, 0x0C, 0x00},
  {0x00, 0x00, 0x00, 0x0C, 0x08},
  {0x00, 0x00, 0x00, 0x0C, 0x04},
  {0x00, 0x00, 0x00, 0x0C, 0x0C},
  {0x00, 0x00, 0x00, 0x0C, 0x32},
  {0x00, 0x00, 0x00, 0x0C, 0x33}
};

uint32_t rgbcolor [] =
{
  0x000000FF,
  0x000000FF,
  0x008A2BE2,
  0x008A2BE2,
  0x00FF0000,
  0x00FF0000,
  0x00FF0000,
  0x00FF0000,
  0x00FF0000,
  0x00FFFF00,
  0x00FFFF00,
  0xFFFFFFFF,
  0xFFFFFFFF,
  0x0000FF00,
  0x0000FF00,
  0x0000FF00,
  0x00FF0000
};

unsigned long lastLEDUpdateMillis = 0;          // will store last time an RFID color was read

void  loop() {


  if(millis() - lastLEDUpdateMillis >= 2000)
  {
    Serial.println("No tag");
    strip.clear();
    strip.show();
  }

  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  if (scanForTag(tagData) == true)
  {
    // Found a tag, now check against known Kyber Crystal ID's for a match
    for (int i = 0; i < last; i++)
    {
      if (compareTagData(tagData, KyberCrystalUID[i]))
      {
        strip.fill(rgbcolor[i]);
        strip.show();
        Serial.println(colorNames[i]);

      }
    }

    lastLEDUpdateMillis = millis();        // will store last time LED was updated


  }


}
