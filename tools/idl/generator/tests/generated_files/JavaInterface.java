package ru.test.docs;

import androidx.annotation.NonNull;

public interface JavaInterface {

    public void method(
        int intValue,
        float floatValue,
        @NonNull Struct someStruct,
        @NonNull Variant andVariant);

    /**
     * Tells if this {@link JavaInterface} is valid or not. Any other method
     * (except for this one) called on an invalid {@link JavaInterface} will
     * throw {@link java.lang.RuntimeException}. An instance becomes invalid
     * only on UI thread, and only when its implementation depends on objects
     * already destroyed by now. Please refer to general docs about the
     * interface for details on its invalidation.
     */
    public boolean isValid();
}
