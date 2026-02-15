/*
 * Macro Definitions
 */
#define SPEC_TRG 1 //not used
#define SPEC_ST 2 //Start, Green, Pin 2
#define SPEC_CLK 3 // Clock, Purple, Pin 3
#define SPEC_VIDEO A0 // Video, Blue, ADC Pin 0

#define SPEC_CHANNELS 288 // New Spec Channel

uint16_t data[SPEC_CHANNELS];


void setup(){

  analogReadResolution(12); //set ADC to 12 bit 

  //Set desired pins to OUTPUT
  pinMode(SPEC_CLK, OUTPUT);
  pinMode(SPEC_ST, OUTPUT);
  digitalWrite(SPEC_ST, LOW); // Set SPEC_ST Low

  pinMode(SPEC_TRG, OUTPUT);
  digitalWrite(SPEC_TRG, LOW); // keep TRG low to avoid accidental triggers - this is pin #1

  digitalWrite(SPEC_CLK, HIGH); // Set SPEC_CLK High

  Serial.begin(115200); // Baud Rate set to 115200
  
}

/*
 * This functions reads spectrometer data from SPEC_VIDEO
 * Look at the Timing Chart in the Datasheet for more info
 */
void readSpectrometer(){

  int delayTime = 1; // delay time

  // Start clock cycle and set start pulse to signal start
  digitalWrite(SPEC_CLK, LOW);
  delayMicroseconds(delayTime); //doesnt seem to do anything
  digitalWrite(SPEC_CLK, HIGH);
  delayMicroseconds(delayTime); //does seem to effect readout integration 
  digitalWrite(SPEC_CLK, LOW);
  digitalWrite(SPEC_ST, HIGH);
  delayMicroseconds(delayTime); //no change

  //Sample for a period of time
  for(int i = 0; i < 15; i++){

      digitalWrite(SPEC_CLK, HIGH);
      delayMicroseconds(delayTime); // this also seems to work (changes the integration time)
      digitalWrite(SPEC_CLK, LOW);
      delayMicroseconds(delayTime); // and this too, but in a different way than above
 
  }

  //Set SPEC_ST to low
  digitalWrite(SPEC_ST, LOW);

  //Sample for a period of time
  for(int i = 0; i < 85; i++){

      digitalWrite(SPEC_CLK, HIGH);
      delayMicroseconds(delayTime); // Works!!
      digitalWrite(SPEC_CLK, LOW);
      delayMicroseconds(delayTime); // but this works tooo.... why?
      
  }

  //One more clock pulse before the actual read
  digitalWrite(SPEC_CLK, HIGH);
  delayMicroseconds(delayTime); //doesnt break but doesnt do anything
  digitalWrite(SPEC_CLK, LOW);
  delayMicroseconds(delayTime); //doesnt break but doesnt do anything

  //Read from SPEC_VIDEO
  for(int i = 0; i < SPEC_CHANNELS; i++){

      data[i] = analogRead(SPEC_VIDEO);
      
      digitalWrite(SPEC_CLK, HIGH);
      delayMicroseconds(delayTime); //doesnt break but doesnt do anything
      digitalWrite(SPEC_CLK, LOW);
      delayMicroseconds(delayTime); //this breaks it 
        
  }

  //Set SPEC_ST to high
  digitalWrite(SPEC_ST, HIGH);

  //Sample for a small amount of time
  for(int i = 0; i < 7; i++){
    
      digitalWrite(SPEC_CLK, HIGH);
      delayMicroseconds(delayTime); // this is now effecting values, I swear it was not before. Using value of 20000 - non linear changes though...
      digitalWrite(SPEC_CLK, LOW);
      delayMicroseconds(delayTime);
    
  }

  digitalWrite(SPEC_CLK, HIGH);
  delayMicroseconds(delayTime);
  
}

/*
 * The function below prints out data to the terminal or 
 * processing plot
 */
void printData(){
  
  for (int i = 0; i < SPEC_CHANNELS; i++){
    
    Serial.print(data[i]);
    Serial.print(',');
    
  }
  
  Serial.print("\n");
}

void loop(){
   
  readSpectrometer();
  printData();
  delay(10);  
   
}

