package com.xfer;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.time.LocalDate;
import java.util.Scanner;
import java.io.FileWriter;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;


public class uxfer {
    public static void main(String[] args) throws Exception {
//        if (args.length != 1) {
//            System.err.println("Pass the server IP as the sole command line argument");
//            return;
//        }
//        try (var socket = new Socket(args[0], 59898)) {
        String lineOfData;
        boolean gotEndMarker;
        Scanner consoleIn = new Scanner(System.in);

        LocalDate date = LocalDate.now();
        String sDate = String.valueOf(date);
        String fileName = "U" + sDate + ".csv";
        FileWriter allData = new FileWriter(fileName);
        System.out.print("Enter wifi address: ");
        String wifiAddress = consoleIn.nextLine();

        try (Socket socket = new Socket(wifiAddress, 23)) {
            Scanner in = new Scanner(socket.getInputStream());
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            out.println("X U");
            gotEndMarker = false;
            while (!gotEndMarker) {
                //               if(scanner.hasNextLine()) {
                //                   out.println(scanner.nextLine());
                //                   System.out.println(in.nextLine());
                //               }
                if (in.hasNextLine()) {
                    lineOfData = in.nextLine();
                    if (lineOfData.contains("!")) {
                        gotEndMarker = true;
                    } else {
                        allData.write(lineOfData+"\n");
                        System.out.println(lineOfData);
                    }
                }
            }
            allData.close();
            FileWriter summaryData = new FileWriter("S" + sDate + ".csv");
            out.println("X S");
            gotEndMarker = false;
            while (!gotEndMarker) {
                //               if(scanner.hasNextLine()) {
                //                   out.println(scanner.nextLine());
                //                   System.out.println(in.nextLine());
                //               }
                if (in.hasNextLine()) {
                    lineOfData = in.nextLine();
                    if (lineOfData.contains("!")) {
                        gotEndMarker = true;
                    } else {
                        summaryData.write(lineOfData+"\n");
                        System.out.println(lineOfData);
                    }
                }
            }
            summaryData.close();


        } catch (IOException e) {
            System.out.println("Couldn't open socket - check wifi address - " + wifiAddress);
            e.printStackTrace();
        }

    }
}
