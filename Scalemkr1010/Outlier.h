// Outlier.h
#ifndef OUTLIER_H
#define OUTLIER_H

class OutlierTbl_ {
public:
    int16_t ndx; // Index into msrBuf of bad data point
    int16_t rawData; // Original (bad) data in msrBuf at ndx
};

class COutlier {
public:
    void removeOutliers(int16_t* msrBuf, int16_t length);
    int16_t originalData(int16_t* msrBuf, int16_t ndx);
};



#endif
