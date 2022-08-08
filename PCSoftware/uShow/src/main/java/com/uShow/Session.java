package com.uShow;
/*
*           Copyright 2022, Jerry Smith
*/
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeries;

import java.time.LocalDate;
import java.util.ArrayList;

public class Session {
    private LocalDate date;
    private int hour;
    private int minute;
    private int second;
    private float qMax;
    private float tEnd;
    private float tPeak;
    private XYSeries rateSamples;

    public Session(LocalDate date,
                   int hr, int min, int sec,
                   float qMax, float tEnd, float tPeak, XYSeries samples) {
        this.date = date;
        this.hour = hr;
        this.minute = min;
        this.second = sec;
        this.qMax = qMax;
        this.tEnd = tEnd;
        this.tPeak = tPeak;
        this.rateSamples = samples;
    }
    public LocalDate getDate() {
        return date;
    }

    public int getHour() {
        return hour;
    }

    public int getMinute() {
        return minute;
    }

    public int getSecond() {
        return second;
    }

    public float getqMax() {
        return qMax;
    }

    public float gettEnd() {
        return tEnd;
    }

    public float gettPeak() {
        return tPeak;
    }

    public XYSeries getRateSamples() {
        return rateSamples;
    }

//    public void setRateSamples(ArrayList<float> rateSamples) {
//       this.rateSamples = rateSamples;
//    }
}
