package yandex.cloud.dashboard.model.spec.dashboard;

import com.fasterxml.jackson.annotation.JsonCreator;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@With
@Value
public class ConductorSpec implements Spec {
    String group;
    Mode mode;
    Boolean fqdn;

    @JsonCreator
    public ConductorSpec(String group) {
        this(group, null, null);
    }

    public enum Mode {
        host, tree
    }
}
