package ru.test.docs;

import androidx.annotation.NonNull;

/**
 * @exclude
 * This interface should be excluded from documentation.
 */
public interface TooInternal {

    public boolean regularMethod(
        @NonNull SuchHidden muchPrivate,
        @NonNull VeryPrivate soInternal);
}
