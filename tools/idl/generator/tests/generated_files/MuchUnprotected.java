package ru.test.docs;

import androidx.annotation.Nullable;
import com.yandex.runtime.NativeObject;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

import java.nio.ByteBuffer;

public class MuchUnprotected implements Serializable {

    /**
     * Use constructor with parameters in your code.
     * This one is for serialization only!
     */
    public MuchUnprotected() {
    }
    
    public MuchUnprotected(
            float regularField,
            boolean oneMoreRegularField,
            @Nullable Integer hiddenField) {
        nativeObject = init(
            regularField,
            oneMoreRegularField,
            hiddenField);
    
        this.regularField = regularField;
        this.regularField__is_initialized = true;
        this.oneMoreRegularField = oneMoreRegularField;
        this.oneMoreRegularField__is_initialized = true;
        this.hiddenField = hiddenField;
        this.hiddenField__is_initialized = true;
    }
    
    private native NativeObject init(
            float regularField,
            boolean oneMoreRegularField,
            Integer hiddenField);
    
    private MuchUnprotected(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    private float regularField;
    private boolean regularField__is_initialized = false;

    public synchronized float getRegularField() {
        if (!regularField__is_initialized) {
            regularField = getRegularField__Native();
            regularField__is_initialized = true;
        }
        return regularField;
    }
    private native float getRegularField__Native();

    private boolean oneMoreRegularField;
    private boolean oneMoreRegularField__is_initialized = false;

    public synchronized boolean getOneMoreRegularField() {
        if (!oneMoreRegularField__is_initialized) {
            oneMoreRegularField = getOneMoreRegularField__Native();
            oneMoreRegularField__is_initialized = true;
        }
        return oneMoreRegularField;
    }
    private native boolean getOneMoreRegularField__Native();

    /** @exclude */
    private Integer hiddenField;
    /** @exclude */
    private boolean hiddenField__is_initialized = false;

    /**
     * @exclude
     * Only this field should be excluded from docs
     *
     * Optional field, can be null.
     */
    @Nullable
    public synchronized Integer getHiddenField() {
        if (!hiddenField__is_initialized) {
            hiddenField = getHiddenField__Native();
            hiddenField__is_initialized = true;
        }
        return hiddenField;
    }
    /** @exclude */
    private native Integer getHiddenField__Native();

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

    public static String getNativeName() { return "test::docs::MuchUnprotected"; }
}
