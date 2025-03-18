package ru.yandex.se.logsng.tool;

import java.util.HashMap;

public class JavaTypeUtils {
  private static final HashMap<String, String> EMF2JAVA_TYPES = new HashMap<String, String>();

  static {
    EMF2JAVA_TYPES.put("Boolean", "boolean");
    EMF2JAVA_TYPES.put("int", "int");
    EMF2JAVA_TYPES.put("Integer", "int");
    EMF2JAVA_TYPES.put("UnsignedShort", "int");
    EMF2JAVA_TYPES.put("UnsignedInt", "int");
    EMF2JAVA_TYPES.put("UnsignedInteger", "int");
    EMF2JAVA_TYPES.put("PositiveInteger", "int");
    EMF2JAVA_TYPES.put("long", "long");
    EMF2JAVA_TYPES.put("Long", "long");
    EMF2JAVA_TYPES.put("UnsignedLong", "long");
    EMF2JAVA_TYPES.put("Float", "float");
    EMF2JAVA_TYPES.put("Double", "double");
  }

  public static String getJavaTypeName(final String emfType) {
    return EMF2JAVA_TYPES.get(emfType);
  }
}
