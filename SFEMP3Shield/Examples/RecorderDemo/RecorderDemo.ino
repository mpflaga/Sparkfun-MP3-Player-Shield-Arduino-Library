/**
 * \file RecorderDemo.ino
 *
 * \brief Example sketch of using the MP3Shield Arduino driver to record OGG.
 * \remarks comments are implemented with Doxygen Markdown format
 *
 * \author Miguel Moreto
 *
 * This sketch listens for commands from 3 pushbuttons that start recoring
 * proccess, stop recording and plays the recorded track.
 *
 * \note This example sketch was tested with a Teensy2.0++ board and a
 * generic MP3 module with VS1053b chipset. You can configure Teensyduino
 * as a mass storage device, so your SD card can be accessed in windows explorer.
 * It is very slow and cannot be safely removed but it is cool!
 */
#include <SPI.h>

//Add the SdFat Libraries
#include <SdFat.h>
#include <SdFatUtil.h>

// Add Bounce lib to eliminate bounce in pushbutton presses.
#include <Bounce.h>

#include <SFEMP3Shield.h>

#define SERIAL_DEBUG // Comment this if you don't want serial debug messages.

// Pin definition:
#define PIN_PUSH_0 5 // Pushbutton Play pin
#define PIN_PUSH_1 7 // Pushbutton Stop pin
#define PIN_PUSH_2 8 // Pushbutton Rec pin
#define PIN_LED_REC 17 // Led to indicate recording
#define PIN_LED_OK 16  // Led to indicate ok status
#define PIN_LED_ERROR 15 // Led error.
#define PIN_AMP_SHUTDOWN 13 // Output pin to turn amplifier module on or off.


/**
 * \brief Object instancing the SdFat library.
 *
 * principal object for handling all SdCard functions.
 */
SdFat sd;

// Objects:
SFEMP3Shield MP3player;

Bounce PushButtonStop = Bounce(PIN_PUSH_1,10);
Bounce PushButtonRec = Bounce(PIN_PUSH_2,10);
Bounce PushButtonPlay = Bounce(PIN_PUSH_0,10);

char trackname[] = "rec001.ogg";

uint8_t ledstate = 0;

void setup() {

	// Input pins:
	pinMode(PIN_PUSH_0, INPUT_PULLUP); // Play button
	pinMode(PIN_PUSH_1, INPUT_PULLUP);
	pinMode(PIN_PUSH_2, INPUT_PULLUP);
  
	// Output pins:
	pinMode(PIN_LED_OK,OUTPUT);
	pinMode(PIN_LED_REC,OUTPUT);
	pinMode(PIN_LED_ERROR,OUTPUT);
	pinMode(PIN_AMP_SHUTDOWN,OUTPUT);

	// Turn off the LEDs
	digitalWrite(PIN_LED_OK, LOW);
	digitalWrite(PIN_LED_REC, LOW);
	digitalWrite(PIN_LED_ERROR, LOW);
	digitalWrite(PIN_AMP_SHUTDOWN, LOW); // Turn off amplifier

	//Initialize the SdCard.
	if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
	if(!sd.chdir("/")) sd.errorHalt("sd.chdir");

	StartMP3();

#if defined(SERIAL_DEBUG)
	Serial.println("Started...");
#endif

}

void loop()
{
	uint16_t result = 0;

	// If rec button is pressed, start recording.
	if (PushButtonRec.update()){
		if (PushButtonRec.fallingEdge()){
#if defined(SERIAL_DEBUG)
			Serial.println("Start recording");
#endif
			MP3player.stopTrack(); // Stop playing if applicable.
			digitalWrite(PIN_LED_OK, LOW); // Turn off "ready to play" Led.
			result = MP3player.startRecordOgg(trackname); // Start recording.
			Serial.println(result);		
		}
	}

	// If play button is pressed, play recorded track.
	if (PushButtonPlay.update()){
		if (PushButtonPlay.fallingEdge()){
#if defined(SERIAL_DEBUG)
			Serial.println("Playing...");
#endif			
			digitalWrite(PIN_AMP_SHUTDOWN, HIGH); // Turn on amplifier
			StartMP3(); // Restart MP3 in playing mode with latest patch.
			result = MP3player.playMP3(trackname);
			Serial.println(result);
		}
	}

	// If stop button is pressed, stop playing track or recording.
	if (PushButtonStop.update()){
		if (PushButtonStop.fallingEdge()){
			if(MP3player.isPlaying()){
#if defined(SERIAL_DEBUG)
				Serial.println("Stop playing");
#endif
				MP3player.stopTrack();
			}
			if (MP3player.isRecording()){
#if defined(SERIAL_DEBUG)
				Serial.println("Stop rec");
#endif
				MP3player.stopRecording(); // Command to finish recording.
				digitalWrite(PIN_LED_REC,LOW); // Turn off red led.
			}
			digitalWrite(PIN_AMP_SHUTDOWN, LOW); // Turn off amplifier
		}
	}


	if (MP3player.isRecording()){
		result = MP3player.doRecordOgg();
		if (result > 0){ // Toogle rec led when one or more datablock (512B) are written.
			ledstate = ~ledstate;
			digitalWrite(PIN_LED_REC,ledstate); // Toogle rec led.
		}
		//Serial.println(result); // Print the number of databloks written.
	}else{
		digitalWrite(PIN_LED_REC, LOW); // Turn on ok led
		digitalWrite(PIN_LED_OK, HIGH); // Turn on ok led
	}

	//delay(200);

  /* add main program code here */

}


/*
 * MP3 Player begin function. Restart VS1053 and load the firmware update.
 */
void StartMP3(){  
	uint8_t result; //result code,
	result = MP3player.begin();
	//check result, see readme for error codes.
	if(result != 0) {
		digitalWrite(PIN_LED_ERROR, HIGH); // Turn on error led
#if defined(SERIAL_DEBUG)
		Serial.print(F("Error code: "));
		Serial.print(result);
		Serial.println(F(" when trying to start MP3 player"));
#endif
		if ( result == 6 ) {
#if defined(SERIAL_DEBUG)
			Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
			Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.
#endif
		}
	}else{
		digitalWrite(PIN_LED_OK, HIGH); // Turn on ok led
	}
}