package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonCreator;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;

import java.util.List;

/**
 * @author ssytnik
 */
@With
@Value
public class TemplatesSpec<T extends Template> implements Spec {
    List<T> list;

    @JsonCreator
    public TemplatesSpec(List<T> list) {
        this.list = list;
    }

    @JsonCreator
    public TemplatesSpec(T template) {
        this(List.of(template));
    }
}
