// PkPicker.h

#ifndef PKPKICKER_H
#define PKPKICKER_H

#include "MainStateTask.h"

class CPkPicker {
public:
  void qualify(float flow,  int16_t ndx, SummaryData_ & summary);
private:  
  struct pkCandidate_ {
    float flow;
    int16_t index;
  } pkCandidate;
};
#endif
