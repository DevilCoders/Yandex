package ru.test.docs;

import androidx.annotation.Nullable;
import com.yandex.runtime.NativeObject;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

import java.nio.ByteBuffer;

public class VeryOpen implements Serializable {

    /**
     * Use constructor with parameters in your code.
     * This one is for serialization only!
     */
    public VeryOpen() {
    }
    
    public VeryOpen(
            int regularField,
            @Nullable Boolean hiddenSwitch) {
        nativeObject = init(
            regularField,
            hiddenSwitch);
    
        this.regularField = regularField;
        this.regularField__is_initialized = true;
        this.hiddenSwitch = hiddenSwitch;
        this.hiddenSwitch__is_initialized = true;
    }
    
    private native NativeObject init(
            int regularField,
            Boolean hiddenSwitch);
    
    private VeryOpen(NativeObject nativeObject) {
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

    /** @exclude */
    private Boolean hiddenSwitch;
    /** @exclude */
    private boolean hiddenSwitch__is_initialized = false;

    /**
     * @exclude
     * Only this field should be excluded from docs
     *
     * Optional field, can be null.
     */
    @Nullable
    public synchronized Boolean getHiddenSwitch() {
        if (!hiddenSwitch__is_initialized) {
            hiddenSwitch = getHiddenSwitch__Native();
            hiddenSwitch__is_initialized = true;
        }
        return hiddenSwitch;
    }
    /** @exclude */
    private native Boolean getHiddenSwitch__Native();

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

    public static String getNativeName() { return "test::docs::VeryOpen"; }
}
