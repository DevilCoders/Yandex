package ru.test.docs;

import androidx.annotation.NonNull;
import com.yandex.runtime.NativeObject;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

import java.nio.ByteBuffer;

public class WithInternalEnum implements Serializable {

    public static enum InternalEnum {
        A,
        B,
        C
    }

    /**
     * Use constructor with parameters in your code.
     * This one is for serialization only!
     */
    public WithInternalEnum() {
    }
    
    public WithInternalEnum(
            @NonNull InternalEnum e) {
        if (e == null) {
            throw new IllegalArgumentException(
                "Required field \"e\" cannot be null");
        }
        nativeObject = init(
            e);
    
        this.e = e;
        this.e__is_initialized = true;
    }
    
    private native NativeObject init(
            InternalEnum e);
    
    private WithInternalEnum(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    private InternalEnum e;
    private boolean e__is_initialized = false;

    @NonNull
    public synchronized InternalEnum getE() {
        if (!e__is_initialized) {
            e = getE__Native();
            e__is_initialized = true;
        }
        return e;
    }
    private native InternalEnum getE__Native();

    @Override
    public void serialize(Archive archive) {
        if (archive.isReader()) {
            ByteBuffer buffer = null;
            buffer = archive.add(buffer);
            nativeObject = loadNative(buffer);
        } else {
            ByteBuffer buffer = ByteBuffer.allocateDirect(10);
            archive.add(saveNative());
        }
    }
    
    private static native NativeObject loadNative(ByteBuffer buffer);
    private native ByteBuffer saveNative();

    private NativeObject nativeObject;

    public static String getNativeName() { return "test::docs::WithInternalEnum"; }
}
