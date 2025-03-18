package ru.yandex.se.logsng.tool;

import java.util.EnumMap;

/**
 * Created by minamoto on 09/06/15.
 */
public class BuiltinTypeResolver {
  private static EnumMap<Lang, EnumMap<TYPES, String>> TYPES_DECLARATIONS = new EnumMap<Lang, EnumMap<TYPES, String>>(Lang.class);
  private static EnumMap<Lang, EnumMap<TYPES, String>> TYPES_INVOCATIONS = new EnumMap<Lang, EnumMap<TYPES, String>>(Lang.class);
  static {
    EnumMap<TYPES, String> javaDeclarations = new EnumMap<TYPES, String>(TYPES.class);
    javaDeclarations.put(TYPES.INET_ADDRESS, "java.net.InetAddress");
    javaDeclarations.put(TYPES.URL, "java.net.URL");
    javaDeclarations.put(TYPES.URI, "java.net.URI");
    TYPES_DECLARATIONS.put(Lang.JAVA, javaDeclarations);

    EnumMap<TYPES, String> javaInvocation = new EnumMap<TYPES, String>(TYPES.class);
    javaInvocation.put(TYPES.INET_ADDRESS, "java.net.InetAddress.getByName");
    javaInvocation.put(TYPES.URL, "new java.net.URL");
    javaInvocation.put(TYPES.URI, "new java.net.URI");
    TYPES_INVOCATIONS.put(Lang.JAVA, javaInvocation);

    EnumMap<TYPES, String> objcDeclarations = new EnumMap<TYPES, String>(TYPES.class);
    objcDeclarations.put(TYPES.INET_ADDRESS, "NSString*");
    objcDeclarations.put(TYPES.URL, "NSURL*");
    objcDeclarations.put(TYPES.URI, "NSURL*");
    TYPES_DECLARATIONS.put(Lang.OBJC, objcDeclarations);

    EnumMap<TYPES, String> objcInvocation = new EnumMap<TYPES, String>(TYPES.class);
    objcInvocation.put(TYPES.INET_ADDRESS, "NSString stringWithString:");
    objcInvocation.put(TYPES.URL, "NSURL URLWithString:");
    objcInvocation.put(TYPES.URI, "NSURL URLWithString:");
    TYPES_INVOCATIONS.put(Lang.OBJC, objcInvocation);

    EnumMap<TYPES, String> pythonDeclarations = new EnumMap<TYPES, String>(TYPES.class);
    pythonDeclarations.put(TYPES.INET_ADDRESS, "host");
    pythonDeclarations.put(TYPES.URL, "url");
    pythonDeclarations.put(TYPES.URI, "url");
    TYPES_DECLARATIONS.put(Lang.PYTHON, pythonDeclarations);

    EnumMap<TYPES, String> pythonInvocation = new EnumMap<TYPES, String>(TYPES.class);
    pythonInvocation.put(TYPES.INET_ADDRESS, "socket.gethostbyname_ex");
    pythonInvocation.put(TYPES.URL, "urlparse");
    pythonInvocation.put(TYPES.URI, "urlparse");
    TYPES_INVOCATIONS.put(Lang.PYTHON, pythonInvocation);
  }

  public static String resolveType(String language, String type) {
    return TYPES_DECLARATIONS.get(Lang.valueOf(language)).get(TYPES.valueOf(type));
  }

  public static String resolveInvocation(String language, String type, String arg) {
    final Lang lang = Lang.valueOf(language);
    final StringBuilder result = new StringBuilder(TYPES_INVOCATIONS.get(lang).get(TYPES.valueOf(type)));
    switch (lang) {
      case OBJC:
        break;
      case CPP:
      case JAVA:
      default:
        result.append('(').append(arg).append(')');
        break;
    }
    return result.toString();
  }

  public enum TYPES{
    INET_ADDRESS,
    URI,
    URL
  }
}
