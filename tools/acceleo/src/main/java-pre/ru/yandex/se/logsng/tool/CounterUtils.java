package ru.yandex.se.logsng.tool;

import gnu.trove.TCollections;
import gnu.trove.map.TObjectIntMap;
import gnu.trove.map.hash.TObjectIntHashMap;

public class CounterUtils {
  private static TObjectIntHashMap<String> counters = new TObjectIntHashMap<String>(); /* give up to shut up Idea and friend it with maven  */

  public static int inc(final String counterName) {
    final int value = counters.get(counterName);
    if (value == counters.getNoEntryValue()) {
      counters.put(counterName, 1);
      return 1;
    } else {
      final int newValue = value + 1;
      counters.put(counterName, newValue);
      return newValue;
    }
  }

  public static int dec(final String counterName) {
    final int value = counters.get(counterName);
    if (value == 0 || value == counters.getNoEntryValue())
      throw new IllegalStateException(counterName + " desn't exists or already 0");

    final int newValue = value - 1;
    counters.put(counterName, newValue);
    return newValue;
  }

  public static int reset(final String counterName) {
    counters.remove(counterName);
    return 0;
  }

  public int set(final String counterName, final Integer value) {
    counters.put(counterName, value);
    return value;
  }

  public static int value(final String counterName) {
    return counters.get(counterName);
  }

  public static int updateIfGreater(final String counterName, final int value){
    final int oldValue = counters.get(counterName);
    if (oldValue == counters.getNoEntryValue() || value > oldValue) {
      counters.put(counterName, value);
      return value;
    }
    return oldValue;
  }

  public static TObjectIntMap<String> getCounters() {
    return TCollections.unmodifiableMap(counters);
  }
}
