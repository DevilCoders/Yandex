package ru.yandex.se.logsng.tool.adapters.dsl.base;

/**
 * Created by astelmak on 06.06.16.
 */
public class Util {
    public enum CASE {
        UPPER,
        LOWER,
        NONE
    }

    public static StringBuilder generateName(String xsdName, CASE charOps) {
        final int len = xsdName.length();
        StringBuilder b = new StringBuilder(len);

        for (int i = 0; i < len; i++) {
            char c = xsdName.charAt(i);
            if (c == '_')
                charOps = CASE.UPPER;
            else {
                switch (charOps) {
                    case UPPER:
                        c = Character.toUpperCase(c);
                        break;
                    case LOWER:
                        c = Character.toLowerCase(c);
                        break;
                }
                charOps = CASE.NONE;
                b.append(c);
            }
        }

        return b;
    }

    public static StringBuilder appendSpaces(StringBuilder b, int num) {
        for (int i = 0; i < num; i++)
            b.append(' ');

        return b;
    }

    private static final String TAB = "    ";
    public static StringBuilder appendTabs(StringBuilder b, int num) {
        for (int i = 0; i < num; i++)
            b.append(TAB);

        return b;
    }

    public static int getTabsOnTail(StringBuilder b) {
        int n = 0;
        for (int i = b.length() - 1; i >= 0 && b.charAt(i) == ' '; i--)
            n++;

        return n / TAB.length() + ( n % TAB.length() == 0 ? 0 : 1);
    }
}
