// Include SPI used for SD card and display
#include <SPI.h>
// Include graphics and display library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
// include the SD library
#include <SD.h>

// Rotary Encoder
#define inputCLK        2     // terminal B
#define inputDT         3     // terminal A
#define inputSW         A0    // push button

// OLED display (SSD1351 4-wire using hardware SPI pins)
#define OLEDDC_PIN      4     // D/C
#define OLEDCS_PIN      5     // ~OLEDCS
#define OLEDRST_PIN     6     // ~RES

// SD utility library functions
#define SDCS_PIN        10     // ~SDCS

// WS2812 LEDs on two panels 64 LEDs each
#define PIXELS_PIN      8     // PORTB 0
#define NUM_PANELS      2     // Number of LED panels
#define NUM_RGB         64    // Number of WS281X per panel

// Screen dimensions
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   128

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

// Rotary encoder
int counter;
int8_t subCounter, counterChange;
int currentStateCLK, previousStateCLK; 
int buttonState, previousStateSW;
String encdir ="";

// OLED display
Adafruit_SSD1351 oled = Adafruit_SSD1351(
  SCREEN_WIDTH,SCREEN_HEIGHT, &SPI,
  OLEDCS_PIN, OLEDDC_PIN, OLEDRST_PIN);

// LEDs
// Number of LEDs (3 per each WS281X: red, green, blue)
uint8_t rgb_arr[NUM_PANELS*3] = {0};
volatile uint8_t *port;
uint8_t pinMask;
uint32_t t_f;
// LED brightness 0-255 (stored as +1)
uint8_t brightness;
int tmpInt;

uint8_t mode1, mode2;
uint32_t color1, color2;
uint32_t timeMillis, timeDelay;
char buff[32];
size_t buffLen;
// https://en.wikipedia.org/wiki/8.3_filename
// 8.3 filenames are limited to at most eight characters (after any directory specifier),
// followed optionally by a filename extension consisting of a period . and at most
// three further characters.
char filenames[5][13];

File root;
int nrFiles = 0;
File dataFile;

// FSM defines
enum {
  fsmIdle = 0,
  fsmFileSelect,
  fsmFileOpen,
  fsmHandleNameLine,
  fsmHandleDescriptionLine,
  fsmHandleDataLine,
  fsmSequenceRun,
  fsmSequenceStop,
};

uint8_t fsmState = fsmIdle;
uint8_t readNextLine = 0;

// the interrupt service routine affects this
volatile bool encoderRotated = false;

// interrupt service routine
void senseEncoderRotated1() {
  currentStateCLK = digitalRead(inputCLK);
   if (currentStateCLK != previousStateCLK) {
     subCounter++;
     if (digitalRead(inputDT) != currentStateCLK) {
       encdir ="CCW";
       counterChange = -1;
     } else {
       encdir ="CW";
       counterChange = 1;
     }
   }
  previousStateCLK = currentStateCLK;
  // signal the change when 4 pulses of encoder were detected
  // this filters undesired movement of the encoder when pressing the switch
  if ((subCounter % 4) == 0) {
    subCounter = 1;
    encoderRotated = true;
  }
}

void oledDrawText(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bgcolor = BLACK) {
  oled.setCursor(x, y);
  oled.setTextColor(color, bgcolor);
  oled.print(text);
}

void printDirectory(File dir) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      // return to the first file in the directory
      dir.rewindDirectory();
      break;
    }
//    Serial.print(entry.name());
    if (entry.isDirectory()) {
//      Serial.println("Skipping directory..");/
    } else {
      // files have sizes, directories do not
      Serial.print(entry.name());
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
      nrFiles++;
    }
    entry.close();
  }
}

