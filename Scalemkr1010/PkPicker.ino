#include "PkPicker.h"
//
//   Copyright 2022, Jerry Smith
//


void CPkPicker::qualify(float flow,  int16_t ndx, SummaryData_ & summary){
  char cbuf[40];

  if (flow > summary.Qmax) {
    if (flow > pkCandidate.flow) {
      pkCandidate.flow = flow;
      pkCandidate.index = ndx;
	    snprintf(cbuf,40,"New candidate: %d ",pkCandidate.index);
//	    Serial.print(cbuf);Serial.print(pkCandidate.flow); Serial.print("\n");
    }
  }
  if (pkCandidate.flow > 0) { // If there is a peak candidate
	  if ( flow <= 1.0 ) {
        if ((ndx - pkCandidate.index) < 10 )  { // Dropped below 1.0mL/sec within 1 sec - disqualify peak
          pkCandidate.flow = 0;
	      pkCandidate.index = 0;
//+		  Serial.print("Pk disqualified\n");
        }
      }
      else if ( (ndx - pkCandidate.index) > 10 ) { // Didn't drop below 1.0mL/sec within 1 sec - declare a peak
	    summary.Qmax = pkCandidate.flow;
	    summary.maxNdx = pkCandidate.index;
		pkCandidate.flow = 0;               // Start looking for a new candidate
		pkCandidate.index = 0;
      }
  }  
}
