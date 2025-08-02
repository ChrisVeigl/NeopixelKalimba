/*  
   Neopixel Kalimba, for Zoom Museum Vienna, 2025
   by Michael Strohmann and Chris Veigl

    This Arduino sketch measures up to 10 Piezo (or FSR) sensors connected to the
    Analog Input ports of the Controller. Activity triggers are calculated, so that
    a stronger impact creates a longer on-phase of the trigger value. 
    Trigger values are stored in two bytes (a bit representing a button on/off state) 
    and sent via Serial1 to the Teensy4.1 microcontroller for controlling Leds and Midi
    The byte for the trigger1-group is identified by the MSB being 0, whereas the
    byte for trigger2-group has the MSB set.
    
*/


#define NUMBER_OF_PLAYERS 5
#define ONLY_CHANNEL_N 2     // for testing: set to 0 for sending all channels!
#define OUTPUT_SIGNAL_TRACES 1  // for testing: display signal traces (serial plotter)

#define PIEZO_THRESHOLD 250
#define PIEZO_IMPACT_VAL 20
#define PIEZO_TRIGGER_MAXVALUE 3500
#define PIEZO_DECAY 5
#define SAMPLING_PERIOD 1
#define REPORTING_PERIOD 10

int piezoTrigger[NUMBER_OF_PLAYERS * 2]={0};

uint8_t trigger1Group=0, lastTrigger1Group=0;
uint8_t trigger2Group=0x80, lastTrigger2Group=0x80;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
  static uint32_t reportingTimestamp=0;
  int reportNow=0;
  uint8_t * actTriggerGroup=0;
  
  if (millis()-reportingTimestamp>REPORTING_PERIOD) {
    reportNow=1;
    reportingTimestamp=millis();
  }
  
  for (int i=0; i < NUMBER_OF_PLAYERS * 2; i++) {
    int piezoVal=analogRead(A0+i);
    
    if ((ONLY_CHANNEL_N) && (i!=ONLY_CHANNEL_N-1)) continue;  // bypass unused channels

    if (i%2) actTriggerGroup = &trigger2Group; else actTriggerGroup = &trigger1Group;
    
    if ((piezoVal > PIEZO_THRESHOLD) && (piezoTrigger[i] < PIEZO_TRIGGER_MAXVALUE))
      piezoTrigger[i] += PIEZO_IMPACT_VAL;
  
    if (piezoTrigger[i] > PIEZO_DECAY) {
      piezoTrigger[i] -= PIEZO_DECAY;
      *actTriggerGroup |= (1<<(i>>1));
    }
    else {
      piezoTrigger[i]=0;
      *actTriggerGroup &= ~(1<<(i>>1));
    }

    if (reportNow && OUTPUT_SIGNAL_TRACES) {
      Serial.print(piezoTrigger[i]); Serial.print(",");
    }
  }
  
  if (reportNow) { 
    if (OUTPUT_SIGNAL_TRACES)  Serial.println("");  // newline, needed by Serial Plotter

    // send changes to Teensy4.1
    if (lastTrigger1Group!=trigger1Group) {
       Serial1.write(trigger1Group);
       lastTrigger1Group=trigger1Group;
    }
    if (lastTrigger2Group!=trigger2Group) {
       Serial1.write(trigger2Group);
       lastTrigger2Group=trigger2Group;
    }
  }
  delay(SAMPLING_PERIOD);
}
