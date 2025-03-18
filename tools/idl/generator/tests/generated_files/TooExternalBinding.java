package ru.test.docs.internal;

import ru.test.docs.MuchUnprotected;
import ru.test.docs.SoDeclassified;
import ru.test.docs.SoSecret;
import ru.test.docs.SuchHidden;
import ru.test.docs.TooExternal;

import androidx.annotation.NonNull;
import com.yandex.runtime.NativeObject;

public class TooExternalBinding implements TooExternal {

    /**
     * Holds native interface memory address.
     */
    private final NativeObject nativeObject;

    /**
     * Invoked only from native code.
     */
    protected TooExternalBinding(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    @Override
    public native boolean regularMethod(
        @NonNull MuchUnprotected structure,
        @NonNull SoDeclassified callback);

    /** @exclude */
    @Override
    public native void hiddenMethod(
        @NonNull SuchHidden structure,
        @NonNull SoSecret callback);
}
