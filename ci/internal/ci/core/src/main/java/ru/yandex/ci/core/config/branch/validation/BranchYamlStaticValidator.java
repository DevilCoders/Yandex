package ru.yandex.ci.core.config.branch.validation;

import java.util.Set;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.config.a.validation.ValidationErrors;
import ru.yandex.ci.core.config.branch.model.BranchCiConfig;
import ru.yandex.ci.core.config.branch.model.BranchYamlConfig;

@RequiredArgsConstructor
public class BranchYamlStaticValidator {

    @Nonnull
    private final BranchYamlConfig branchYaml;

    public Set<String> validate() {
        var ci = branchYaml.getCi();
        if (ci == null) {
            return Set.of();
        }
        return new CiConfigValidator(ci).validate();
    }

    @AllArgsConstructor
    private static class CiConfigValidator {

        @Nonnull
        private final BranchCiConfig ci;

        private final ValidationErrors errors = new ValidationErrors();

        Set<String> validate() {
            validateLargeTests();
            return errors.getErrors();
        }

        private void validateLargeTests() {
            var autocheck = ci.getAutocheck();
            if (autocheck == null) {
                return; // ----
            }
            if (autocheck.getLargeAutostart().isEmpty()) {
                return; // ---
            }
            if (StringUtils.isEmpty(ci.getDelegatedConfig())) {
                errors.add("Delegated config is required when large autostart settings are provided");
            }
        }
    }
}
