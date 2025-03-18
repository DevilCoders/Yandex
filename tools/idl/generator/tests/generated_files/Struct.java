package ru.test.docs;

import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

public class Struct implements Serializable {

    public Struct(
            int i) {
        this.i = i;
    }
    
    /**
     * Use constructor with parameters in your code.
     * This one is for bindings only!
     */
    public Struct() {
    }

    private int i;

    public int getI() {
        return i;
    }

    @Override
    public void serialize(Archive archive) {
        i = archive.add(i);
    }
}
