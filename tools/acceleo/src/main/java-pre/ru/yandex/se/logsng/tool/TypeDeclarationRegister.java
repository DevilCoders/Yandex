package ru.yandex.se.logsng.tool;

import java.util.EnumMap;
import java.util.HashSet;
import java.util.Set;

/**
 *
 * Created by minamoto on 11/06/15.
 */
public class TypeDeclarationRegister {
  private final static EnumMap<Lang, Set<String>> DECLARED_SIMPLES_PER_LANG = new EnumMap<Lang, Set<String>>(Lang.class);
  static {
    DECLARED_SIMPLES_PER_LANG.put(Lang.CPP, new HashSet<String>());
    DECLARED_SIMPLES_PER_LANG.put(Lang.OBJC, new HashSet<String>());
    DECLARED_SIMPLES_PER_LANG.put(Lang.PYTHON, new HashSet<String>());
  }

  public static boolean iSdeclaredInSimple(String language, String typeName) {
    return DECLARED_SIMPLES_PER_LANG.get(Lang.valueOf(language)).contains(typeName);
  }

  public static boolean declareInSimple(String language, String typename) {
    DECLARED_SIMPLES_PER_LANG.get(Lang.valueOf(language)).add(typename);
    return true;
  }

  public static int numberOfDeclaredInSimple(String language){
    return DECLARED_SIMPLES_PER_LANG.get(Lang.valueOf(language)).size();
  }
}
