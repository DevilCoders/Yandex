package yandex.cloud.dashboard.model.spec.panel.template;

import yandex.cloud.dashboard.model.spec.Spec;

import java.util.List;

/**
 * @author ssytnik
 */
public interface Template<S extends TemplatedPanelSpec<S, T>, T extends Template<S, T>> extends Spec {

    default List<T> precedingTemplates() {
        return List.of();
    }

    S transform(S source);

    default List<T> successiveTemplates() {
        return List.of();
    }

}



