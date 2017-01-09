// Duino Tag release V1.01
// Laser Tag for the arduino based on the Miles Tag Protocol.
// By J44industries: www.J44industries.blogspot.com
// For information on building your own DuinoTagger go to: http://www.instructables.com/member/j44/
//
// Much credit deserves to go to Duane O'Brien if it had not been for the excellent DuinoTag tutorials he wrote I would have never been able to write this code.
// Duane's tutorials are highly recommended reading in order to gain a better understanding of the arduino and IR communication. See his site http://aterribleidea.com/duino-tag-resources/
//
// This code sets out the basics for arduino based laser tag system and tries to stick to the miles tag protocol where possible.
// MilesTag details: http://www.lasertagparts.com/mtdesign.htm
// There is much scope for expanding the capabilities of this system, and hopefully the game will continue to evolve for some time to come.
// Licence: Attribution Share Alike: Give credit where credit is due, but you can do what you like with the code.
// If you have code improvements or additions please go to http://duinotag.blogspot.com
//



// Digital IO's
int triggerPin = 3; // Push button for primary fire. Low = pressed
int trigger2Pin = 13; // Push button for secondary fire. Low = pressed
int speakerPin = 4; // Direct output to piezo sounder/speaker
int audioPin = 9; // Audio Trigger. Can be used to set off sounds recorded in the kind of electronics you can get in greetings card that play a custom message.
int lifePin = 6; // An analogue output (PWM) level corresponds to remaining life. Use PWM pin: 3,5,6,9,10 or 11. Can be used to drive LED bar graphs. eg LM3914N
int ammoPin = 5; // An analogue output (PWM) level corresponds to remaining ammunition. Use PWM pin: 3,5,6,9,10 or 11.
int hitPin = 7; // LED output pin used to indicate when the player has been hit.
int IRtransmitPin = 12; // Primary fire mode IR transmitter pin: Use pins 2,4,7,8,12 or 13. DO NOT USE PWM pins!! More info: http://j44industries.blogspot.com/2009/09/arduino-frequency-generation.html#more
int IRtransmit2Pin = 8; // Secondary fire mode IR transmitter pin: Use pins 2,4,7,8,12 or 13. DO NOT USE PWM pins!!
int IRreceivePin = 2; // The pin that incoming IR signals are read from
int IRreceive2Pin = 11; // Allows for checking external sensors are attached as well as distinguishing between sensor locations (eg spotting head shots)
int ReloadPin = 10; // Push button for primary Reload. Low = pressed
#define IRpin_PIN PIND
// Minimum gun requirements: trigger, receiver, IR led, hit LED.

// Player and Game details
int myTeamID = 2; // 1-7 (0 = system message)
int myPlayerID = 1; // Player ID
int myGameID = 0; // Interprited by configureGane subroutine; allows for quick change of game types.
int myWeaponID = 0; // Deffined by gameType and configureGame subroutine.
int myWeaponHP = 0; // Deffined by gameType and configureGame subroutine.
int maxAmmo = 0; // Deffined by gameType and configureGame subroutine.
int maxLife = 0; // Deffined by gameType and configureGame subroutine.
int automatic = 0; // Deffined by gameType and configureGame subroutine. Automatic fire 0 = Semi Auto, 1 = Fully Auto.
int automatic2 = 0; // Deffined by gameType and configureGame subroutine. Secondary fire auto?

//Incoming signal Details
int received[18]; // Received data: received[0] = which sensor, received[1] - [17] byte1 byte2 parity (Miles Tag structure)
int check = 0; // Variable used in parity checking

// Stats
int ammo = 0; // Current ammunition
int life = 0; // Current life

// Code Variables
int timeOut = 0; // Deffined in frequencyCalculations (IRpulse + 50)
int FIRE = 0; // 0 = don't fire, 1 = Primary Fire, 2 = Secondary Fire
int TR = 0; // Trigger Reading
int LTR = 0; // Last Trigger Reading
int T2R = 0; // Trigger 2 Reading (For secondary fire)
int LT2R = 0; // Last Trigger 2 Reading (For secondary fire)
int RL = 0; // Trigger Reading reload button
bool debug = 1; //sets debug mode 1 = Debug ON, 0 = Debug OFF

