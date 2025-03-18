package ru.test.docs;

import androidx.annotation.NonNull;

public interface TooExternal {

    public boolean regularMethod(
        @NonNull MuchUnprotected structure,
        @NonNull SoDeclassified callback);

    /**
     * @exclude
     * Only this method should be excluded from docs
     */
    public void hiddenMethod(
        @NonNull SuchHidden structure,
        @NonNull SoSecret callback);
}
