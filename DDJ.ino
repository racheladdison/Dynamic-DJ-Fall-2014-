#include <SPI.h>

#include <SdFat.h>
#include <SdFatUtil.h>

#include <SFEMP3Shield.h>

#include <Wire.h> 
#include <SFE_MMA8452Q.h>  

//declaring accelerometers
MMA8452Q Aleft; 
MMA8452Q Aright=MMA8452Q(0x1C);

#if defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_Timer1
  #include <TimerOne.h>
#elif defined(USE_MP3_REFILL_MEANS) && USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer
  #include <SimpleTimer.h>
#endif

SdFat sd;

SFEMP3Shield MP3player;
int16_t last_ms_char; // milliseconds of last recieved character from Serial port.
int8_t buffer_pos; // next position to recieve character from Serial port.

char buffer[6]; // 0-35K+null

char stateLeft;
char stateRight;

boolean flatVol=false;
boolean flatTempo=false;
boolean choosepps=true;
boolean choseDecade=true;

void setup() {

  uint8_t result; //result code from some function as to be tested at later time.

  Serial.begin(115200);
  
  Serial.println("In setup");
  Aleft.init();
  Serial.println(" past left");
  Aright.init();
  
  Serial.println("leaving Flor's setup");

  Serial.print(F("F_CPU = "));
  Serial.println(F_CPU);
  Serial.print(F("Free RAM = ")); // available in Version 1.0 F() bases the string to into Flash, to use less SRAM.
  Serial.print(FreeRam(), DEC);  // FreeRam() is provided by SdFatUtil.h
  Serial.println(F(" Should be a base line of 1017, on ATmega328 when using INTx"));


  //Initialize the SdCard.
  if(!sd.begin(SD_SEL, SPI_FULL_SPEED)) sd.initErrorHalt();
  // depending upon your SdCard environment, SPI_HAVE_SPEED may work better.
  if(!sd.chdir("/")) sd.errorHalt("sd.chdir");

  //Initialize the MP3 Player Shield
  result = MP3player.begin();
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print(F("Error code: "));
    Serial.print(result);
    Serial.println(F(" when trying to start MP3 player"));
    if( result == 6 ) {
      Serial.println(F("Warning: patch file not found, skipping.")); // can be removed for space, if needed.
      Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.
    }
  }

#if (0)
  // Typically not used by most shields, hence commented out.
  Serial.println(F("Applying ADMixer patch."));
  if(MP3player.ADMixerLoad("admxster.053") == 0) {
    Serial.println(F("Setting ADMixer Volume."));
    MP3player.ADMixerVol(-3);
  }
#endif

}

//------------------------------------------------------------------------------
/**
 * \brief Main Loop the Arduino Chip
 *
 * This is called at the end of Arduino kernel's main loop before recycling.
 * And is where the user's serial input of bytes are read and analyzed by
 * parsed_menu.
 *
 * Additionally, if the means of refilling is not interrupt based then the
 * MP3player object is serviced with the availaible function.
 *
 * \note Actual examples of the libraries public functions are implemented in
 * the parse_menu() function.
 */
void loop() {

// Below is only needed if not interrupt driven. Safe to remove if not using.
#if defined(USE_MP3_REFILL_MEANS) \
    && ( (USE_MP3_REFILL_MEANS == USE_MP3_SimpleTimer) \
    ||   (USE_MP3_REFILL_MEANS == USE_MP3_Polled)      )

  MP3player.available();
#endif
    char stateLeft;
    char stateRight;
    
    boolean flatVol=false;
    boolean flatTempo=false;
    boolean choosepps=true;
    boolean choseDecade=true;

  
  if( Aleft.available()&& Aright.available()) {  
    //Serial.println("reading accel"); 
    Aleft.read();
    Aright.read();
    stateLeft=OrientationLEFT();
    stateRight=OrientationRIGHT();
    //Serial.println(stateLEFT);
    //Serial.print(stateRIGHT);
    parse_menu(stateLeft, stateRight);
  }
  delay(100);
}

uint32_t  millis_prv;

//------------------------------------------------------------------------------
/**
 * \brief Decode the Menu.
 *
 * Parses through the characters of the users input, executing corresponding
 * MP3player library functions and features then displaying a brief menu and
 * prompting for next input command.
 */
