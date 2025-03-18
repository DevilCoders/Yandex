package ru.test.docs;

import com.yandex.runtime.bindings.Archive;
import com.yandex.runtime.bindings.Serializable;

/**
 * @exclude
 * This struct should be excluded from documentation.
 */
public final class SuchHidden implements Serializable {

    public SuchHidden(
            float regularField,
            int oneMoreRegularField,
            float twoMoreRegularFields) {
        this.regularField = regularField;
        this.oneMoreRegularField = oneMoreRegularField;
        this.twoMoreRegularFields = twoMoreRegularFields;
    }
    
    /**
     * Use constructor with parameters in your code.
     * This one is for bindings only!
     */
    public SuchHidden() {
    }

    private float regularField;

    public float getRegularField() {
        return regularField;
    }

    /**
     * See {@link #getRegularField()}.
     */
    public SuchHidden setRegularField(float regularField) {
        this.regularField = regularField;
        return this;
    }

    private int oneMoreRegularField;

    public int getOneMoreRegularField() {
        return oneMoreRegularField;
    }

    /**
     * See {@link #getOneMoreRegularField()}.
     */
    public SuchHidden setOneMoreRegularField(int oneMoreRegularField) {
        this.oneMoreRegularField = oneMoreRegularField;
        return this;
    }

    private float twoMoreRegularFields;

    public float getTwoMoreRegularFields() {
        return twoMoreRegularFields;
    }

    /**
     * See {@link #getTwoMoreRegularFields()}.
     */
    public SuchHidden setTwoMoreRegularFields(float twoMoreRegularFields) {
        this.twoMoreRegularFields = twoMoreRegularFields;
        return this;
    }

    @Override
    public void serialize(Archive archive) {
        regularField = archive.add(regularField);
        oneMoreRegularField = archive.add(oneMoreRegularField);
        twoMoreRegularFields = archive.add(twoMoreRegularFields);
    }
}
