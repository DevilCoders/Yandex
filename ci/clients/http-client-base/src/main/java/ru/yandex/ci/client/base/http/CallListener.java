package ru.yandex.ci.client.base.http;

public interface CallListener {
    void onCall(long totalTimeMillis, int retCode);

    static CallListener empty() {
        return (totalTimeMillis, retCode) -> {
            // nothing to do
        };
    }
}