void parse_menu(char stateLeft, char stateRight) {
  int i=3;
  uint8_t result; // result code from some function as to be tested at later time.

  // Note these buffer may be desired to exist globably.
  // but do take much space if only needed temporarily, hence they are here.
  char title[30]; // buffer to contain the extract the Title from the current filehandles
  char artist[30]; // buffer to contain the extract the artist name from the current filehandles
  char album[30]; // buffer to contain the extract the album name from the current filehandles
  char year[4];

  /*  VOLUME CONTROL */
  
  if(stateLeft == 'D') {
    union twobyte mp3_vol; // create key_command existing variable that can be both word and double byte of left and right.
    mp3_vol.word = MP3player.getVolume(); // returns a double uint8_t of Left and Right packed into int16_t

    if (stateRight == 'F') {
      flatVol=true;
    }
    if(stateRight == 'D' && (flatVol)) { // note dB is negative
      // assume equal balance and use byte[1] for math
      if(mp3_vol.byte[1] >= 250) { // range check
        mp3_vol.byte[1] = 250;
      } else {
        mp3_vol.byte[1] += 6; // keep it simpler with whole dB's
      }
      // push byte[1] into both left and right assuming equal balance.
      MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]); // commit new volume
      Serial.print(F("Volume changed to -"));
      Serial.print(mp3_vol.byte[1]>>1, 1);
      Serial.println(F("[dB]"));
      flatVol=false;
    } 
    if(stateRight == 'U' && (flatVol)) {
      if(mp3_vol.byte[1] <= 6) { // range check
        mp3_vol.byte[1] = 6;
      } else {
        mp3_vol.byte[1] -= 6;
      }
      // push byte[1] into both left and right assuming equal balance.
      MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]); // commit new volume
      Serial.print(F("Volume changed to -"));
      Serial.print(mp3_vol.byte[1]>>1, 1);
      Serial.println(F("[dB]"));
      flatVol=false;
    }
    if (stateRight == 'R' && (flatVol)) {
      byte key_command;
      
        key_command =( i+'0') - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      
      flatVol=false;
    }
    if (stateRight == 'L' && (flatVol)) {
      if( MP3player.getState() == playback) {
          MP3player.pauseMusic();
          Serial.println(F("Pausing"));
        } else if( MP3player.getState() == paused_playback) {
          MP3player.resumeMusic();
          Serial.println(F("Resuming"));
        } else {
          Serial.println(F("Not Playing!"));
        }
        flatVol=false;
    }
      
/* Genre/Stop */

  } else if(stateLeft == 'U') {
      if(stateRight == 'F') {
        choosepps=true;
      }
      if(stateRight == 'U' && choosepps) {
        byte key_command;
      
        key_command =('6' - 48);

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
        choosepps=false;
      }
      if(stateRight == 'R' && choosepps) {
        byte key_command;
      
        key_command = '1' - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      
        choosepps=false;
      }
      if(stateRight == 'L' && choosepps) {

         MP3player.stopTrack();

        choosepps=false;
      }
      if (stateRight == 'D' && (choosepps)) {
        byte key_command;
      
        key_command ='4' - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      choosepps=false;
      }
   
  } else if(stateLeft == 'R') {
      if(stateRight == 'F') {
        flatTempo=true;
      }
      if (stateRight == 'R' && (flatTempo)) {
        int8_t TrebleAmplitude = MP3player.getTrebleAmplitude();
        Serial.print(F("Former TrebleAmplitude = "));
        Serial.println(TrebleAmplitude, DEC);
        if (TrebleAmplitude >= 7) { // Range is from -8 - 7dB
          TrebleAmplitude = -8;
        } else {
          TrebleAmplitude+=2;
        }
        MP3player.setTrebleAmplitude(TrebleAmplitude);
        Serial.print(F("New TrebleAmplitude = "));
        Serial.println(MP3player.getTrebleAmplitude(), DEC);
        flatTempo=false;
      }
      if (stateRight == 'L' && (flatTempo)) {
        int8_t TrebleAmplitude = MP3player.getTrebleAmplitude();
        Serial.print(F("Former TrebleAmplitude = "));
        Serial.println(TrebleAmplitude, DEC);
        if (TrebleAmplitude <= -8) { // Range is from -8 - 7dB
          TrebleAmplitude = 7;
        } else {
          TrebleAmplitude-=2;
        }
        MP3player.setTrebleAmplitude(TrebleAmplitude);
        Serial.print(F("New TrebleAmplitude = "));
        Serial.println(MP3player.getTrebleAmplitude(), DEC);
        flatTempo=false;
      }
      if (stateRight == 'U' && (flatTempo)) {
        uint16_t BassAmplitude = MP3player.getBassAmplitude();
        Serial.print(F("Former BassAmplitude = "));
        Serial.println(BassAmplitude, DEC);
        if (BassAmplitude >= 15) { // Range is from 0 - 15dB
          BassAmplitude = 0;
        } else {
          BassAmplitude+=2;
        }
        MP3player.setBassAmplitude(BassAmplitude);
        Serial.print(F("New BassAmplitude = "));
        Serial.println(MP3player.getBassAmplitude(), DEC);
        flatTempo=false;
      }
      if (stateRight == 'D' && (flatTempo)) {
        uint16_t BassAmplitude = MP3player.getBassAmplitude();
        Serial.print(F("Former BassAmplitude = "));
        Serial.println(BassAmplitude, DEC);
        if (BassAmplitude <= 0) { // Range is from 0 - 15dB
          BassAmplitude = 15;
        } else {
          BassAmplitude-=2;
        }
        MP3player.setBassAmplitude(BassAmplitude);
        Serial.print(F("New BassAmplitude = "));
        Serial.println(MP3player.getBassAmplitude(), DEC);
        flatTempo=false;
      }

  } else if(stateLeft == 'L') {
      if(stateRight == 'F') {
        choseDecade=true;
      }
      if(stateRight == 'U' && (choseDecade)) {
        //2010+
        char ans;
        find2010((char*) &ans);
        if(ans!='n') {
      
          byte key_command =ans - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      
        choseDecade=false;
      }
      }
      if(stateRight == 'R' && (choseDecade)) {
        //2000-2010
        char ans;
        find2000((char*) &ans);
        if(ans!='n') {
          byte key_command =ans - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      
        choseDecade=false;
      }
      }
      if(stateRight == 'D' && (choseDecade)) {
        //1990-2000
        
       char ans;
        find1990((char*) &ans);
        if(ans!='n') {
      
          byte key_command =ans - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      
        choseDecade=false;
      }
      }
      if(stateRight == 'L' && (choseDecade)) {
        //1990-
        char ans;
        find1980((char*) &ans);
        if(ans!='n') {
      
          byte key_command =ans - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
      //tell the MP3 Shield to play a track
      result = MP3player.playTrack(key_command);

      //check result, see readme for error codes.
      if(result != 0) {
        Serial.print(F("Error code: "));
        Serial.print(result);
        Serial.println(F(" when trying to play track"));
      } else {
  
        Serial.println(F("Playing:"));
  
        //we can get track info by using the following functions and arguments
        //the functions will extract the requested information, and put it in the array we pass in
        MP3player.trackTitle((char*)&title);
        MP3player.trackArtist((char*)&artist);
        MP3player.trackAlbum((char*)&album);
        MP3player.trackYear((char*)&year);

        //print out the arrays of track information
        Serial.write((byte*)&title, 30);
        Serial.println();
        Serial.print(F("by:  "));
        Serial.write((byte*)&artist, 30);
        Serial.println();
        Serial.print(F("Album:  "));
        Serial.write((byte*)&album, 30);
        Serial.println();
        Serial.print(F("Year:  "));
        Serial.write((byte*)&year, 4);
        Serial.println();
      }
      
        choseDecade=false;
      }
      }
  }
}

