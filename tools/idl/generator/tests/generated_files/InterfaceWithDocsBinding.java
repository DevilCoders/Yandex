package ru.test.docs.internal;

import ru.test.docs.InterfaceWithDocs;
import ru.test.docs.JavaInterface;
import ru.test.docs.LambdaListener;
import ru.test.docs.Struct;

import androidx.annotation.NonNull;
import com.yandex.runtime.NativeObject;

public class InterfaceWithDocsBinding implements InterfaceWithDocs {

    /**
     * Holds native interface memory address.
     */
    private final NativeObject nativeObject;

    /**
     * Invoked only from native code.
     */
    protected InterfaceWithDocsBinding(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    @Override
    public native boolean methodWithDocs(
        @NonNull JavaInterface i,
        @NonNull Struct s,
        @NonNull LambdaListener l);
}
