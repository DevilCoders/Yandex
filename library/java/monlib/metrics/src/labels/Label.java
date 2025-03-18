package ru.yandex.monlib.metrics.labels;

import javax.annotation.ParametersAreNonnullByDefault;


/**
 * @author Sergey Polovko
 */
@ParametersAreNonnullByDefault
public interface Label {

    String getKey();
    String getValue();

    byte getKeyCharAt(int pos);
    byte getValueCharAt(int pos);

    int getKeyLength();
    int getValueLength();

    boolean hasKey(String key);
    boolean hasSameKey(Label label);

    int compareKey(String key);
    int compareKeys(Label label);

    void toString(StringBuilder sb);
}
