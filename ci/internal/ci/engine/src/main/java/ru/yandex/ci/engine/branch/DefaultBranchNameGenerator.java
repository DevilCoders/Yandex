package ru.yandex.ci.engine.branch;

import java.util.Map;

import com.google.common.base.Preconditions;
import org.apache.commons.text.StringSubstitutor;

import ru.yandex.ci.core.config.a.model.ReleaseConfig;

/**
 * Логика генерации имени ветки. Вынесена в отдельный бин для фиксации имени ветки в тестах.
 */
public class DefaultBranchNameGenerator implements BranchNameGenerator {

    @Override
    public String generateName(ReleaseConfig releaseConfig, String version, int seq) {
        Preconditions.checkArgument(releaseConfig.getBranches().isEnabled());
        Preconditions.checkArgument(seq >= 0, "seq %s < 0", seq);

        StringSubstitutor substitutor = new StringSubstitutor(Map.of("version", version));
        substitutor.setEnableUndefinedVariableException(true);

        return substitutor.replace(releaseConfig.getBranches().getPattern()) + formatSeq(seq);
    }

    private static String formatSeq(int seq) {
        if (seq == 0) {
            return "";
        }
        return "-" + seq;
    }

}