// Read the root folder for files and copy count names to the filenames
// buffers. As long as start is small (< 10) the response is fine. With
// start at 30 it takes longer to traverse the folder and UI is jerky..
// XXX: Consider using a single file with known name listing all the
//      other filenames.
void getFileNames(File dir, uint16_t start, uint16_t count) {
  uint16_t index = 0;
  uint16_t pos = 0;

  if (start == 0) {
    pos++;
    pos++;
  }
  if (start == 1) {
    pos++;
    start--;
  }
  if ((start >= 2) && (start <= (nrFiles - 2))) {
    start--;
    start--;
  }

  if (start == (nrFiles - 1)) {
    start--;
    start--;
    count--;
    count--;
  }

  if (start == (nrFiles - 2)) {
    start--;
    count--;
  }

  for(uint16_t i = 0; i < 5; i++) {
    filenames[i][0] = 0;
  }

//  Serial.print(" >> index "); Serial.print(index); Serial.print(", pos "); Serial.println(pos);

  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    if (entry.isDirectory()) {
//      Serial.println("Skipping directory..");/
    } else {
      if ((index >= start) && (index < (start + count))) {
//        Serial.print(" ++ index "); Serial.print(index); Serial.print(", pos "); Serial.println(pos);
        strcpy(filenames[pos], entry.name());
        filenames[pos][12] = 0;
        pos++;
      } else {
//        Serial.print(" -- index "); Serial.print(index); Serial.print(", pos "); Serial.println(pos);
      }
      index++;
    }
    entry.close();
    if (pos == count) {
      break;
    }
  }

//  Serial.print(" << index "); Serial.print(index); Serial.print(", pos "); Serial.println(pos);

  // return to the first file in the directory
  dir.rewindDirectory();
}

long parseInt(const char *buffer, const size_t length) {
  long value = 0;
  char c;
//  Serial.println("Buffer decode:");

  for (size_t i = 0; i < length; i++) {
    if (buffer[i] >= '0' && buffer[i] <= '9')
      c = buffer[i] - '0';
    else {
//      Serial.print("ignoring ");/
//      Serial.println(buffer[i], HEX);/
//      contin/ue;
      break;
    }
//    Serial.println(tmp, HEX);
    value *= 10;
    value += c;
//    Serial.println(data, HEX);
  }

  return value;
}

long parseHex(const char *buffer, const size_t length) {
  long value = 0;
  char c;
//  Serial.println("Buffer decode:");

  for (size_t i = 0; i < length; i++) {
    if (buffer[i] >= '0' && buffer[i] <= '9')
      c = buffer[i] - '0';
    else if (buffer[i] >= 'A' && buffer[i] <= 'F')
      c = buffer[i] - 'A' + 10;
    else if (buffer[i] >= 'a' && buffer[i] <= 'f')
      c = buffer[i] - 'a' + 10;
    else {
//      Serial.print("ignoring ");/
//      Serial.println(buffer[i], HEX/);
//      continue;/
      break;
    }
//    Serial.println(tmp, HEX);
    value <<= 4;
    value |= c;
//    Serial.println(data, HEX);
  }

  return value;
}

void setColorRGB(uint16_t idx, uint8_t r, uint8_t g, uint8_t b) 
{
  if(idx < NUM_PANELS) 
  {
    uint8_t *p = &rgb_arr[idx*3];
    if (brightness) {
      *p++ = (g * brightness) >> 8;
      *p++ = (r * brightness) >> 8;
      *p   = (b * brightness) >> 8;
    } else {
      *p++ = g;  
      *p++ = r;
      *p   = b;
    }
  }
}

