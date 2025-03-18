package ru.yandex.ci.client.abc;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Value
public class AbcServiceInfo {

    int id;
    String slug;
    LocalizedName name;
    LocalizedName description;
    String path;

    public String getSlug() {
        return slug.toLowerCase();
    }

    @Persisted
    @Value
    public static class LocalizedName {
        String ru;
        String en;

        public LocalizedName toLower() {
            return new LocalizedName(ru.toLowerCase(), en.toLowerCase());
        }
    }

}
