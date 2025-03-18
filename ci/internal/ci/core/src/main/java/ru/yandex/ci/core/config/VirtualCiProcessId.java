package ru.yandex.ci.core.config;

import java.nio.file.Path;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class VirtualCiProcessId {
    public static final CiProcessId LARGE_FLOW =
            CiProcessId.ofFlow(Path.of("autocheck/large-tests/a.yaml"), "large-flow");

    CiProcessId ciProcessId;

    @Nullable
    VirtualType virtualType;

    public Path getResolvedPath() {
        return virtualType != null
                ? virtualType.getCiProcessId().getPath()
                : ciProcessId.getPath();
    }

    public static VirtualCiProcessId of(CiProcessId ciProcessId) {
        return new VirtualCiProcessId(ciProcessId, VirtualType.of(ciProcessId));
    }

    public static CiProcessId toVirtual(CiProcessId ciProcessId, VirtualType type) {
        var currentType = VirtualType.of(ciProcessId);
        if (currentType == type) {
            return ciProcessId;
        }
        Preconditions.checkState(currentType == null,
                "Unable to convert %s to different %s", ciProcessId, type);
        return CiProcessId.of(Path.of(type.prefix + ciProcessId.getPath().toString()), ciProcessId.getType(),
                ciProcessId.getSubId());
    }

    @Persisted
    public enum VirtualType {
        VIRTUAL_LARGE_TEST("large-test@", "autocheck", "Large tests"),
        VIRTUAL_NATIVE_BUILD("native-build@", "autocheck", "Native builds");

        private final String prefix;
        private final String service;
        private final String titlePrefix;

        VirtualType(String prefix, String service, String titlePrefix) {
            this.prefix = prefix;
            this.service = service;
            this.titlePrefix = titlePrefix;
        }

        public CiProcessId getCiProcessId() {
            return switch (this) {
                case VIRTUAL_LARGE_TEST, VIRTUAL_NATIVE_BUILD -> LARGE_FLOW; // Same flow so far
            };
        }

        public String getPrefix() {
            return prefix;
        }

        public String getService() {
            return service;
        }

        public String getTitlePrefix() {
            return titlePrefix;
        }

        @Nullable
        public static VirtualType of(CiProcessId ciProcessId) {
            return of(ciProcessId.getPath());
        }

        @Nullable
        public static VirtualType of(Path configPath) {
            var path = configPath.toString();
            for (var type : values()) {
                if (path.startsWith(type.prefix)) {
                    return type;
                }
            }
            return null;
        }
    }
}