// Signal Properties
int IRpulse = 600; // Basic pulse duration of 600uS MilesTag standard 4*IRpulse for header bit, 2*IRpulse for 1, 1*IRpulse for 0.
int IRfrequency = 38; // Frequency in kHz Standard values are: 38kHz, 40kHz. Choose dependant on your receiver characteristics
int IRt = 0; // LED on time to give correct transmission frequency, calculated in setup.
int IRpulses = 0; // Number of oscillations needed to make a full IRpulse, calculated in setup.
int header = 4; // Header lenght in pulses. 4 = Miles tag standard
int maxSPS = 10; // Maximum Shots Per Seconds. Not yet used.
int TBS = 0; // Time between shots. Not yet used.

// Transmission data
int byte1[8]; // String for storing byte1 of the data which gets transmitted when the player fires.
int byte2[8]; // String for storing byte1 of the data which gets transmitted when the player fires.
int myParity = 0; // String for storing parity of the data which gets transmitted when the player fires.

// Received data
int memory = 10; // Number of signals to be recorded: Allows for the game data to be reviewed after the game, no provision for transmitting / accessing it yet though.
int hitNo = 0; // Hit number
// Byte1
int player[10]; // Array must be as large as memory
int team[10]; // Array must be as large as memory
// Byte2
int weapon[10]; // Array must be as large as memory
int hp[10]; // Array must be as large as memory
int parity[10]; // Array must be as large as memory


// the maximum pulse we'll listen for - 65 milliseconds is a long time
#define MAXPULSE 65000

// what our timing resolution should be, larger is better
// as its more 'precise' - but too large and you wont get
// accurate timing
#define RESOLUTION 20

// we will store up to 100 pulse pairs (this is -a lot-)
uint16_t pulses[100][2]; // pair is high and low pulse
uint8_t currentpulse = 0; // index for pulses we're storing