void render(void) 
{
  while((micros() - t_f) < 300L);  // wait for 300us (data latch)

  // The volatile attribute is used to tell the compiler not to optimize 
  // this section.  We want every instruction to be left as is.
  //
  // Generating an 800KHz signal (1.25us period) implies that we have
  // exactly 20 instructions clocked at 16MHz (0.0625us duration) to 
  // generate either a 1 or a 0---we need to do it within a single 
  // period. 
  // 
  // By choosing 1 clock cycle as our time unit we can keep track of 
  // the signal's phase (T) after each instruction is executed.
  //
  // To generate a value of 1, we need to hold the signal HIGH (maximum)
  // for 0.8us, and then LOW (minimum) for 0.45us.  Since our timing has a
  // resolution of 0.0625us we can only approximate these values. Luckily, 
  // the WS281X chips were designed to accept a +/- 300ns variance in the 
  // duration of the signal.  Thus, if we hold the signal HIGH for 13 
  // cycles (0.8125us), and LOW for 7 cycles (0.4375us), then the variance 
  // is well within the tolerated range.
  //
  // To generate a value of 0, we need to hold the signal HIGH (maximum)
  // for 0.4us, and then LOW (minimum) for 0.85us.  Thus, holding the
  // signal HIGH for 6 cycles (0.375us), and LOW for 14 cycles (0.875us)
  // will maintain the variance within the tolerated range.
  //
  // For a full description of each assembly instruction consult the AVR
  // manual here: http://www.atmel.com/images/doc0856.pdf

  volatile uint16_t
    i   = NUM_RGB;  // Loop counter
  volatile uint8_t
   *ptr = rgb_arr,  // Pointer to next byte
    b   = *ptr++,   // Current byte value
    hi,             // PORT w/output bit set high
    lo;             // PORT w/output bit set low

  volatile uint8_t next, bit;

  hi   = *port |  pinMask;
  lo   = *port & ~pinMask;
  next = lo;
  bit  = 8;

  cli(); // Disable interrupts so that timing is as precise as possible

  asm volatile(
   "head0:"                    "\n\t" // Clk  Pseudocode    (T =  0)
    "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
    "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
    "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
    "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
    "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
    "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
    "breq nextbyte0"           "\n\t" // 1-2  if(bit == 0) (from dec above)
    "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
    "nop"                      "\n\t" // 1    nop           (T = 13)
    "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp head0"               "\n\t" // 2    -> head20 (next bit out)
   "nextbyte0:"                "\n\t" //                    (T = 10)
    "ldi  %[bit]  , 8"         "\n\t" // 1    bit = 8       (T = 11)
    "ld   %[byte] , %a[ptr]+"  "\n\t" // 2    b = *ptr++    (T = 13)
    "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 20)

   "head1:"                    "\n\t" // Clk  Pseudocode    (T =  0)
    "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
    "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
    "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
    "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
    "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
    "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
    "breq nextbyte1"           "\n\t" // 1-2  if(bit == 0) (from dec above)
    "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
    "nop"                      "\n\t" // 1    nop           (T = 13)
    "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp head1"               "\n\t" // 2    -> head20 (next bit out)
   "nextbyte1:"                "\n\t" //                    (T = 10)
    "ldi  %[bit]  , 8"         "\n\t" // 1    bit = 8       (T = 11)
    "ld   %[byte] , %a[ptr]+"  "\n\t" // 2    b = *ptr++    (T = 13)
    "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
// need to reset the data pointer back to first color byte:
// *p  = rgb_arr
// val = *p++
    "sbiw %a[ptr], 3"          "\n\t" // 2    ptr = ptr - 3 (T = 18)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 20)

   "head2:"                    "\n\t" // Clk  Pseudocode    (T =  0)
    "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
    "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
    "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
    "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
    "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
    "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
    "breq nextbyte2"           "\n\t" // 1-2  if(bit == 0) (from dec above)
    "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
    "nop"                      "\n\t" // 1    nop           (T = 13)
    "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp head2"               "\n\t" // 2    -> head20 (next bit out)
   "nextbyte2:"                "\n\t" //                    (T = 10)
    "ldi  %[bit]  , 8"         "\n\t" // 1    bit = 8       (T = 11)
    "ld   %[byte] , %a[ptr]+"  "\n\t" // 2    b = *ptr++    (T = 13)
    "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
    "brne head0"               "\n"   // 2    if(i != 0) -> (next byte)

    : [port]  "+e" (port),
      [byte]  "+r" (b),
      [bit]   "+r" (bit),
      [next]  "+r" (next),
      [count] "+w" (i)
    : [ptr]    "e" (ptr),
      [hi]     "r" (hi),
      [lo]     "r" (lo));

    i   = NUM_RGB;      // Loop counter
   *ptr = rgb_arr[3];   // Pointer to next byte
    b   = *ptr++;       // Current byte value

  asm volatile(
   "head3:"                    "\n\t" // Clk  Pseudocode    (T =  0)
    "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
    "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
    "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
    "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
    "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
    "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
    "breq nextbyte3"           "\n\t" // 1-2  if(bit == 0) (from dec above)
    "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
    "nop"                      "\n\t" // 1    nop           (T = 13)
    "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp head3"               "\n\t" // 2    -> head20 (next bit out)
   "nextbyte3:"                "\n\t" //                    (T = 10)
    "ldi  %[bit]  , 8"         "\n\t" // 1    bit = 8       (T = 11)
    "ld   %[byte] , %a[ptr]+"  "\n\t" // 2    b = *ptr++    (T = 13)
    "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 20)

   "head4:"                    "\n\t" // Clk  Pseudocode    (T =  0)
    "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
    "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
    "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
    "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
    "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
    "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
    "breq nextbyte4"           "\n\t" // 1-2  if(bit == 0) (from dec above)
    "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
    "nop"                      "\n\t" // 1    nop           (T = 13)
    "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp head4"               "\n\t" // 2    -> head20 (next bit out)
   "nextbyte4:"                "\n\t" //                    (T = 10)
    "ldi  %[bit]  , 8"         "\n\t" // 1    bit = 8       (T = 11)
    "ld   %[byte] , %a[ptr]+"  "\n\t" // 2    b = *ptr++    (T = 13)
    "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
// need to reset the data pointer back to first color byte:
// *p  = rgb_arr
// val = *p++
    "sbiw %a[ptr], 3"          "\n\t" // 2    ptr = ptr - 3 (T = 18)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 20)

   "head5:"                    "\n\t" // Clk  Pseudocode    (T =  0)
    "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
    "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
    "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
    "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
    "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
    "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
    "breq nextbyte5"           "\n\t" // 1-2  if(bit == 0) (from dec above)
    "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
    "nop"                      "\n\t" // 1    nop           (T = 13)
    "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
    "rjmp head5"               "\n\t" // 2    -> head20 (next bit out)
   "nextbyte5:"                "\n\t" //                    (T = 10)
    "ldi  %[bit]  , 8"         "\n\t" // 1    bit = 8       (T = 11)
    "ld   %[byte] , %a[ptr]+"  "\n\t" // 2    b = *ptr++    (T = 13)
    "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
    "nop"                      "\n\t" // 1    nop           (T = 16)
    "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
    "brne head3"               "\n"   // 2    if(i != 0) -> (next byte)

    : [port]  "+e" (port),
      [byte]  "+r" (b),
      [bit]   "+r" (bit),
      [next]  "+r" (next),
      [count] "+w" (i)
    : [ptr]    "e" (ptr),
      [hi]     "r" (hi),
      [lo]     "r" (lo));

  sei();                          // Enable interrupts
  t_f = micros();                 // t_f will be used to measure the 300us 
                                  // latching period in the next call of the 
                                  // function.
}

