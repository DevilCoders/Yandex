package yandex.cloud.dashboard.model.spec.panel;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Splitter;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.util.Strings;

import java.util.List;

/**
 * @author ssytnik
 */
@With
@Value
public class FunctionParamsSpec implements Spec {
    public static final FunctionParamsSpec NO_PARAMS = new FunctionParamsSpec(List.of());

    private static final Splitter splitter = Splitter.on(',').trimResults();
    List<String> params;

    @JsonCreator
    public FunctionParamsSpec(String params) {
        this(parseParams(params));
    }

    @JsonCreator
    public FunctionParamsSpec(List<String> params) {
        this.params = params;
    }

    static List<String> parseParams(String params) {
        return Strings.isBlank(params) ? List.of() : splitter.splitToList(params);
    }
}
