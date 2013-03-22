/**
 * \file RecorderDemo.ino
 *
 * \brief Example sketch of using the MP3Shield Arduino driver to record OGG.
 * \remarks comments are implemented with Doxygen Markdown format
 *
 * \author Miguel Moreto
 *
 * This sketch listens for commands from serial. The user can start recoring,
 * stop recording and play the recorded track.
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


#include <SFEMP3Shield.h>


#include <Bounce.h>

// Pin definition:
#define PIN_PUSH_0 5 // Pushbutton Play pin
#define PIN_PUSH_1 7 // Pushbutton Stop pin
#define PIN_PUSH_2 8 // Pushbutton Rec pin
#define PIN_LED_REC 17 // Led to indicate recording
#define PIN_LED_OK 16  // Led to indicate ok status
#define PIN_LED_ERROR 15 // Led error.
#define PIN_AMP_SHUTDOWN 13 // Output pin to turn amplifier module on or off.


// bounce objects to debounce buttons.
Bounce PushButtonRotary = Bounce(PIN_PUSH_0, 10);  // 10 ms debounce
Bounce PushButtonStop = Bounce(PIN_PUSH_1,10);
Bounce PushButtonRec = Bounce(PIN_PUSH_2,10);

/**
 * \brief Object instancing the SdFat library.
 *
 * principal object for handling all SdCard functions.
 */
SdFat sd;

// Objects:
SFEMP3Shield MP3player;

// Using SDFatLib, limit the filenames to 8.3 format.
char trackname[] = "rec001.ogg";

/* 
 * Recorder plugin filename. This one is include in this demo folder.
 * If you whant to use other profile, consult Ogg Vorbis Encoder
 * application manual from VLSI:
 * http://www.vlsi.fi/fileadmin/software/VS10XX/VorbisEncoder170c.pdf
 * All the profile plugin files are stored in recorder_plugins.zip
 * included in SFEMP3Shield library plugins folder.
 */
char pluginfilename[] = "e44k1q05.vs"; // HiFi Voice Profile number 5: typical 87kbps

uint8_t ledstate = 0;

void setup() {

	Serial.begin(115200);

	// Play, stop and rec buttons.
	pinMode(PIN_PUSH_0, INPUT_PULLUP);
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

	// Binds a user defined function as a handler that is
	// executed at the end of recording.
	MP3player.bindEndRecordHandler(endRecordingHandler);

	help();
}

void loop()
{
	uint16_t result = 0;

	if (PushButtonRotary.update()) {
		if (PushButtonRotary.fallingEdge()){
			parse_menu('p');
		}
	}

	if (PushButtonStop.update()) {
		if (PushButtonStop.fallingEdge()){
			parse_menu('s');
		}
	}

	if (PushButtonRec.update()) {
		if (PushButtonRec.fallingEdge()){
			parse_menu('r');
		}
	}

	// If recorder is active, read data blocks from VS1053b chipset and
	// write them to track file.
	if (MP3player.isRecording()){
		result = MP3player.doRecordOgg();
		if (result > 0){ // Toogle rec led when one or more datablock (512B) are written.
			ledstate = ~ledstate;
			digitalWrite(PIN_LED_REC,ledstate); // Toogle rec led.
		}
		//Serial.println(result); // Print the number of databloks written.
	}

	// You can do something else here.


	if(Serial.available()) {
		parse_menu(Serial.read()); // get command from serial input
	}

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
		Serial.print(F("Error code: "));
		Serial.print(result);
		Serial.println(F(" when trying to start MP3 player"));
		if ( result == 6 ) {
			Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
			Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.
		}
	}else{
		digitalWrite(PIN_LED_OK, HIGH); // Turn on ok led
	}
}


//------------------------------------------------------------------------------
/**
 * \brief Decode the Menu.
 *
 * Parses through the characters of the users input, executing corresponding
 * MP3player library functions and features then displaying a brief menu and
 * prompting for next input command.
 */
void parse_menu(byte key_command) {

  static uint8_t gain = 0;

  uint8_t result; // result code from some function as to be tested at later time.

  Serial.print(F("Received command: "));
  Serial.write(key_command);
  Serial.println(F(" "));

  //if s, stop the playing or recording the current track
  if(key_command == 's') {
    if(MP3player.isPlaying()){
	  MP3player.stopTrack();
	  Serial.println("Player stoped.");
	}
	if (MP3player.isRecording()){
	  Serial.println("Stoping recording...");
	  MP3player.stopRecording(); // Command to finish recording.
	  digitalWrite(PIN_LED_REC,LOW); // Turn off red led.
	}
	digitalWrite(PIN_AMP_SHUTDOWN, LOW); // Turn off amplifier
  } // 's' command
  else if(key_command == 'r'){ // If 'r', start recording.
    Serial.println("Start recording");
    MP3player.stopTrack(); // Stop playing if applicable.
	digitalWrite(PIN_LED_OK, LOW); // Turn off "ready to play" Led.
	result = MP3player.startRecordOgg(trackname, pluginfilename); // Start recording.
	if (result != 0){
		Serial.print("Error starting recorder: ");
		Serial.println(result);	
	}
  } // 'r' command
  else if (key_command == 'p'){ // If 'p', play recorded track.
    Serial.println("Restarting VS10xx...");	
	StartMP3(); // Restart MP3 in playing mode with latest patch.
	result = MP3player.playMP3(trackname);
	if (result != 0){
		Serial.print("Error playing track: ");
		Serial.println(result);
	}else{
		digitalWrite(PIN_AMP_SHUTDOWN, HIGH); // Turn on amplifier
		Serial.println("Playing...");	
	}
  } // 'p' command
  else if(key_command == 'h') { // Help
	  help();
  }
  else if(key_command == 'G') { // Help
	  if (gain < 63){
		  if(!MP3player.setRecGain(gain++)){
			  gain--;
			  Serial.println("Not recording.");
		  }else{
			  Serial.print("Gain setted to: ");
			  Serial.println(gain);
		  }
	  }
  }

  else if(key_command == 'g') { // Help
	  if (gain > 0){
		  if(!MP3player.setRecGain(gain--)){
			  gain++;
			  Serial.println("Not recording.");
		  }else{
			  Serial.print("Gain setted to: ");
			  Serial.println(gain);
		  }
	  }
  }
}

//------------------------------------------------------------------------------
/**
 * \brief Print Help Menu.
 *
 * Prints a full menu of the commands available along with descriptions.
 */
void help() {
  Serial.println("Arduino SFEMP3Shield Library OGG recorder Example:");
  Serial.println("SFEMP3Shield by Bill Porter & Michael P. Flaga");
  Serial.println("Recording functions by Miguel Moreto based on VLSI docs.");
  Serial.println("COMMANDS:");
  Serial.println(" [s] to stop recording or playing");
  Serial.println(" [r] start recording ogg file");
  Serial.println(" [p] play recorded track");
  Serial.println(" [G] increase gain");
  Serial.println(" [g] decrease gain");
}

/**
 * \brief Function executed once the recording ends.
 * 
 * A pointer to this function is passed to MP3player class by using 
 * the method bindEndRecordHandler(function);
 */

void endRecordingHandler(void) 
{
	Serial.println("Record finished");
	digitalWrite(PIN_LED_REC, LOW); // Turn off rec led
	digitalWrite(PIN_LED_OK, HIGH); // Turn on ok led
}
