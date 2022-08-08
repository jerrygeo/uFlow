// Outlier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//  Copyright 2022, Jerry Smith
//

/* For test in Visual Studio
#include <iostream>

using std::cout;
using std::cin;
using std::endl;
*/

#include "Outlier.h"


OutlierTbl_ outlierTbl[20];
int16_t ndxOutlierTbl;



void COutlier::removeOutliers(int16_t * msrBuf, int16_t length) {
/*!
*  Function removeOutliers
*  Inputs:   msrBuf - buffer of scale data at 100msec intervals stored in units of 0.1mL
*            length - length of msrBuf
*  Processing:  On rare occasions, the HX711 system on the MKR1010 system  reads data incorrectly.
*                 I don't know why this happens - I assume it is some sort of hardware issue since
*                   there aren't other processes active (as far as I know) and the acquisition code
*                   is pretty simple. If this assumption is correct, there is not much that can be done
*                   except to identify outlier points and linearly interpolate between good data points. Since 
*                   flow is expected to be under 100mL/s, the maximum change should be less than 100 units. So
*                   a point is declared an outlier if it is more than 100 above the previous point, or less than
*                   20 below (2mL) the previous point.
*                 As outliers are identified, they are entered into the table outlierTbl so that both the raw
*                   data and the cleaned data can be saved on the uSD card
*
***************************************/
  int i;
  int16_t ndxLastGoodData; // index in msrBuf of good data point preceeding one or more bad data points
  int16_t lastGoodData;
  int16_t nConsecutiveOutliers = 0;
  int j;


  lastGoodData = msrBuf[0]; // Assume first point is OK
  ndxLastGoodData = 0;
  for (i = 1; i < length; i++) {
      if ( (msrBuf[i] > lastGoodData + 100 * (nConsecutiveOutliers + 1)) ||
          (msrBuf[i] < lastGoodData - 20 * (nConsecutiveOutliers + 1)) ) { // Data is outlier 
              nConsecutiveOutliers++;
 //             cout << "Index " << i << " is an outlier" << std::endl;
              outlierTbl[ndxOutlierTbl].ndx = i;   // Put outlier into table
              outlierTbl[ndxOutlierTbl++].rawData = msrBuf[i];
      }
      else {  // Data is not outlier
          if (nConsecutiveOutliers > 0) { // Need to interpolate earlier point(s)
              for (int n = 1; n <= nConsecutiveOutliers; n++) {
                  msrBuf[ndxLastGoodData + n] = lastGoodData + n * (msrBuf[i] - lastGoodData) / (nConsecutiveOutliers + 1);
              }
          }
          lastGoodData = msrBuf[i];
          ndxLastGoodData = i;
          nConsecutiveOutliers = 0;
      }
      j = i;
   }
//   std::cout << "outlierTbl:" << std::endl;
//   for (i = 0; i < ndxOutlierTbl; i++) {
//      std::cout << outlierTbl[i].ndx << "," << outlierTbl[i].rawData << std::endl;
//  }
}
int16_t COutlier::originalData(int16_t* msrBuf, int16_t index) {
    /*!
    *  Function originalData
    *  Inputs:   msrBuf - buffer of scale data at 100msec intervals stored in units of 0.1mL
    *            index - original element of msrBuf to retrieve
    *  Processing:  Return the original value of msrBuf at index ndx, taking it from msrBuf
    *                 if ndx is not in outlierTbl
    *
    ***************************************/
    int i;
    if (ndxOutlierTbl > 0) {
      for (i = 0; i < ndxOutlierTbl; i++) {
        if (outlierTbl[i].ndx == index) return outlierTbl[i].rawData;
      }
    }
    return msrBuf[index];
}

/* Test in Visual Studio
int main()
{
    int16_t testTbl[20];
    int i;
    COutlier outlier;
    int flag;

    for (i = 0; i < 20; i++) testTbl[i] = 2 * i;
    testTbl[3] = 500;
    testTbl[8] = -40;
    testTbl[9] = 1800;
    testTbl[10] = 1200;
    std::cout << "Test of Outlier\n";

    outlier.removeOutliers(testTbl, sizeof(testTbl) / sizeof(testTbl[0]));
    for (i = 0; i < 20; i++) {
        std::cout << i << " " << testTbl[i] << " " << outlier.originalData(testTbl, i) << std::endl;
    }
    flag = cin.get();
}
*/
