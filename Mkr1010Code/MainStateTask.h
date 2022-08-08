// MainStateTask.h
#ifndef MAINSTATETASK_H
#define MAINSTATETASK_H

struct SummaryData_ {
    int16_t finalVolume; // Scale-based volume after correcting for visual measurement
    int16_t visualVolume; 
    int16_t endT;  // Time when flow < 2mL/sec
    int16_t startT;   // Index in buffer when 10mL measured
    float Qmax;
    int16_t maxNdx;  // Index of maximum derivative
    int16_t strainGaugeScale;
    float rescale;  // Correction factor used if visual volume measurement different from electronic measurement
  };

enum  MainState_ {
   POWERON,
   ERROR_STATE,
   STANDBY,
   WIFI_CONNECT,
   XFER,
   TARE,
   WAIT_FOR_10ML,
   ACQUIRE,
   VOLUME_CHECK,
   COMPUTE_QMAX,
   WAIT_FOR_CAL,
   RECAL_STATE,
   POWER_OFF
};

#endif
