package ru.test.docs;

import androidx.annotation.NonNull;

public interface InterfaceWithDocs {

    /**
     * Link to {@link Struct}, to {@link Variant#f}, and some unsupported
     * tag {@some.unsupported.tag}.
     *
     * More links after separator: {@link Struct}, {@link
     * JavaInterface#method(int, float, Struct, Variant)}, and link to self:
     * {@link #methodWithDocs(JavaInterface, Struct, LambdaListener)}.
     *
     * @param i - {@link JavaInterface}, does something important
     * @param s - some struct
     *
     * @return true if successful, false - otherwise
     */
    public boolean methodWithDocs(
        @NonNull JavaInterface i,
        @NonNull Struct s,
        @NonNull LambdaListener l);
}
