package ru.test.docs.internal;

import ru.test.docs.JavaInterface;
import ru.test.docs.Struct;
import ru.test.docs.Variant;

import androidx.annotation.NonNull;
import com.yandex.runtime.NativeObject;

public class JavaInterfaceBinding implements JavaInterface {

    /**
     * Holds native interface memory address.
     */
    private final NativeObject nativeObject;

    /**
     * Invoked only from native code.
     */
    protected JavaInterfaceBinding(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    @Override
    public native void method(
        int intValue,
        float floatValue,
        @NonNull Struct someStruct,
        @NonNull Variant andVariant);

    @Override
    public native boolean isValid();
}