void setBrightness(uint8_t b) {
  // Stored brightness value is different than what's passed.
  // This simplifies the actual scaling math later, allowing a fast
  // 8x8-bit multiply and taking the MSB. 'brightness' is a uint8_t,
  // adding 1 here may (intentionally) roll over...so 0 = max brightness
  // (color values are interpreted literally; no scaling), 1 = min
  // brightness (off), 255 = just below max brightness.
  uint8_t newBrightness = b + 1;
  if(newBrightness != brightness) { // Compare against prior value
    // Brightness has changed -- re-scale existing data in RAM,
    // This process is potentially "lossy," especially when increasing
    // brightness. The tight timing in the WS2811/WS2812 code means there
    // aren't enough free cycles to perform this scaling on the fly as data
    // is issued. So we make a pass through the existing color data in RAM
    // and scale it (subsequent graphics commands also work at this
    // brightness level). If there's a significant step up in brightness,
    // the limited number of steps (quantization) in the old data will be
    // quite visible in the re-scaled version. For a non-destructive
    // change, you'll need to re-render the full strip data. C'est la vie.
    uint8_t  c,
            *ptr           = rgb_arr,
             oldBrightness = brightness - 1; // De-wrap old brightness value
    uint16_t scale;
    if(oldBrightness == 0) scale = 0; // Avoid /0
    else if(b == 255) scale = 65535 / oldBrightness;
    else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
    for(uint16_t i=0; i<NUM_PANELS; i++) {
      c      = *ptr;
      *ptr++ = (c * scale) >> 8;
    }
    brightness = newBrightness;
  }
}

