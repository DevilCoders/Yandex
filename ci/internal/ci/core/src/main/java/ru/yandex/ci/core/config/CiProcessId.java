package ru.yandex.ci.core.config;

import java.nio.file.Path;
import java.util.NoSuchElementException;
import java.util.Objects;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.Getter;
import lombok.Value;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.ydb.Persisted;

/**
 * Сущность объединяющая процессы, описываемые как простыми флоу, так и релизами
 */
@Persisted
@Value
public class CiProcessId {
    public static final String DELIMITER = ":";

    @Getter(AccessLevel.NONE)
    String fullId;

    Path path;
    Type type;
    String subId;

    private CiProcessId(Path path, Type type, String subId) {
        AYamlService.verifyIsAYaml(path);
        this.path = path;
        this.type = type;
        this.subId = subId;

        this.fullId = path + DELIMITER + type.prefix + DELIMITER + subId;
    }

    public String asString() {
        return fullId;
    }

    public String getDir() {
        var parent = path.getParent();
        return parent == null ? "" : parent.toString();
    }

    public static CiProcessId ofString(String fullId) {
        String[] splits = fullId.split(DELIMITER, 4); // always limit
        Preconditions.checkArgument(splits.length == 3,
                "Expect [%s] to be separated as path:type:subId",
                fullId);

        return new CiProcessId(Path.of(splits[0]), Type.byPrefix(splits[1]), splits[2]);
    }

    static CiProcessId of(Path path, Type type, String subId) {
        return new CiProcessId(path, type, subId);
    }

    public static CiProcessId ofFlow(Path configPath, String flowId) {
        return new CiProcessId(configPath, Type.FLOW, flowId);
    }

    public static CiProcessId ofFlow(FlowFullId flowFullId) {
        return new CiProcessId(flowFullId.getPath(), Type.FLOW, flowFullId.getId());
    }

    public static CiProcessId ofRelease(Path path, String releaseId) {
        return new CiProcessId(path, Type.RELEASE, releaseId);
    }

    @Persisted
    public enum Type {
        RELEASE("r"),

        // Может быть как Flow, так и Action-ом; требует проверки в месте использования
        // Переименовывать нельзя, т.к. это название сохраняется в БД
        // Использовать новый тип нельзя, т.к. это сломает историю выполнения и привязку коммитов
        FLOW("f"),

        // Only for backward compatibility with old flows
        @Deprecated
        SYSTEM("s");

        private final String prefix;

        private final String prettyPrint;

        Type(String prefix) {
            this.prefix = prefix;
            this.prettyPrint = StringUtils.capitalize(this.name().toLowerCase());
        }

        public String getPrettyPrint() {
            return prettyPrint;
        }

        static Type byPrefix(String prefix) {
            for (Type value : values()) {
                if (value.prefix.equals(prefix)) {
                    return value;
                }
            }
            throw new NoSuchElementException("No type with prefix '" + prefix + "'");
        }

        public boolean isRelease() {
            return this == RELEASE;
        }
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof CiProcessId)) {
            return false;
        }
        CiProcessId that = (CiProcessId) o;
        return Objects.equals(fullId, that.fullId);
    }

    @Override
    public int hashCode() {
        return Objects.hash(fullId);
    }

    @Override
    public String toString() {
        return asString();
    }
}
