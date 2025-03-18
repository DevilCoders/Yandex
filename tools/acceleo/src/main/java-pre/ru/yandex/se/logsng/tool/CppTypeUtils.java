package ru.yandex.se.logsng.tool;

import java.util.HashMap;

/**
 * Created by inikifor on 09.05.15.
 */
public class CppTypeUtils {
  private static final HashMap<String, String> EMF2CPP_TYPES = new HashMap<String, String>();

  static {
    EMF2CPP_TYPES.put("Boolean", "bool");
    EMF2CPP_TYPES.put("int", "int");
    EMF2CPP_TYPES.put("Integer", "int");
    EMF2CPP_TYPES.put("UnsignedInt", "unsigned int");
    EMF2CPP_TYPES.put("long", "long");
    EMF2CPP_TYPES.put("Long", "long");
    EMF2CPP_TYPES.put("UnsignedLong", "unsigned long");
    EMF2CPP_TYPES.put("Float", "float");
    EMF2CPP_TYPES.put("Double", "double");
  }

  public static String getCppTypeName(final String emfType) {
    return EMF2CPP_TYPES.get(emfType);
  }
}
