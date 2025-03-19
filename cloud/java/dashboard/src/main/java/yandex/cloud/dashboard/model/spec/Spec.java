package yandex.cloud.dashboard.model.spec;

import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;
import yandex.cloud.dashboard.model.spec.validator.Validatable;

/**
 * @author ssytnik
 */
public interface Spec extends Resolvable, Validatable<SpecValidationContext> {
}
