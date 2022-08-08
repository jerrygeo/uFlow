/*! uShow - A program to display flow data collected from the Uroflow system.
*           The file structure is composed of records for each session. The records
*           are all ASCII data. The first line of each session is a summary of the measurements:
*              "DATE", Date (in format mm/dd/yyyy),
*              "TIME", Time (in format hh,mm,ss),
*              "Qmax", Max flow rate in mL/sec, floating pt, after adjusting total volume
*              "Tpeak", time in seconds from 10mL level to peak flow, floating pt.
*              "Ttotal", time in seconds from 10mL level to flow less than 2mL/sec, floating pt.
*              "Volscale", volume in mL as determined by the scale, before adjusting for the value observed on the graduated beaker
*              “Volvisual”, volume observed on the graduated beaker
*              “Calib”, scale calibration constant used
*           Next there are a variable number of lines of each volume measurement. There are no labels for
*              these numbers:
*                     Time in seconds
*                     Corrected volume (with outliers removed) in mL
*                     Volume in mL
*                     Flow in mL/sec
*           The sessions are read in groups of 5, and the flow data for these 5 sessions is plotted on one graph.
*              As the sessions are read, histograms of Qmax (maximum flow rate) are compiled for each week.
*              The final plot shows all of these weekly histograms together on one plot.
*
*           Copyright 2022, Jerry Smith
*/


package com.uShow;

import java.awt.*;
import java.io.File;
import java.util.List;
import java.util.ArrayList;
import java.awt.geom.Ellipse2D;
import java.io.FileReader;
import com.opencsv.CSVReader;


import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartUtilities;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.renderer.AbstractRenderer;
import org.jfree.data.xy.XYSeries;

import org.jfree.chart.ChartPanel;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeriesCollection;
import org.jfree.ui.ApplicationFrame;
import org.jfree.ui.RectangleInsets;

import javax.swing.*;

import static java.lang.StrictMath.abs;

import java.time.LocalDate;
import  java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

public class FlowShow extends ApplicationFrame {

    private static class TitleDataset {
        public final XYDataset dataset;
        public final String title;
        public final String yAxisLabel;
        public final String xAxisLabel;
        public TitleDataset(XYDataset dataset, String title, String xAxisLabel, String yAxisLabel) {
            this.dataset = dataset;
            this.title = title;
            this.xAxisLabel = xAxisLabel;
            this.yAxisLabel = yAxisLabel;
        }
    }

    /**
     * Program to display flow data and a histogram of maximum flow values acquired from uflow system.
     *
     */
    public FlowShow(List<TitleDataset> datasets) {
        super("FlowShow");
        JPanel mainPanel = new JPanel();
        LayoutManager layout = new GridLayout(3, 3);
        mainPanel.setLayout(layout);
        for (TitleDataset dataset : datasets) {
            JFreeChart chart = createChart(dataset.title, dataset.xAxisLabel, dataset.yAxisLabel, dataset.dataset);
            ChartPanel chartPanel = new ChartPanel(chart);
            chartPanel.setPreferredSize(new java.awt.Dimension(500, 270));
            chartPanel.setMouseZoomable(true, false);
            mainPanel.add(chartPanel);
        }
        setContentPane(mainPanel);

    }



