package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.util.List;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.branch.model.BranchAutocheckConfig;
import ru.yandex.ci.core.config.branch.model.BranchYamlConfig;

@Value
public class BranchConfigBundle {
    ArcRevision revision;
    Path path;
    @Nullable
    BranchYamlConfig config;
    @Nullable
    ConfigBundle delegatedConfig;
    List<ConfigProblem> problems;

    public static BranchConfigBundle forException(ArcRevision revision, Path path, Exception e) {
        return new BranchConfigBundle(revision, path, null, null, List.of(ConfigProblem.crit(e)));
    }

    public static BranchConfigBundle forParsed(ArcRevision revision,
                                               Path path,
                                               @Nullable BranchYamlConfig config,
                                               @Nullable ConfigBundle delegatedConfig,
                                               List<ConfigProblem> problems) {
        return new BranchConfigBundle(revision, path, config, delegatedConfig, problems);
    }

    public boolean isValid() {
        return ConfigProblem.isValid(problems);
    }

    public boolean hasAutocheckSection() {
        Preconditions.checkState(isValid() && config != null, "Invalid config");
        if (config.getCi() == null) {
            return false;
        }
        return config.getCi().getAutocheck() != null;
    }

    @SuppressWarnings("ConstantConditions")
    public BranchAutocheckConfig getAutocheckSection() {
        Preconditions.checkState(
                hasAutocheckSection(),
                "No autocheck section present in config %s on revision %s",
                path, revision
        );
        return config.getCi().getAutocheck();
    }
}
