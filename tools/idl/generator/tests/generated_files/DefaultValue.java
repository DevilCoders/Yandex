package ru.test.docs;

import androidx.annotation.NonNull;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

public class DefaultValue implements Serializable {

    public DefaultValue(
            @NonNull String filled) {
        if (filled == null) {
            throw new IllegalArgumentException(
                "Required field \"filled\" cannot be null");
        }
        this.filled = filled;
    }
    
    /**
     * Use constructor with parameters in your code.
     * This one is for bindings only!
     */
    public DefaultValue() {
    }

    private String filled = "default value";

    @NonNull
    public String getFilled() {
        return filled;
    }

    @Override
    public void serialize(Archive archive) {
        filled = archive.add(filled, false);
    }
}