    /**
     * Creates a chart.
     *
     * @param dataset a dataset.
     *
     * @return A chart.
     */
    private static JFreeChart createChart(String title, String xaxis, String yaxis, XYDataset dataset) {
        JFreeChart chart = ChartFactory.createXYLineChart(
                title,
                xaxis,
                yaxis,
                dataset,
                PlotOrientation.VERTICAL,
                true,
                true,
                false
        );
        ChartUtilities.applyCurrentTheme(chart);

        XYPlot plot = (XYPlot) chart.getPlot();
        AbstractRenderer r1 = (AbstractRenderer) plot.getRenderer(0);
        if (title == "Flow") {
            r1.setSeriesPaint(0, Color.red);
            r1.setSeriesPaint(1, Color.red);
            r1.setSeriesPaint(2, Color.green);
            r1.setSeriesPaint(3, Color.green);
            r1.setSeriesPaint(4, Color.blue);
            r1.setSeriesPaint(5, Color.blue);
            r1.setSeriesPaint(6, Color.yellow);
            r1.setSeriesPaint(7, Color.yellow);
            r1.setSeriesPaint(8, Color.magenta);
            r1.setSeriesPaint(9, Color.magenta);
        }
        else {
            r1.setSeriesPaint(0, Color.red);
            r1.setSeriesPaint(1, Color.green);
            r1.setSeriesPaint(2, Color.blue);
            r1.setSeriesPaint(3, Color.yellow);
            r1.setSeriesPaint(4, Color.magenta);

        }



        NumberAxis yAxis = (NumberAxis) plot.getRangeAxis();
        yAxis.setAutoRangeIncludesZero(false);

        Shape shape = new Ellipse2D.Double(-4.0, -4.0, 8.0, 8.0);
        XYLineAndShapeRenderer renderer1 = (XYLineAndShapeRenderer)
                plot.getRenderer();
        renderer1.setBaseShapesVisible(true);
        renderer1.setSeriesShape(0, shape);
        renderer1.setSeriesPaint(0, Color.red);
        renderer1.setSeriesFillPaint(0, Color.yellow);
        renderer1.setSeriesOutlinePaint(0, Color.gray);

        chart.setBackgroundPaint(Color.white);
        plot.setBackgroundPaint(Color.lightGray);
        plot.setDomainGridlinePaint(Color.white);
        plot.setRangeGridlinePaint(Color.white);
        plot.setAxisOffset(new RectangleInsets(5.0, 5.0, 5.0, 5.0));
        plot.setDomainCrosshairVisible(true);
        plot.setRangeCrosshairVisible(true);
        XYItemRenderer r = plot.getRenderer();
        if (r instanceof XYLineAndShapeRenderer) {
            XYLineAndShapeRenderer renderer = (XYLineAndShapeRenderer) r;
            renderer.setBaseShapesVisible(false);
            renderer.setBaseShapesFilled(true);
        }
//        DateAxis axis = (DateAxis) plot.getDomainAxis();
//        axis.setDateFormatOverride(new SimpleDateFormat("MMM-yyyy"));
        return chart;
    }



    public static void main(String[] args) {
        // write your code here
            ArrayList<TitleDataset> datasets = new ArrayList<>();
            ArrayList<Session> session = new ArrayList<Session>();
//            String csvFileName = "C:\\Users\\jerry\\IdeaProjects\\Uxfer\\U2022-07-27.csv";
            CSVReader reader = null;
            int nPlot = 0;   // Keeps track of which session to plot
            LocalDate currentHgramDate = null;
            XYSeriesCollection hPlots = new XYSeriesCollection();
            Integer[] currentHistogram = new Integer[30];
            boolean hgramEmpty;

            JFileChooser myChooser = new JFileChooser();
            int action = myChooser.showOpenDialog(null);
            if (action == JFileChooser.APPROVE_OPTION) {
                File csvFile = myChooser.getSelectedFile();
                System.out.printf("File = %s\n", csvFile.toString());

                for (int i = 0; i < currentHistogram.length; i++) currentHistogram[i] = 0;
                hgramEmpty = true;
                try {
//parsing a CSV file into CSVReader class constructor
                    reader = new CSVReader(new FileReader(csvFile));
//reads one line at a time
                } catch (Exception e) {
                    e.printStackTrace();
                }
                int nSessions = 5;
                while (nSessions == 5) {
                    nSessions = Gimme5(reader, session);
                    System.out.printf("Got %d sessions\n", nSessions);
                    XYSeriesCollection flowPlots = new XYSeriesCollection();
                    int nTmax = 0;
                    for (int n = 0; n < nSessions; n++) {
                        flowPlots.addSeries(session.get(nPlot).getRateSamples());
                        XYSeries marker = new XYSeries("-" + Integer.toString(n));  // Add vertical line indicating maximum flow
                        marker.add(session.get(nPlot).gettPeak() + 1.9, 0.);
                        marker.add(session.get(nPlot).gettPeak() + 1.9, session.get(nPlot).getRateSamples().getDataItem((int) Math.round(session.get(nPlot).gettPeak() / .1 + 19)).getY());
                        flowPlots.addSeries(marker);
                        // Now handle the historgrams
                        if (currentHgramDate == null) {
                            currentHgramDate = session.get(nPlot).getDate();
                            int bin = (int) Math.round(session.get(nPlot).getqMax());
                            currentHistogram[bin] = currentHistogram[bin] + 1;
                            hgramEmpty = false;
                        } else if (session.get(nPlot).getDate().isAfter(currentHgramDate.plusDays(6))) {
                            // Save histogram to an XYSeries, add it to the collection hPlots
                            XYSeries hgram = new XYSeries(currentHgramDate.toString());
                            // Scale histogram so that maximimum is 100.
                            //                   float scale = (float) (100./ Collections.max(Arrays.asList(currentHistogram)));
                            float scale = (float) 1.0;
                            int totalCounts = 0;
                            for (int i = 0; i < currentHistogram.length; i++) {
                                totalCounts += currentHistogram[i];
                                hgram.add(i, currentHistogram[i] * scale);
                                currentHistogram[i] = 0;
                            }
                            System.out.printf("Total counts: %d\n", totalCounts);
                            hPlots.addSeries(hgram);
                            currentHgramDate = session.get(nPlot).getDate();
                            currentHistogram[(int) Math.round(session.get(nPlot).getqMax())]++;
                            hgramEmpty = false;
                        } else {
                            currentHistogram[(int) Math.round(session.get(nPlot).getqMax())]++;
                        }
                        nPlot++;
                    }

//              FlowShow uShow = new FlowShow("Flow","Sec", "mL/Sec", flowPlots);
//              uShow.pack();
//            RefineryUtilities.centerFrameOnScreen(ushow);
//              uShow.setVisible(true);
                    datasets.add(new TitleDataset(flowPlots, "Flow", "Sec", "mL/Sec"));
                }
                // Add left-over histogram to hPlots
                XYSeries hgram = new XYSeries(currentHgramDate.toString());
                // Scale histogram so that maximimum is 100.
                //           float scale = (float) (100./ Collections.max(Arrays.asList(currentHistogram)));
                float scale = (float) 1.0;
                for (int i = 0; i < currentHistogram.length; i++) {
                    hgram.add(i, currentHistogram[i] * scale);
                    currentHistogram[i] = 0;
                }
                hPlots.addSeries(hgram);
                datasets.add(new TitleDataset(hPlots, "Histogram", "mL/Sec", "Count"));

                FlowShow hShow = new FlowShow(datasets);
                hShow.pack();
                hShow.setVisible(true);
            }
    }


