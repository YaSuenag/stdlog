package jp.dip.ysfactory.stdlog.client;

import java.net.*;
import java.io.*;


public class Main{

  private static void printUsage(){
    System.out.println("Usage:");
    System.out.println("  java -jar stdlog-client.jar <port> <command>");
  }

  public static void main(String[] args) throws IOException{

    if(args.length != 2){
      printUsage();
      System.exit(1);
    }

    int port = Integer.parseInt(args[0]);
    byte[] command = args[1].getBytes();

    try(Socket sock = new Socket(InetAddress.getLoopbackAddress(), port)){
      sock.getOutputStream().write(command);

      try(BufferedReader reader = new BufferedReader(
                                    new InputStreamReader(
                                      sock.getInputStream()))){
        reader.lines()
              .forEachOrdered(System.out::println);
      }

    }

  }

}
