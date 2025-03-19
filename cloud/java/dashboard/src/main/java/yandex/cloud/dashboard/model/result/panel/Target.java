package yandex.cloud.dashboard.model.result.panel;

import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.spec.generic.LabelsSpec;
import yandex.cloud.dashboard.model.spec.panel.FunctionParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.GroupByTimeSpec;

import java.util.List;
import java.util.Map;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Target {
    List<FunctionCall> groupBy;
    Boolean hide;
    String refId;
    List<List<FunctionCall>> select;
    Map<String, String> tags;
    String target;
    String type;

    public void initFieldValue(LabelsSpec labels) {
        target = labels.getSerializedSelector(true);
        addSelect("field", List.of("value"));
    }

    // TODO verify logic correctness here
    public void addGroupByTime(GroupByTimeSpec spec, String defaultTimeWindow) {
        String resolvedWindow = spec.getResolvedWindow(defaultTimeWindow);
        addGroupBy("time", List.of(resolvedWindow));
        addSelect(spec.getFn(), List.of());
        addFunctionCall("group_by_time", List.of(resolvedWindow, spec.getFn()));
    }

    public void addFunctionCall(String fn, FunctionParamsSpec params) {
        FunctionDescriptor d = FunctionDescriptor.get(fn);
        if (d.isAddToSelect()) {
            addSelect(d.getName(), params.getParams());
        }
        addFunctionCall(fn, params.getParams());
    }


    private void addGroupBy(String type, List<String> params) {
        groupBy.add(new FunctionCall(type, params));
    }

    private void addSelect(String fn, List<String> params) {
        select.get(0).add(new FunctionCall(fn, params));
    }

    private void addFunctionCall(String fn, List<String> params) {
        target = FunctionDescriptor.get(fn).wrapWithCall(target, params);
    }
}
