package ru.test.docs.internal;

import ru.test.docs.SuchHidden;
import ru.test.docs.TooInternal;
import ru.test.docs.VeryPrivate;

import androidx.annotation.NonNull;
import com.yandex.runtime.NativeObject;

/** @exclude */
public class TooInternalBinding implements TooInternal {

    /**
     * Holds native interface memory address.
     */
    private final NativeObject nativeObject;

    /**
     * Invoked only from native code.
     */
    protected TooInternalBinding(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    @Override
    public native boolean regularMethod(
        @NonNull SuchHidden muchPrivate,
        @NonNull VeryPrivate soInternal);
}
