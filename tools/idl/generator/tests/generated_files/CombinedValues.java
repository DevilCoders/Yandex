package ru.test.docs;

import androidx.annotation.NonNull;
import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

public class CombinedValues implements Serializable {

    public CombinedValues(
            @NonNull String empty,
            @NonNull String filled) {
        if (empty == null) {
            throw new IllegalArgumentException(
                "Required field \"empty\" cannot be null");
        }
        if (filled == null) {
            throw new IllegalArgumentException(
                "Required field \"filled\" cannot be null");
        }
        this.empty = empty;
        this.filled = filled;
    }
    
    /**
     * Use constructor with parameters in your code.
     * This one is for bindings only!
     */
    public CombinedValues() {
    }

    private String empty;

    @NonNull
    public String getEmpty() {
        return empty;
    }

    private String filled = "default value";

    @NonNull
    public String getFilled() {
        return filled;
    }

    @Override
    public void serialize(Archive archive) {
        empty = archive.add(empty, false);
        filled = archive.add(filled, false);
    }
}
