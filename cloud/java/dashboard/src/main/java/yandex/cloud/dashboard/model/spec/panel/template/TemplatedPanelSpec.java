package yandex.cloud.dashboard.model.spec.panel.template;

import yandex.cloud.dashboard.model.spec.panel.PanelSpec;

/**
 * @author girevoyt
 */
public interface TemplatedPanelSpec<S extends TemplatedPanelSpec<S, T>, T extends Template<S, T>> extends PanelSpec {
    TemplatesSpec<T> getTemplates();

    S withTemplates(TemplatesSpec<T> templates);

    String getRepeat();
}
