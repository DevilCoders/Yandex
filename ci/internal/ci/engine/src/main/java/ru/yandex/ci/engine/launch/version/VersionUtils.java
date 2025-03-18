package ru.yandex.ci.engine.launch.version;

import java.util.Objects;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.versioning.Version;

@Slf4j
public class VersionUtils {
    public static final String VERSION_NAMESPACE = "version";

    private VersionUtils() {
    }

    public static String counterKey(CiProcessId processId, ArcBranch branch) {
        return branch + "|" + processId.asString();
    }

    static Version next(CiMainDb db, CiProcessId processId, ArcBranch branch, @Nullable Integer startVersion) {
        Preconditions.checkArgument(startVersion == null || startVersion >= 0,
                "start version can be null or greater that zero, got %s", startVersion);

        long minimal = Objects.requireNonNullElse(startVersion, 1);
        log.info("Generating next version in {}", branch);
        return Version.major(
                String.valueOf(db.counter().incrementAndGetWithLowLimit(
                        VersionUtils.VERSION_NAMESPACE,
                        VersionUtils.counterKey(processId, branch),
                        minimal
                ))
        );

    }
}
