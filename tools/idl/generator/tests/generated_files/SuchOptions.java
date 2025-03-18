package ru.test.docs;

import androidx.annotation.NonNull;

/**
 * @exclude
 * This struct should be excluded from documentation.
 */
public final class SuchOptions {

    public SuchOptions(
            @NonNull TooInternal interfaceField) {
        this.interfaceField = interfaceField;
    }
    
    /**
     * Use constructor with parameters in your code.
     * This one is for bindings only!
     */
    public SuchOptions() {
    }

    private TooInternal interfaceField;

    @NonNull
    public TooInternal getInterfaceField() {
        return interfaceField;
    }

    /**
     * See {@link #getInterfaceField()}.
     */
    public SuchOptions setInterfaceField(@NonNull TooInternal interfaceField) {
        this.interfaceField = interfaceField;
        return this;
    }
}
