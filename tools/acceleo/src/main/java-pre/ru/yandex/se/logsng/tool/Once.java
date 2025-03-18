package ru.yandex.se.logsng.tool;

/**
 *
 * Created by minamoto on 12/11/15.
 */
public class Once {
  boolean once = true;
  public Once() {}

  public boolean once() {
    if (once) {
      once = false;
      return true;
    }
    return once;
  }
}
