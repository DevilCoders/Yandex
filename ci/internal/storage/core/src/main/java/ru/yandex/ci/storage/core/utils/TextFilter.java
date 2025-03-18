package ru.yandex.ci.storage.core.utils;

import yandex.cloud.repository.kikimr.yql.YqlPredicate;

public class TextFilter {
    private TextFilter() {

    }

    public static YqlPredicate get(String field, String value) {
        if (value.endsWith("*")) {
            return YqlPredicate.like(field, value.substring(0, value.length() - 1) + "%");
        } else {
            return YqlPredicate.eq(field, value);
        }
    }
}