void setup() {
  // Serial coms set up to help with debugging.
  Serial.begin(9600);
  Serial.println("Startup...");
  // Pin declarations
  pinMode(triggerPin, INPUT);
  pinMode(trigger2Pin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(audioPin, OUTPUT);
  pinMode(lifePin, OUTPUT);
  pinMode(ammoPin, OUTPUT);
  pinMode(hitPin, OUTPUT);
  pinMode(IRtransmitPin, OUTPUT);
  pinMode(IRtransmit2Pin, OUTPUT);
  pinMode(IRreceivePin, INPUT);
  pinMode(IRreceive2Pin, INPUT);
  pinMode(ReloadPin, INPUT);

  frequencyCalculations(); // Calculates pulse lengths etc for desired frequency
  configureGame(); // Look up and configure game details
  tagCode(); // Based on game details etc works out the data that will be transmitted when a shot is fired


  digitalWrite(triggerPin, HIGH); // Not really needed if your circuit has the correct pull up resistors already but doesn't harm
  digitalWrite(trigger2Pin, HIGH); // Not really needed if your circuit has the correct pull up resistors already but doesn't harm
  digitalWrite(ReloadPin, HIGH); // Not really needed if your circuit has the correct pull up resistors already but doesn't harm

  for (int i = 1; i < 254; i++) { // Loop plays start up noise
    analogWrite(ammoPin, i);
    playTone((3000 - 9 * i), 2);
  }

  // Next 4 lines initialise the display LEDs
  analogWrite(ammoPin, ((int) ammo));
  analogWrite(lifePin, ((int) life));
  lifeDisplay();
  ammoDisplay();
  GameInfo();
  Serial.println("Ready....");

}


// Main loop most of the code is in the sub routines
void loop() {

  uint16_t highpulse, lowpulse; // temporary storage timing
  highpulse = lowpulse = 0; // start out with no pulse length

  while (IRpin_PIN & (1 << IRreceivePin)) {
    // pin is still HIGH
    // count off another few microseconds
    highpulse++;
    delayMicroseconds(RESOLUTION);
    //Add triggers here
    if (FIRE != 0) {
        shoot();
        ammoDisplay();
    }
    triggers();
    //to here 
    
    // If the pulse is too long, we 'timed out' - either nothing
    // was received or the code is finished, so print what
    // we've grabbed so far, and then reset
    if ((highpulse >= MAXPULSE) && (currentpulse != 0)) {
      printpulses();
      currentpulse = 0;
    }
  }
  // we didn't time out so lets stash the reading
  pulses[currentpulse][0] = highpulse;

  // same as above
  while (! (IRpin_PIN & _BV(IRreceivePin))) {
    // pin is still LOW
    lowpulse++;
    delayMicroseconds(RESOLUTION);
    if ((lowpulse >= MAXPULSE) && (currentpulse != 0)) {
      printpulses();
      currentpulse = 0;

    }
 
  }
  pulses[currentpulse][1] = lowpulse;

  // we read one high-low pulse successfully, continue!
  currentpulse++;
} //END LOOP

/*OLD STUFF  
  receiveIR();
  //listenForIR();
  if (FIRE != 0) {
    shoot();
    ammoDisplay();
  }
  triggers();
}
*/

// SUB ROUTINES

void GameInfo(){
  Serial.println("Game info:...");
  Serial.print("Team ID: ");
  Serial.println(myTeamID);
  Serial.print("Player ID: ");
  Serial.println(myPlayerID);
        
}
void ammoDisplay() { // Updates Ammo LED output
  float ammoF;
  ammoF = (260 / maxAmmo) * ammo;
  if (ammoF <= 0) {
    ammoF = 0;
  }
  if (ammoF > 255) {
    ammoF = 255;
  }
  analogWrite(ammoPin, ((int) ammoF));
  Serial.print("Ammo:");
  Serial.println(ammoF);
}

void lifeDisplay() { // Updates Ammo LED output
  float lifeF;
  lifeF = (260 / maxLife) * life;
  if (lifeF <= 0) {
    lifeF = 0;
  }
  if (lifeF > 255) {
    lifeF = 255;
  }
  analogWrite(lifePin, ((int) lifeF));
   Serial.print("Life:");
  Serial.println(lifeF);
}

void interpritReceived() { // After a message has been received by the ReceiveIR subroutine this subroutine decidedes how it should react to the data
  if (hitNo == memory) {
    hitNo = 0; // hitNo sorts out where the data should be stored if statement means old data gets overwritten if too much is received
  }
  team[hitNo] = 0;
  player[hitNo] = 0;
  weapon[hitNo] = 0;
  hp[hitNo] = 0;
  // Next few lines Effectivly converts the binary data into decimal
  // Im sure there must be a much more efficient way of doing this
  if (received[1] == 1) {
    team[hitNo] = team[hitNo] + 4;
  }
  if (received[2] == 1) {
    team[hitNo] = team[hitNo] + 2;
  }
  if (received[3] == 1) {
    team[hitNo] = team[hitNo] + 1;
  }

  if (received[4] == 1) {
    player[hitNo] = player[hitNo] + 16;
  }
  if (received[5] == 1) {
    player[hitNo] = player[hitNo] + 8;
  }
  if (received[6] == 1) {
    player[hitNo] = player[hitNo] + 4;
  }
  if (received[7] == 1) {
    player[hitNo] = player[hitNo] + 2;
  }
  if (received[8] == 1) {
    player[hitNo] = player[hitNo] + 1;
  }

  if (received[9] == 1) {
    weapon[hitNo] = weapon[hitNo] + 4;
  }
  if (received[10] == 1) {
    weapon[hitNo] = weapon[hitNo] + 2;
  }
  if (received[11] == 1) {
    weapon[hitNo] = weapon[hitNo] + 1;
  }

  if (received[12] == 1) {
    hp[hitNo] = hp[hitNo] + 16;
  }
  if (received[13] == 1) {
    hp[hitNo] = hp[hitNo] + 8;
  }
  if (received[14] == 1) {
    hp[hitNo] = hp[hitNo] + 4;
  }
  if (received[15] == 1) {
    hp[hitNo] = hp[hitNo] + 2;
  }
  if (received[16] == 1) {
    hp[hitNo] = hp[hitNo] + 1;
  }

  parity[hitNo] = received[17];

  Serial.print("Hit No: ");
  Serial.print(hitNo);
  Serial.print(" Player: ");
  Serial.print(player[hitNo]);
  Serial.print(" Team: ");
  Serial.print(team[hitNo]);
  Serial.print(" Weapon: ");
  Serial.print(weapon[hitNo]);
  Serial.print(" HP: ");
  Serial.print(hp[hitNo]);
  Serial.print(" Parity: ");
  Serial.println(parity[hitNo]);


  //This is probably where more code should be added to expand the game capabilities at the moment the code just checks that the received data was not a system message and deducts a life if it wasn't.
  if (player[hitNo] != 0) {
    hit();
  }
  hitNo++ ;
}

void shoot() {
  if (FIRE == 1) { // Has the trigger been pressed?
    if (debug == 1){
      Serial.println("FIRE 1");
    }
    sendPulse(IRtransmitPin, 4); // Transmit Header pulse, send pulse subroutine deals with the details
    delayMicroseconds(IRpulse);
    if (debug == 1){
        Serial.println("Sending byte1.................");
    }
    for (int i = 0; i < 8; i++) { // Transmit Byte1
     if (debug == 1){
        Serial.print("Byte");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(byte1[i]);
     }
      if (byte1[i] == 1) {
        sendPulse(IRtransmitPin, 1);
        //Serial.print("1 ");
      }
      //else{Serial.print("0 ");}
      sendPulse(IRtransmitPin, 1);
      delayMicroseconds(IRpulse);
    }

     if (debug == 1){
        Serial.println("Sending byte2.................");
    }
    for (int i = 0; i < 8; i++) { // Transmit Byte2
      if (debug == 1){
        Serial.print("Byte");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(byte1[i]);
     }
      if (byte2[i] == 1) {
        sendPulse(IRtransmitPin, 1);
        // Serial.print("1 ");
      }
      //else{Serial.print("0 ");}
      sendPulse(IRtransmitPin, 1);
      delayMicroseconds(IRpulse);
    }

    if (myParity == 1) { // Parity
      sendPulse(IRtransmitPin, 1);
    }
    sendPulse(IRtransmitPin, 1);
    if (debug == 1){
        Serial.print("Sent Parity:");
        Serial.println(myParity);
    }
    delayMicroseconds(IRpulse);
  }


  if (FIRE == 2) { // Where a secondary fire mode would be added
    Serial.println("FIRE 2");
    sendPulse(IRtransmitPin, 4); // Header
    Serial.println("DONE 2");
  }
  FIRE = 0;
  ammo = ammo - 1;
}


void sendPulse(int pin, int length) { // importing variables like this allows for secondary fire modes etc.
  // This void genertates the carrier frequency for the information to be transmitted
  int i = 0;
  int o = 0; 
  while ( i < length ) {
    i++;
    while ( o < IRpulses ) {
      o++;
      digitalWrite(pin, HIGH);
      delayMicroseconds(IRt);
      digitalWrite(pin, LOW);
      delayMicroseconds(IRt);
    }
  }
}


void triggers() { // Checks to see if the triggers have been presses
  LTR = TR; // Records previous state. Primary fire
  LT2R = T2R; // Records previous state. Secondary fire
  TR = digitalRead(triggerPin); // Looks up current trigger button state
  T2R = digitalRead(trigger2Pin); // Looks up current trigger button state
  RL = digitalRead(ReloadPin); // Looks up current reload button state
  // Code looks for changes in trigger state to give it a semi automatic shooting behaviour
  if (RL == HIGH){
    reload();
  }
  if (TR != LTR && TR == LOW) {
    FIRE = 1;
  }
  if (T2R != LT2R && T2R == LOW) {
    FIRE = 2;
  }
  if (TR == LOW && automatic == 1) {
    FIRE = 1;
  }
  if (T2R == LOW && automatic2 == 1) {
    FIRE = 2;
  }
  if (FIRE == 1 || FIRE == 2) {
    if (ammo < 1) {
      FIRE = 0;
      noAmmo();
    }
    if (life < 1) {
      FIRE = 0;
      dead();
    }
    // Fire rate code to be added here
  }

}


void configureGame() { // Where the game characteristics are stored, allows several game types to be recorded and you only have to change one variable (myGameID) to pick the game.
  if (myGameID == 0) {
    myWeaponID = 1;
    maxAmmo = 30;
    ammo = 8;
    maxLife = 3;
    life = 3;
    myWeaponHP = 1;
  }
  if (myGameID == 1) {
    myWeaponID = 1;
    maxAmmo = 100;
    ammo = 100;
    maxLife = 10;
    life = 10;
    myWeaponHP = 2;
  }
}


void frequencyCalculations() { // Works out all the timings needed to give the correct carrier frequency for the IR signal
  IRt = (int) (500 / IRfrequency);
  IRpulses = (int) (IRpulse / (2 * IRt));
  IRt = IRt - 4;
  // Why -4 I hear you cry. It allows for the time taken for commands to be executed.
  // More info: http://j44industries.blogspot.com/2009/09/arduino-frequency-generation.html#more
  if (debug == 1){
      Serial.print("Oscilation time period /2: ");
      Serial.println(IRt);
      Serial.print("Pulses: ");
      Serial.println(IRpulses);
  }
  timeOut = IRpulse + 50; // Adding 50 to the expected pulse time gives a little margin for error on the pulse read time out value
}


void tagCode() { // Works out what the players tagger code (the code that is transmitted when they shoot) is
  byte1[0] = myTeamID >> 2 & B1;
  byte1[1] = myTeamID >> 1 & B1;
  byte1[2] = myTeamID >> 0 & B1;

  byte1[3] = myPlayerID >> 4 & B1;
  byte1[4] = myPlayerID >> 3 & B1;
  byte1[5] = myPlayerID >> 2 & B1;
  byte1[6] = myPlayerID >> 1 & B1;
  byte1[7] = myPlayerID >> 0 & B1;


  byte2[0] = myWeaponID >> 2 & B1;
  byte2[1] = myWeaponID >> 1 & B1;
  byte2[2] = myWeaponID >> 0 & B1;

  byte2[3] = myWeaponHP >> 4 & B1;
  byte2[4] = myWeaponHP >> 3 & B1;
  byte2[5] = myWeaponHP >> 2 & B1;
  byte2[6] = myWeaponHP >> 1 & B1;
  byte2[7] = myWeaponHP >> 0 & B1;

  myParity = 0;
  for (int i = 0; i < 8; i++) {
    if (byte1[i] == 1) {
      myParity = myParity + 1;
    }
    if (byte2[i] == 1) {
      myParity = myParity + 1;
    }
    myParity = myParity >> 0 & B1;
    
  }

  // Next few lines print the full tagger code.
  Serial.print("Byte1: ");
  Serial.print(byte1[0]);
  Serial.print(byte1[1]);
  Serial.print(byte1[2]);
  Serial.print(byte1[3]);
  Serial.print(byte1[4]);
  Serial.print(byte1[5]);
  Serial.print(byte1[6]);
  Serial.print(byte1[7]);
  Serial.println();

  Serial.print("Byte2: ");
  Serial.print(byte2[0]);
  Serial.print(byte2[1]);
  Serial.print(byte2[2]);
  Serial.print(byte2[3]);
  Serial.print(byte2[4]);
  Serial.print(byte2[5]);
  Serial.print(byte2[6]);
  Serial.print(byte2[7]);
  Serial.println();

  Serial.print("Parity: ");
  Serial.print(myParity);
  Serial.println();
}


void playTone(int tone, int duration) { // A sub routine for playing tones like the standard arduino melody example
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}


void dead() { // void determines what the tagger does when it is out of lives
  // Makes a few noises and flashes some lights
  for (int i = 1; i < 254; i++) {
    analogWrite(ammoPin, i);
    playTone((1000 + 9 * i), 2);
  }
  analogWrite(ammoPin, ((int) ammo));
  analogWrite(lifePin, ((int) life));
  Serial.println("DEAD");

  for (int i = 0; i < 10; i++) {
    analogWrite(ammoPin, 255);
    digitalWrite(hitPin, HIGH);
    delay (500);
    analogWrite(ammoPin, 0);
    digitalWrite(hitPin, LOW);
    delay (500);
  }
  if (debug == 1){
    Serial.print ("Revive.......");
    life = 3;
  }
  
}


void noAmmo() { // Make some noise and flash some lights when out of ammo
  digitalWrite(hitPin, HIGH);
  playTone(500, 100);
  playTone(1000, 100);
  digitalWrite(hitPin, LOW);
  Serial.println("OutOf Ammo");
}

void reload(){
   ammo=30;
   if (debug == 1){
    Serial.print ("Reloading....."); 
    ammoDisplay();
   }
   
   playTone(500, 600);
}

void hit() { // Make some noise and flash some lights when you get shot
  digitalWrite(hitPin, HIGH);
  life = life - hp[hitNo];
  Serial.print("Life: ");
  Serial.println(life);
  playTone(500, 500);
  if (life <= 0) {
    dead();
  }
  digitalWrite(hitPin, LOW);
  lifeDisplay();
}


void printpulses(void) {

  if (debug == 1){
      Serial.println("\n\r\n\rReceived: \n\rOFF \tON");
      for (uint8_t i = 0; i < currentpulse; i++) {
          Serial.print(pulses[i][0] * RESOLUTION, DEC);
          Serial.print(" usec, ");
          Serial.print(pulses[i][1] * RESOLUTION, DEC);
          Serial.println(" usec");
      }
      // print it in a 'array' format
      Serial.print("Number of pulses:");
      Serial.println(currentpulse);
  }
  for (uint8_t i = 0; i < currentpulse; i++) {
    if ((pulses[i][1] * RESOLUTION) < 800) {
      //Serial.println (": 0");
      received[i] = 0;
    }
    else {
      //Serial.println (": 1");
      received[i] = 1;
    }

  }
  int check = 0;
  int error = 0;

  if (debug == 1){
        Serial.println("Recived Byte1.....");
        for (int x = 1; x <= 8; x++) {
              Serial.print("Byte");
              Serial.print(x);
              Serial.print(": ");
              Serial.println(received[x]);
        }
        Serial.println("Recived Byte2.....");
        for (int x = 9; x <= 16; x++) {
              Serial.print("Byte");
              Serial.print(x);
              Serial.print(": ");
              Serial.println(received[x]);
        }
              Serial.print("Recived Parity: ");
              Serial.println(received[currentpulse-1]);
  }    
  for (int x = 1; x <= 16; x++) {
    if (received[x] == 1) {
      check = check + 1;
    }
     check = check >> 0 & B1; 
    }
  if (debug == 1){
    Serial.print("Calculated Parity: ");
    Serial.println(check);
  }
  if (check != received[currentpulse-1]) {
    error = 1;
  }
  if (error == 0) {
    if (debug == 1){
      Serial.println("Valid Signal");
    }
    interpritReceived();
  }
  else {
    Serial.println("ERROR");
  }

}
