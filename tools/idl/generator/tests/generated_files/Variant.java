package ru.test.docs;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

public class Variant {

    private Integer i;
    private Float f;

    @NonNull
    public static Variant fromI(int i) {
        Variant variant = new Variant();
        variant.i = i;
        return variant;
    }

    @NonNull
    public static Variant fromF(float f) {
        Variant variant = new Variant();
        variant.f = f;
        return variant;
    }

    @Nullable
    public Integer getI() {
        return i;
    }

    @Nullable
    public Float getF() {
        return f;
    }
}
