package ru.test.docs;

import com.yandex.runtime.NativeObject;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

import java.nio.ByteBuffer;

public class Response implements Serializable {

    /**
     * Use constructor with parameters in your code.
     * This one is for serialization only!
     */
    public Response() {
    }
    
    public Response(
            int i,
            float f) {
        nativeObject = init(
            i,
            f);
    
        this.i = i;
        this.i__is_initialized = true;
        this.f = f;
        this.f__is_initialized = true;
    }
    
    private native NativeObject init(
            int i,
            float f);
    
    private Response(NativeObject nativeObject) {
        this.nativeObject = nativeObject;
    }

    private int i;
    private boolean i__is_initialized = false;

    public synchronized int getI() {
        if (!i__is_initialized) {
            i = getI__Native();
            i__is_initialized = true;
        }
        return i;
    }
    private native int getI__Native();

    private float f;
    private boolean f__is_initialized = false;

    public synchronized float getF() {
        if (!f__is_initialized) {
            f = getF__Native();
            f__is_initialized = true;
        }
        return f;
    }
    private native float getF__Native();

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

    public static String getNativeName() { return "test::docs::Response"; }
}