    static int Gimme5(CSVReader reader, ArrayList<Session> session) {
        int nSessions = 0;
        boolean gotHeader = false;

        String[] nextLine = null;

        boolean gotEOF = false;


        while (!gotEOF) {
            try {
                while ((nextLine = reader.readNext()) != null) {
                    if (nextLine[0].contains("Ver")) {  // If "Ver" is in first token
                        String date = nextLine[3].substring(1);
						LocalDate date1 = null;
                        try {
                            date1 = LocalDate.parse(date);
                        } catch (Exception e) {
                            System.out.printf("TIME error on line: ");System.out.println(Arrays.toString(nextLine));
                        }
                        //                   System.out.println(date1);
                        int hr = Integer.parseInt(nextLine[5]);
                        int mn = Integer.parseInt(nextLine[6]);
                        int sec = Integer.parseInt(nextLine[7]);
                        XYSeries rateSamples = new XYSeries(date+":"+nextLine[5]+nextLine[6]+nextLine[7]);
                        float qmax = Float.parseFloat(nextLine[9]);
                        float tpk = Float.parseFloat(nextLine[11]);
                        float ttl = Float.parseFloat(nextLine[13]);
                        float vscale = Float.parseFloat(nextLine[15]);
                        float vvisual = Float.parseFloat(nextLine[17]);
                        float rescale;
                        if (abs(vscale - vvisual) < 5) rescale = (float) 1.0;
                        else rescale = vvisual / vscale;
                        gotHeader = true;
                        //                   System.out.printf("%s, %d:%d:%d, %4.1f, %4.1f, %4.1f, %4.1f, %4.1f, %4.1f\n",
                        //                           date, hr, mn, sec, qmax, tpk, ttl, vscale, vvisual, rescale);

                        while ((nextLine = reader.readNext()) != null) {
                            if (isNumeric(nextLine[0])) {
                                try {
                                    rateSamples.add(Float.parseFloat(nextLine[0]),Float.parseFloat(nextLine[3]));
                                } catch (Exception e) {
                                    System.out.printf("Bad rate value: %s\n", nextLine[3]);
                                    e.printStackTrace();
                                }
                            } else {
                                gotHeader = false;
                                break;
                            }
                        }
 //                       for (int ndx=0; ndx < 5; ndx++) {
 //                           System.out.println(rateSamples.get(ndx));
 //                       }
                        nSessions++;
//                        rateSamples.clear();
                        Session thisSession = new Session(date1, hr, mn, sec, qmax, ttl, tpk, rateSamples);
                        session.add(thisSession);
                        if ((nSessions >= 5) || gotEOF) break;
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            if (null == nextLine) {
                gotEOF = true;
            }
            if (nSessions >= 5) break;
        }
        return nSessions;
    }


    public static boolean isNumeric(String strNum) {
        if (strNum == null) {
            return false;
        }
        try {
            double d = Double.parseDouble(strNum);
        } catch (NumberFormatException nfe) {
            return false;
        }
        return true;
    }
}