package ru.test.docs;

import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

public class DefaultTimeintervalValue implements Serializable {

    public DefaultTimeintervalValue(
            long empty,
            long filled) {
        this.empty = empty;
        this.filled = filled;
    }
    
    /**
     * Use constructor with parameters in your code.
     * This one is for bindings only!
     */
    public DefaultTimeintervalValue() {
    }

    private long empty;

    public long getEmpty() {
        return empty;
    }

    private long filled = 300;

    public long getFilled() {
        return filled;
    }

    @Override
    public void serialize(Archive archive) {
        empty = archive.add(empty);
        filled = archive.add(filled);
    }
}
