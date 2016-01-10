
/*Libraries */
#include <Wire.h> 
#include <SFE_MMA8452Q.h>  

//declaring accelerometers
MMA8452Q Aleft; 
MMA8452Q Aright=MMA8452Q(0x1C);

// variables
char stateLEFT;
char stateRIGHT;

byte LEFTADD=0x1D;
byte RIGHTADD= 0x1C;

boolean flatVol=false;
boolean flatTempo=false;
boolean choseGenre=true;
boolean choseDecade=true;


void setup(){
  
  Serial.begin(115200);
  
  Serial.println("In setup");
  Aleft.init();
  Aright.init();
   
  Serial.println("leaving setup");
  delay(1000);

}

void loop(){
 

 if( Aleft.available()&& Aright.available())
 {  
   //Serial.println("reading accel"); 
   Aleft.read();
   Aright.read();
   stateLEFT=OrientationLEFT();
   stateRIGHT=OrientationRIGHT();
   //Serial.println(stateLEFT);
   //Serial.print(stateRIGHT);
   
   /* VOLUME CONTROL*/
   if(stateLEFT=='D') 
   {
     
     if (stateRIGHT == 'F')
     {
      flatVol=true; 
     }
     if (stateRIGHT =='U' && (flatVol))
     {
        // increase volume
        Serial.println("increment one");
        flatVol=false;
     }
     if (stateRIGHT =='D' && (flatVol))
     {
         //decrease volume
        Serial.println("decrement one");
        flatVol=false; 
     }
   }
   
   /*GENRE CONTROL*/
   if(stateLEFT=='U') 
   {
      
      if(stateRIGHT=='F')
      {
         choseGenre=false; 
      }
       if (stateRIGHT =='U' && !choseGenre) // ROCK
       {
          Serial.println("Rock Song");
          choseGenre=true;
          
       }
       if (stateRIGHT =='R' && !choseGenre)
       {
          Serial.println("Country Song");
          choseGenre=true; 
       }
       if (stateRIGHT =='D' && !choseGenre)
       {
          Serial.println("R&B Song");
          choseGenre=true; 
       }
       if (stateRIGHT =='L' && !choseGenre)
       {
          Serial.println("Rap Song");
          choseGenre=true; 
       }
   }
   /* TEMPO CONTROL*/
   if(stateLEFT=='R') 
   {
     
     
     if (stateRIGHT == 'F')
     {
      flatTempo=true; 
     }
     if (stateRIGHT =='U' && (flatTempo))
     {
        // increase volume
        Serial.println("Faster ");
        flatTempo=false;
     }
     if (stateRIGHT =='D' && (flatTempo))
     {
         //decrease volume
        Serial.println("Slower");
        flatTempo=false; 
     }
   }
     /*DECADE CONTROL*/
   if(stateLEFT=='L') 
   {
      
      if(stateRIGHT=='F')
      {
         choseDecade=false; 
      }
       if (stateRIGHT =='U' && !choseDecade) // ROCK
       {
          Serial.println("Rock Song");
          choseDecade=true;
          
       }
       if (stateRIGHT =='R' && !choseDecade)
       {
          Serial.println("Country Song");
          choseDecade=true; 
       }
       if (stateRIGHT =='D' && !choseDecade)
       {
          Serial.println("R&B Song");
          choseDecade=true; 
       }
       if (stateRIGHT =='L' && !choseDecade)
       {
          Serial.println("Rap Song");
          choseDecade=true; 
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
char OrientationRIGHT()
{
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
