#include <Adafruit_VC0706.h>
#include <SD.h>

// comment out this line if using Arduino V23 or earlier
#include <SoftwareSerial.h>         
#include <RTClib.h>          // Realtime clock library
#include <Wire.h>            // Also needed for RTC
#include "LiquidCrystal.h" // LCD Library

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal lcd(0);

#define chipSelect 10
RTC_DS1307      clock;
// Time display variables
int second;
int hour;
#define MAX_OUT_CHARS 16  //max nbr of characters to be sent on any one serial command
char   buffer1[MAX_OUT_CHARS + 1];  //buffer used to format a line (+1 is for trailing 0)
char   buffer2[MAX_OUT_CHARS + 1];  //buffer used to format a line (+1 is for trailing 0)
char   amOrPm[1];  //buffer used to format a line (+1 is for trailing 0)

char            directory[] = "DCIM/CANON999", // Emulate Canon folder layout
                filename[]  = "DCIM/CANON999/IMG_0000.JPG";
int             imgNum      = 0;
const int       minFileSize = 20 * 1024; // Eye-Fi requires minimum file size


// Using SoftwareSerial (Arduino 1.0+) or NewSoftSerial (Arduino 0023 & prior):
#if ARDUINO >= 100
SoftwareSerial cameraconnection = SoftwareSerial(2, 3);
#else
NewSoftSerial cameraconnection = NewSoftSerial(2, 3);
#endif

Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);

void setup() {
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  Wire.begin();  // IMPORTANT: the clock should have previously been set
  clock.begin(); // using the 'ds1307' example sketch included with RTClib.
  SdFile::dateTimeCallback(dateTime); // Register timestamp callback

  // When using hardware SPI, the SS pin MUST be set to an
  // output (even if not connected or used).  If left as a
  // floating input w/SPI on, this can cause lockuppage.
#if !defined(SOFTWARE_SPI)
  if(chipSelect != 10) pinMode(10, OUTPUT); // SS on Uno, etc.
#endif

  Serial.begin(9600);
  Serial.println("VC0706 Camera snapshot test");
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }  
  
  // Try to locate the camera
  if (cam.begin()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }

  if(!SD.exists(directory) && !SD.mkdir(directory)) {
    Serial.println("directory write failed");
  }
  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }

  // Set the picture size - you can choose one of 640x480, 320x240 or 160x120 
  // Remember that bigger pictures take longer to transmit!
  
  cam.setImageSize(VC0706_640x480);        // biggest
  //cam.setImageSize(VC0706_320x240);        // medium
  //cam.setImageSize(VC0706_160x120);          // small

  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_640x480) Serial.println("640x480");
  if (imgsize == VC0706_320x240) Serial.println("320x240");
  if (imgsize == VC0706_160x120) Serial.println("160x120");
  nextFilename();                // Scan directory for next name
  Serial.println("Snap in 3 secs...");

  delay(3000);

  if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");
  

  File imgFile = SD.open(filename, FILE_WRITE);
  if(imgFile == NULL) {
    // Couldn't open file.  Show RED (no GREEN) for 5 sec:
    return; // Resume motion detection
  }

  uint16_t jpegLen = cam.frameLength();
  uint16_t bytesRemaining;
  uint8_t  b, *ptr;

  // Transfer data from camera to SD file:
  for(bytesRemaining = jpegLen; bytesRemaining ; bytesRemaining -= b) {
    b   = min(32, bytesRemaining); // Max of 32 bytes at a time
    ptr = cam.readPicture(b);      // From camera
    imgFile.write(ptr, b);         // To SD card
  }

  // Pad file to minimum Eye-Fi file size if required:
  if(jpegLen < minFileSize) {
    for(bytesRemaining  = minFileSize - jpegLen; bytesRemaining ;
        bytesRemaining -= b) {
      b = min(32, bytesRemaining); // Max of 32 bytes at a time
      imgFile.write(ptr, b);       // Just repeat last data, it's ignored
    }
  }

  imgFile.close();  

  Serial.println("done!");

}

void loop() {
  displayTimeAndDate();
}

void nextFilename(void) {
  // filename format is "DCIM/CANON999/IMG_nnnn.JPG";
  // Start of image # is at pos            ^ 18
  // If you decide to change the path or name, the index into
  // filename[] will need to be changed to suit.
  for(;;) {
    // Screwy things will happen if over 10,000 images in folder.
    // As explained above, this is not industrial-grade code.  It's
    // expecting other limits (e.g. FAT16 stuff) will be hit first.

    // sprintf() is a costly function to invoke (about 2K of code),
    // so this instead assembles the filename manually:
    filename[18] = '0' +  imgNum / 1000;
    filename[19] = '0' + (imgNum /  100) % 10;
    filename[20] = '0' + (imgNum /   10) % 10;
    filename[21] = '0' +  imgNum         % 10;
    if(!SD.exists(filename)) return; // Name available!
    imgNum++;                        // Keep looking
  }
}

void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = clock.now();
  *date = FAT_DATE(now.year(), now.month(), now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

