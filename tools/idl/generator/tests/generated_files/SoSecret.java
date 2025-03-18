package ru.test.docs;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;

/**
 * @exclude
 * This listener should be excluded from documentation.
 */
public interface SoSecret {
    @UiThread
    public void firstCallback(
        @NonNull VeryPrivate muchClassified);
}