void setup() {
  // init the variables early
  memset(filenames, 0, sizeof(filenames));
  fsmState = fsmIdle;
  counter = 0;
  subCounter = 1;
  brightness = 0;
  
  // Set LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Set encoder and switch pins as inputs  
  pinMode(inputCLK, INPUT);
  pinMode(inputDT, INPUT);
  pinMode(inputSW, INPUT);

  pinMode(PIXELS_PIN, OUTPUT);
  digitalWrite(PIXELS_PIN, LOW);
  port = portOutputRegister(digitalPinToPort(PIXELS_PIN));
  pinMask = digitalPinToBitMask(PIXELS_PIN);

  // use an interrupt to sense when the button is pressed
  attachInterrupt(digitalPinToInterrupt(inputCLK), senseEncoderRotated1, CHANGE);

  // Setup OLED display
  oled.begin();
  
  // Setup Serial Monitor
  Serial.begin (115200);
  while (!Serial) {}
  Serial.println("panel lights v0.9");
  
  // clear the display
//  timeMillis= millis();/
  oled.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
//  timeMillis= millis() - timeMillis;/
//  Serial.print("OLED clear took ");/
  Serial.println(timeMillis, DEC);
  oledDrawText(0, 0, "panel lights v0.9", RED);

  if (! SD.begin(SDCS_PIN)) {
    Serial.println("SD init failed!");
    while (1);
  }

  // Read the initial state of inputCLK
  // Assign to previousStateCLK variable
  previousStateCLK = digitalRead(inputCLK);
  previousStateSW = digitalRead(inputSW);

  t_f = micros();

  // intial LED brightness ~10%
  setBrightness(25);

  root = SD.open("/");
  printDirectory(root);
  Serial.println("setup() done!");
  Serial.print("# files: ");
  Serial.println(nrFiles);
  sprintf(buff, "# files %d", nrFiles);
  oledDrawText(0, 10, buff, WHITE);

  sprintf(buff, "Counter %d", counter);
  oledDrawText(0, 20, buff, YELLOW);

  sprintf(buff, "Button ???");
  oledDrawText(0, 30, buff, YELLOW);

  sprintf(buff, "Opened <none>");
  oledDrawText(0, 40, buff, YELLOW);

  tmpInt = (int)brightness;
  tmpInt *= 100;
  sprintf(buff, "BRIGHT: %3d %%", tmpInt >> 8);
  oledDrawText(0, 50, buff, YELLOW);

  // blink LEDs
  setColorRGB(0, 0, 0, 255);
  setColorRGB(1, 0, 255, 255);
  render();
  delay(1000);

  setColorRGB(0, 0, 0, 0);
  setColorRGB(1, 0, 0, 0);
  render();
  delay(1000);

  delay(100);

  // force the SD display update for the first time
  encoderRotated = true;
  
} // setup()

