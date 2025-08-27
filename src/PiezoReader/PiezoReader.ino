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

#define ONLY_SHOW_CHANNEL_TRACE 0  // only show this ADC channel raw data with high speed sampling (for testing)
#define OUTPUT_SIGNAL_TRACES 1    // for testing: display signal traces (serial plotter)
#define NUMBER_OF_PLAYERS 5
#define SAMPLING_PERIOD 1          // delay for sampling loop (in milliseconds)
#define REPORTING_PERIOD 10        // send updates to Teensy4.1 and Terminal every 10 ms

#define TRIGGER_SIGNAL_LOWPASS_CUTOFF   35.0f   // cutoff frequency for trigger signal
#define BASELINE_SIGNAL_LOWPASS_CUTOFF   0.8f   // cutoff frequency for baseline signal

#define PIEZO_THRESHOLD 8
#define FSR_THRESHOLD 40

#define SENSOR_THRESHOLD FSR_THRESHOLD

#define PIEZO_IMPACT_VAL 20
#define PIEZO_TRIGGER_MAXVALUE 1000
#define PIEZO_DECAY 10

// IIR lowpass filter parameters

#define FS  1000.0f  // Sampling rate (Hz)

typedef struct {
    // coefficients
    float b0, b1, b2;
    float a1, a2;
    // state
    float xv1, xv2;  // x[n-1], x[n-2]
    float yv1, yv2;  // y[n-1], y[n-2]
} IIRLowPassState;

// Initialize a filter instance
//   f    = cutoff freq in Hz (e.g. 20.0f)
//   Q    = quality factor (e.g. 0.707f for Butterworth)
//   st   = pointer to your filter instance
static inline void iir_lowpass2_init(IIRLowPassState *st, float f, float Q) {
    float w0    = 2.0f * M_PI * (f / FS);
    float alpha = sinf(w0) / (2.0f * Q);

    float b0n =  (1.0f - cosf(w0)) / 2.0f;
    float b1n =   1.0f - cosf(w0);
    float b2n =  (1.0f - cosf(w0)) / 2.0f;
    float a0n =   1.0f + alpha;
    float a1n =  -2.0f * cosf(w0);
    float a2n =   1.0f - alpha;
    st->b0 = b0n / a0n;
    st->b1 = b1n / a0n;
    st->b2 = b2n / a0n;
    st->a1 = a1n / a0n;
    st->a2 = a2n / a0n;
    st->xv1 = st->xv2 = 0.0f;
    st->yv1 = st->yv2 = 0.0f;
}

// Returns the filtered sample (int).
static inline int iir_lowpass2_process(IIRLowPassState *st, int x) {
    float xn = (float)x;
    // standard biquad difference equation:
    float yn = st->b0 * xn
             + st->b1 * st->xv1
             + st->b2 * st->xv2
             - st->a1 * st->yv1
             - st->a2 * st->yv2;

    // shift delay‐lines
    st->xv2 = st->xv1;
    st->xv1 = xn;
    st->yv2 = st->yv1;
    st->yv1 = yn;
    return (int)yn;
}

IIRLowPassState filterState[NUMBER_OF_PLAYERS * 4];   // 2 triggers per player, 2 signals per trigger

int piezoTrigger[NUMBER_OF_PLAYERS * 2]={0};
uint8_t trigger1Group=0, lastTrigger1Group=0;
uint8_t trigger2Group=0x80, lastTrigger2Group=0x80;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  for(int i = 0; i < NUMBER_OF_PLAYERS*2; i++) {
    iir_lowpass2_init(&filterState[i*2], TRIGGER_SIGNAL_LOWPASS_CUTOFF, 0.707f);
    iir_lowpass2_init(&filterState[i*2+1], BASELINE_SIGNAL_LOWPASS_CUTOFF, 0.707f);
  }
}

void loop() {
 
  static uint32_t reportingTimestamp=0;
  static int fps=0;
  int reportNow=0;
  uint8_t * actTriggerGroup=0;

  uint32_t now=millis();
  if (now-reportingTimestamp>REPORTING_PERIOD) {
    reportNow=1;
    reportingTimestamp=now;
  }
  
  for (int i=0; i < NUMBER_OF_PLAYERS * 2; i++) {
    int raw=analogRead(A0+i);
    int signal=iir_lowpass2_process(&filterState[i*2],raw);      //  20 Hz LP
    int baseline=iir_lowpass2_process(&filterState[i*2+1],raw);  // 0.5 Hz LP
    int sensorVal=signal-baseline;

    if (ONLY_SHOW_CHANNEL_TRACE && reportNow)  Serial.printf("%d,%d,",signal, baseline);
    
    if (i%2) actTriggerGroup = &trigger2Group; else actTriggerGroup = &trigger1Group;
    
    if ((sensorVal > SENSOR_THRESHOLD) && (piezoTrigger[i] < PIEZO_TRIGGER_MAXVALUE))
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
    if (OUTPUT_SIGNAL_TRACES || ONLY_SHOW_CHANNEL_TRACE )  { 
      // Serial.print(fps); fps=0;
      Serial.println("");  // newline, needed by Serial Plotter
    }

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
  fps++;
  while (millis()-now < SAMPLING_PERIOD);   // try to keep the loop update rate at sampling frequency
}

/*
// fast sampling and display of one channel  (for debugging)
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

*/
