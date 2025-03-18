package ru.yandex.se.logsng.tool;
import java.util.*;
/**
 * Created by gamgy on 20/09/16.
 */


public class PythonName {
  private static final Set<String> PYTHON_KEYWORDS = new HashSet<>(Arrays.asList("and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else", "except", "exec", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try", "while", "with", "yield"));

  public static String getPythonName(String name) {
  if (PYTHON_KEYWORDS.contains(name)){
        return name + "_pn";
  } else {
        return name;
  }
  }
}