char OrientationLEFT()
{
  // accel.readPL() will return a byte containing information
  // about the orientation of the sensor. It will be either
  // PORTRAIT_U, PORTRAIT_D, LANDSCAPE_R, LANDSCAPE_L, or
  // LOCKOUT.
  byte pl = Aleft.readPL();
  switch (pl)
  {
  case PORTRAIT_U:
    return('U');
    break;
  case PORTRAIT_D:
     return('D');
    break;
  case LANDSCAPE_R:
     return('R');
    break;
  case LANDSCAPE_L:
     return('L');
    break;
  case LOCKOUT:
     return('F');
    break;
  }
}
char OrientationRIGHT() {
  // accel.readPL() will return a byte containing information
  // about the orientation of the sensor. It will be either
  // PORTRAIT_U, PORTRAIT_D, LANDSCAPE_R, LANDSCAPE_L, or
  // LOCKOUT.
  byte pl = Aright.readPL();
  switch (pl)
  {
  case PORTRAIT_U:
    return('U');
    break;
  case PORTRAIT_D:
     return('D');
    break;
  case LANDSCAPE_R:
     return('R');
    break;
  case LANDSCAPE_L:
     return('L');
    break;
  case LOCKOUT:
     return('F');
    break;
  }
}
void find2010(char* information) {
  char hey;
  char year[4];
  MP3player.playTrack('1'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='1';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('2'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='2';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('3'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='3';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('4'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='4';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('5'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='5';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('6'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='6';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('7'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='7';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('8'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='8';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('9'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year>="2010") {
    hey='9';
  }
  else {
    hey='n';
  }
  *information=hey;
}
void find2000(char* information) {
  char hey;
  char year[4];
  MP3player.playTrack('1'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='1';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('2'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='2';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('3'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='3';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('4'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='4';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('5'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='5';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('6'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='6';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('7'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='7';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('8'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='8';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('9'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="2009") && (year>="2000")) {
    hey='9';
  }
  else {
    hey='n';
  }
  *information=hey;
}
void find1990(char* information) {
  char hey;
  char year[4];
  MP3player.playTrack('1'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='1';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('2'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='2';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('3'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='3';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('4'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='4';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('5'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='5';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('6'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='6';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('7'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='7';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('8'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='8';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('9'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if((year<="1999") && (year>="1990")) {
    hey='9';
  }
  else {
    hey='n';
  }
  *information=hey;
}
void find1980(char* information) {
  char hey;
  char year[4];
  MP3player.playTrack('1'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='1';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('2'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='2';
  }
  else {
    hey='n';
  }
    MP3player.playTrack('3'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='3';
  }
  else {
    hey='n';
  }
    MP3player.playTrack('4'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='4';
  }
  else {
    hey='n';
  }
    MP3player.playTrack('5'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='5';
  }
  else {
    hey='n';
  }
    MP3player.playTrack('6'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='6';
  }
  else {
    hey='n';
  }
    MP3player.playTrack('7'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='7';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('8'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='8';
  }
  else {
    hey='n';
  }
  MP3player.playTrack('9'-48);
  delay(10);
  MP3player.pauseMusic();
  MP3player.trackYear((char*)&year);
  if(year<="1989") {
    hey='9';
  }
  else {
    hey='n';
  }
  *information=hey;
}
