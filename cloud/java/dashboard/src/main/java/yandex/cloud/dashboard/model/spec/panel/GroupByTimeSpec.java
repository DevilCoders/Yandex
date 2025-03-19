package yandex.cloud.dashboard.model.spec.panel;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;

import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;

import static com.google.common.base.MoreObjects.firstNonNull;
import static com.google.common.base.Preconditions.checkNotNull;

/**
 * @author ssytnik
 */
@With
@Value
public class GroupByTimeSpec implements Spec {
    // https://wiki.yandex-team.ru/solomon/userguide/el/#groupbytime
    // 'integrate' is unavailable for now -- see https://st.yandex-team.ru/CLOUD-40480
    private static final List<String> availableFnList = List.of(
            "min", "max", "avg", "last", "sum", "integrate", "std", "count"
    );
    private static final String GRAFANA_AUTO_WINDOW = "$__interval";
    private static final String DEFAULT_WINDOW = "default";
    private static final String DEFAULT_VALUE_FOR_DEFAULT_WINDOW = GRAFANA_AUTO_WINDOW;

    public static final GroupByTimeSpec AVG_GRAFANA_AUTO = new GroupByTimeSpec("avg", GRAFANA_AUTO_WINDOW);
    public static final GroupByTimeSpec AVG_DEFAULT = new GroupByTimeSpec("avg", DEFAULT_WINDOW);
    public static final GroupByTimeSpec SUM_DEFAULT = new GroupByTimeSpec("sum", DEFAULT_WINDOW);
    public static final GroupByTimeSpec INTEGRATE_DEFAULT = new GroupByTimeSpec("integrate", DEFAULT_WINDOW);
    public static final GroupByTimeSpec MAX_DEFAULT = new GroupByTimeSpec("max", DEFAULT_WINDOW);

    String fn;
    String window;

    public GroupByTimeSpec(String fn, String window) {
        this(Map.of(fn, window));
    }

    @JsonCreator
    public GroupByTimeSpec(Map<String, String> m) {
        Preconditions.checkArgument(m.size() == 1, "GroupByTime should specify a single aggregate fn, but got '%s'", m);
        Entry<String, String> e = m.entrySet().iterator().next();
        fn = e.getKey();
        Preconditions.checkArgument(availableFnList.contains(fn), "Unknown GroupByTime function name: '%s'", fn);
        window = checkNotNull(e.getValue(), "Group by time window should be set explicitly").toLowerCase(Locale.US);
    }

    public String getResolvedWindow(String defaultWindowValue) {
        return DEFAULT_WINDOW.equals(window) ? firstNonNull(defaultWindowValue, DEFAULT_VALUE_FOR_DEFAULT_WINDOW) : window;
    }
}
