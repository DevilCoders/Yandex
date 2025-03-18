package ru.yandex.se.logsng.tool;

public class LogUtils {
  private static boolean hasErrors = false;

  public static void error(final String msg){
    System.err.println("ERROR:" + msg);
    hasErrors = true;
  }

  public static boolean hasErrors() {
    return hasErrors;
  }

  public static void warn(final String msg) {
    System.err.println("WARN: " + msg);
  }

  public static void info(final String msg) {
    System.err.println("INFO: " + msg);
  }
}
