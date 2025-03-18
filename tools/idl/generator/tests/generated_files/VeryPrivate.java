package ru.test.docs;

import com.yandex.runtime.NativeObject;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

import java.nio.ByteBuffer;

/**
 * @exclude
 * This struct should be excluded from documentation.
 */
public class VeryPrivate implements Serializable {

    /**
     * Use constructor with parameters in your code.
     * This one is for serialization only!
     */
    public VeryPrivate() {
    }
    
    public VeryPrivate(
            int regularField,
            float oneMoreRegularField) {
        nativeObject = init(
            regularField,
            oneMoreRegularField);
    
        this.regularField = regularField;
        this.regularField__is_initialized = true;
        this.oneMoreRegularField = oneMoreRegularField;
        this.oneMoreRegularField__is_initialized = true;
    }
    
    private native NativeObject init(
            int regularField,
            float oneMoreRegularField);
    
    private VeryPrivate(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    private int regularField;
    private boolean regularField__is_initialized = false;

    public synchronized int getRegularField() {
        if (!regularField__is_initialized) {
            regularField = getRegularField__Native();
            regularField__is_initialized = true;
        }
        return regularField;
    }
    private native int getRegularField__Native();

    private float oneMoreRegularField;
    private boolean oneMoreRegularField__is_initialized = false;

    public synchronized float getOneMoreRegularField() {
        if (!oneMoreRegularField__is_initialized) {
            oneMoreRegularField = getOneMoreRegularField__Native();
            oneMoreRegularField__is_initialized = true;
        }
        return oneMoreRegularField;
    }
    private native float getOneMoreRegularField__Native();

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

    public static String getNativeName() { return "test::docs::VeryPrivate"; }
}
