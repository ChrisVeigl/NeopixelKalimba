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

#include <math.h>

// #define ONLY_SHOW_CHANNEL_TRACE 0  // only show this ADC channel raw data with high speed sampling (for testing)
#define OUTPUT_SIGNAL_TRACES 1     // for testing: display signal traces (serial plotter)
#define NUMBER_OF_PLAYERS 5
#define SAMPLING_PERIOD 1          // delay for sampling loop (in milliseconds)
#define REPORTING_PERIOD 10

#define PIEZO_THRESHOLD 5
#define PIEZO_IMPACT_VAL 10
#define PIEZO_TRIGGER_MAXVALUE 1000
#define PIEZO_DECAY 3

// IIR lowpass filter parameters
#define LP_CUTOFF   1.0f         // Cutoff frequency (Hz)
#define LP_Q        0.70710678f  // Quality factor for a Butterworth response
#define FS          1000.0f      // Sampling rate (Hz)

static const float w0    = 2.0f * M_PI * (LP_CUTOFF / FS);
static const float alpha = sinf(w0) / (2.0f * LP_Q);
static const float b0    =  (1.0f - cosf(w0)) / 2.0f;
static const float b1    =   1.0f - cosf(w0);
static const float b2    =  (1.0f - cosf(w0)) / 2.0f;
static const float a0    =   1.0f + alpha;
static const float a1    =  -2.0f * cosf(w0);
static const float a2    =   1.0f - alpha;
static const float bb0   = b0 / a0;
static const float bb1   = b1 / a0;
static const float bb2   = b2 / a0;
static const float aa1   = a1 / a0;
static const float aa2   = a2 / a0;

typedef struct {
    float xv1, xv2;  // x[n-1], x[n-2]
    float yv1, yv2;  // y[n-1], y[n-2]
} IIRLowPass2State;

static inline void iir_lowpass2_init(IIRLowPass2State *st) {
    st->xv1 = st->xv2 = 0.0f;
    st->yv1 = st->yv2 = 0.0f;
}

static inline int iir_lowpass2_process(IIRLowPass2State *st, int x) {
    float xn = (float)x;
    // biquad difference equation
    float yn = bb0 * xn
             + bb1 * st->xv1
             + bb2 * st->xv2
             - aa1 * st->yv1
             - aa2 * st->yv2;
    // shift delay-lines
    st->xv2 = st->xv1;
    st->xv1 = xn;
    st->yv2 = st->yv1;
    st->yv1 = yn;
    return (int)yn;
}

int piezoTrigger[NUMBER_OF_PLAYERS * 2]={0};
IIRLowPass2State filterState[NUMBER_OF_PLAYERS*2];

uint8_t trigger1Group=0, lastTrigger1Group=0;
uint8_t trigger2Group=0x80, lastTrigger2Group=0x80;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  for(int ch = 0; ch < NUMBER_OF_PLAYERS*2; ch++) {
      iir_lowpass2_init(&filterState[ch]);
  }
}

void loop() {
  #ifdef ONLY_SHOW_CHANNEL_TRACE
    showChannelTrace(ONLY_SHOW_CHANNEL_TRACE);  return;   // for testing: only show raw values of one channel!
  #endif
  
  static uint32_t reportingTimestamp=0;
  int reportNow=0;
  uint8_t * actTriggerGroup=0;
  
  if (millis()-reportingTimestamp>REPORTING_PERIOD) {
    reportNow=1;
    reportingTimestamp=millis();
  }
  
  for (int i=0; i < NUMBER_OF_PLAYERS * 2; i++) {
    int signal=analogRead(A0+i);
    int piezoVal=signal-iir_lowpass2_process(&filterState[i],signal);
    
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

void showChannelTrace(int c) {
  static int sum=0;
  static int count=0;
  int val=analogRead(c);
  if (val<3) val=0;
  sum+=val;
  if (count++ >= 100) {
    Serial.println(sum);
    count=0;
    sum=0;
  }
  delayMicroseconds(200);
}
