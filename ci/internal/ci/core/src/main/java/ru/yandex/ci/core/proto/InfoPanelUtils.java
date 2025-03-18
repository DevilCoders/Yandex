package ru.yandex.ci.core.proto;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

import ru.yandex.ci.common.info.InfoPanelOuterClass.InfoEntity;

public class InfoPanelUtils {

    // точку вместо запятой как десятичный разделитель
    private static final DecimalFormatSymbols DECIMAL_FORMAT_SYMBOLS = DecimalFormatSymbols.getInstance(Locale.UK);

    private InfoPanelUtils() {
    }

    public static InfoEntity createAttribute(String key, Number value, boolean hasCopyButton) {
        var valueString = value.toString();
        return createAttribute(key, valueString, hasCopyButton);
    }

    public static InfoEntity createAttribute(String key, String value, boolean hasCopyButton) {
        return createAttribute(key, value, "", hasCopyButton);
    }

    public static InfoEntity createAttribute(String key, String value, String link, boolean hasCopyButton) {
        return InfoEntity.newBuilder()
                .setKey(key)
                .setValue(value)
                .setLink(link)
                .setHasCopyButton(hasCopyButton)
                .build();
    }

    public static String formatSeconds(double seconds) {
        var df = new DecimalFormat("0.##", DECIMAL_FORMAT_SYMBOLS);
        double s = seconds % 60;
        double h = seconds / 60;
        double m = h % 60;
        h = h / 60;

        if (h >= 1) {
            return plural(df.format(h), "hour");
        } else if (m >= 1) {
            return plural(df.format(m), "minute");
        } else {
            return plural(df.format(s), "second");
        }
    }

    private static String plural(String formattedValue, String word) {
        if (formattedValue.equals("1") || formattedValue.equals("-1")) {
            return formattedValue + " " + word;
        } else {
            return formattedValue + " " + word + "s";
        }
    }
}
