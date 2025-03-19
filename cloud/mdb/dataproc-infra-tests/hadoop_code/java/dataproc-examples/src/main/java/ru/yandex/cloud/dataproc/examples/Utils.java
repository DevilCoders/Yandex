package ru.yandex.cloud.dataproc.examples;

import com.ibm.icu.text.Transliterator;

public class Utils
{
    public static String translit(String direction, String source) {
        Transliterator transliterator = Transliterator.getInstance(direction);
        return transliterator.transliterate(source);
    }
}
