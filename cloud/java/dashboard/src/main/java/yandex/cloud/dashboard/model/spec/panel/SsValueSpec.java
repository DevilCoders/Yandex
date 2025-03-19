package yandex.cloud.dashboard.model.spec.panel;

import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.util.Mergeable;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author girevoyt
 */
@Builder(toBuilder = true)
@With
@Value
public class SsValueSpec implements Mergeable<SsValueSpec> {
    public static final SsValueSpec DEFAULT = new SsValueSpec(
            null,
            "",
            "50%",
            "",
            "50%",
            "80%",
            "avg"
    );

    Integer decimals;
    String postfix;
    String postfixFontSize;
    String prefix;
    String prefixFontSize;
    String valueFontSize;
    String valueFunction;

    @Override
    public SsValueSpec merge(SsValueSpec lowerPrecedence) {
        return new SsValueSpec(
                firstNonNullOrNull(decimals, lowerPrecedence.decimals),
                firstNonNullOrNull(postfix, lowerPrecedence.postfix),
                firstNonNullOrNull(postfixFontSize, lowerPrecedence.postfixFontSize),
                firstNonNullOrNull(prefix, lowerPrecedence.prefix),
                firstNonNullOrNull(prefixFontSize, lowerPrecedence.prefixFontSize),
                firstNonNullOrNull(valueFontSize, lowerPrecedence.valueFontSize),
                firstNonNullOrNull(valueFunction, lowerPrecedence.valueFunction)
        );
    }
}
