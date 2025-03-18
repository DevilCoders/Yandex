package ru.yandex.se.logsng.tool;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

/**
 * Created by inikifor on 17.05.15.
 */
public final class CppEnumUtils {

  private static Set<String> constants = new HashSet<>();
  private static String one = null;

  public static void addEnumConstant(String constant) {
    if (one == null) {
      one = constant;
    } else {
      constants.add(constant);
    }
  }

  public Set<String> getAllConstantsButOne() {
    return Collections.unmodifiableSet(constants);
  }

  public String getOneConstant() {
    return one;
  }

}
