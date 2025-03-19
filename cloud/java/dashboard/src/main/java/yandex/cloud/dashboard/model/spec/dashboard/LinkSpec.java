package yandex.cloud.dashboard.model.spec.dashboard;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.List;
import java.util.Objects;
import java.util.stream.Stream;

/**
 * @author ssytnik
 */
@With
@Value
public class LinkSpec implements Spec {
    String title;
    Boolean targetBlank;
    Boolean vars;

    // one of
    List<String> tags; // title is optional: null => dashboards will be displayed separately, not as a dropdown list
    String uid;
    String url;


    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkArgument(1 == Stream.of(tags, uid, url).filter(Objects::nonNull).count(),
                "Exactly one of 'tags', 'uid' or 'url' should be specified at %s", this);

        Preconditions.checkArgument(title != null || tags != null,
                "Title is only optional for dashboard-to-dashboard links (having 'tags' specified) at %s", this);
    }

    @JsonIgnore
    public boolean isDashboardToDashboardList() {
        return tags != null;
    }

    @JsonIgnore
    public boolean isGraphToDashboard() {
        return uid != null;
    }

    @JsonIgnore
    public boolean isExternal() {
        return url != null;
    }
}