void loop() {

  // If the previous and the current state of the inputSW
  // are different then a button press has occured
  buttonState = digitalRead(inputSW);
  if (buttonState != previousStateSW) {
    Serial.print("SW button: "); Serial.println(buttonState ? "UP" : "DOWN");
    sprintf(buff, "%s", buttonState ? "UP" : "DOWN");
    oled.fillRect(42, 30, 24, 10, BLACK);
    oledDrawText(42, 30, buff, YELLOW);

    if (buttonState) {
      // button was released
      digitalWrite(LED_BUILTIN, LOW);
      
      if (fsmState == fsmIdle) {
        // in the main menu, button pressed to select a file
        // move to next state
        fsmState = fsmFileSelect;
      } else if ((fsmState == fsmFileSelect) ||
                  (fsmState == fsmFileOpen) ||
                  (fsmState == fsmHandleNameLine) ||
                  (fsmState == fsmHandleDescriptionLine) ||
                  (fsmState == fsmHandleDataLine) ||
                  (fsmState == fsmSequenceRun)) {
        // in the main menu, button pressed to stop current run
        // move to next state
        fsmState = fsmSequenceStop;
      } else {
        // unknown fsmState
        // error: move to intial state
        fsmState = fsmIdle;
      }
      
    } else {
      // button was pressed
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
  // Update previousStateSW with the current state
  previousStateSW = buttonState;

  // if dataFile is opened and readNextLine is set
  // read a line from file into buffer
  if (dataFile && readNextLine) {
    if (dataFile.peek() == -1) {
      Serial.println("EOF.. rewind!");
      // go to start of the file
      dataFile.seek(0);
    }
    // get the line of text into the buffer
    buffLen = dataFile.readBytesUntil('\n', buff, 31);
    if (buffLen > 0) {
      // terminate the string
      buff[buffLen] = '\0';

      // check the first character of the buffer
      if (buff[0] == '#') {
        // this is a commented line
        if (fsmState == fsmFileOpen) {
          // first line of the file; short sequence name
          fsmState = fsmHandleNameLine;
        } else {
          // other lines of the file; sequence description
          fsmState = fsmHandleDescriptionLine;
        }
      } else {
        // data line (not a comment)
        fsmState = fsmHandleDataLine;
      }
    } else {
      // errors?
//      Serial.println("WTF?!!");
      fsmState = fsmSequenceStop;
    }
  }
  // make sure we do not try to read file when not desired!
  readNextLine = 0;

  // handle FSM states
  if (fsmState == fsmIdle) {

      if (encoderRotated) {
          // new counter value is either +1 or -1
          counter = counter + counterChange;
          // clip to number of files found on the SD card
          if (counter < 0) counter = 0;
          if (counter >= nrFiles) counter = nrFiles - 1;
          Serial.print("Direction: "); Serial.print(encdir); Serial.print(" -- Value: "); Serial.println(counter);

          oled.fillRect(48, 20, 12, 10, BLACK);
          sprintf(buff, "%d", counter);
          oledDrawText(48, 20, buff, YELLOW);

          getFileNames(root, counter, 5);

          uint16_t fgcolor = GREEN;
          uint16_t bgcolor = BLACK;
          // show 5 files max
          for(uint16_t i = 0; i < 5; i++) {
            if (i == 2) {
              bgcolor = WHITE;
              fgcolor = RED;
            } else {
              bgcolor = BLACK;
              fgcolor = GREEN;
            }
            oled.fillRect(0, i*10+60, 100, 10, BLACK);
            if (filenames[i][0] != 0) {
              oledDrawText(0, i*10+60, filenames[i], fgcolor, bgcolor);
            }
          }
      }
      encoderRotated = false;

  } else if (fsmState == fsmFileSelect) {
    if ((filenames[2][0] != 0) && (! dataFile)) {
      // data file not yet opened.. open it!
      dataFile = SD.open(filenames[2]);
      Serial.print("opened "); Serial.println(dataFile.name());
      oled.fillRect(42, 40, 78, 10, BLACK);
      oledDrawText(42, 40, dataFile.name(), RED);
      // move to next state
      fsmState = fsmFileOpen;
      readNextLine = 1;
    } else {
      // error: move to intial state
      fsmState = fsmIdle;
    }

  } else if (fsmState == fsmFileOpen) {
    // nothing to do here; see buffer handling above
    readNextLine = 1;

  } else if (fsmState == fsmHandleNameLine) {
    // buffer holds short sequence name
    // TODO: show it on display
    readNextLine = 1;

  } else if (fsmState == fsmHandleDescriptionLine) {
    // buffer holds (part of) sequence description
    // TODO: show it on display
    readNextLine = 1;
    
  } else if (fsmState == fsmHandleDataLine) {
    // Serial.print("File: "); Serial.print(dataFile.name()); Serial.print(" size "); Serial.println(dataFile.size());

    // file data line : XX AAAAAA YY BBBBBB CC
    //  XX       - panel 1 control byte  (00 by default)
    //  AAAAAA   - panel 1 RGB color
    //  YY       - panel 2 control byte  (00 by default)
    //  BBBBBB   - panel 2 RGB color
    //  CC       - time (in 100 ms)
    // Example:
    // # this is a name (max 31 chars)
    // # this is description (max 255 chars)
    // # description can span several lines..
    // # first line without leading # is data line
    // 00 70E79f 00 E6d3e7 36
    // 00 000000 00 000000 18
    // 00 cFdbCe 00 FcC2a5 18
    // 00 000000 00 000000 13
    // 00 0245B5 00 BdcB5e 23
    // Notes:
    //   - color 000000 means LED off

    Serial.print(">line "); Serial.print(buffLen); Serial.print(" : "); Serial.print(buff); Serial.println();
    // parse both panel colors and time to delay
    mode1 = parseHex(buff, 2);
    color1 = parseHex(buff+3, 8);
    mode2 = parseHex(buff+10, 2);
    color2 = parseHex(buff+13, 8);
    timeDelay = parseInt(buff+20, 2);
    Serial.print("mode1: "); Serial.print(mode1, HEX);
    Serial.print(" col1: "); Serial.print(color1, HEX);
    Serial.print(" mode2: "); Serial.print(mode2, HEX);
    Serial.print(" col2: "); Serial.print(color2, HEX);
    Serial.println();
    // time in ms to wait in next FSM state
    timeDelay *= 100;
    Serial.print("delay: "); Serial.print(timeDelay); Serial.println();

    // set lights on both panels
    setColorRGB(0, (uint8_t)(color1 >> 16), (uint8_t)(color1 >> 8), (uint8_t)(color1));
    setColorRGB(1, (uint8_t)(color2 >> 16), (uint8_t)(color2 >> 8), (uint8_t)(color2));
    render();
    
    // record current time in ms, used in next state
    timeMillis = millis();

    // move to next state
    fsmState = fsmSequenceRun;

  } else if (fsmState == fsmSequenceRun) {
    // wait until delay elapsed
    if ((millis() - timeMillis) < timeDelay) {
      // wait a little bit..
      delay(10);

      // if encoder value changed adjust brightness accordingly
      if (encoderRotated) {
        tmpInt = (int)brightness + (counterChange * 10);
        if (tmpInt > 255) tmpInt = 255;
        if (tmpInt < 25) tmpInt = 25;
        brightness = tmpInt;
        tmpInt *= 100;
        sprintf(buff, "BRIGHT: %3d %%", tmpInt >> 8);
        oledDrawText(0, 50, buff, YELLOW);
      }
      encoderRotated = false;

      // remain is this state, do not read next line!

    } else {
      // go to previous state, read next line from file!
      fsmState = fsmHandleDataLine;
      readNextLine = 1;
    }
    
  } else if (fsmState == fsmSequenceStop) {
    // close the file if opened
    if (dataFile) {
      // close the file
      Serial.print("closing file "); Serial.println(dataFile.name());
      dataFile.close();
      oled.fillRect(42, 40, 78, 10, BLACK);
      oledDrawText(42, 40, "<none>", RED);
    }

    // turn off the lights
    setColorRGB(0, 0, 0, 0);
    setColorRGB(1, 0, 0, 0);
    render();

    // move to intial state
    fsmState = fsmIdle;

  } else {
    // error: move to intial state
    fsmState = fsmIdle;
  }

} // loop()

