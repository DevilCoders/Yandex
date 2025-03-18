package ru.yandex.ci.core.config;

import java.nio.file.Path;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import lombok.Value;

import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class FlowFullId {

    public static final FlowFullId EMPTY = new FlowFullId("", "");

    /**
     * Директория файла конфига (a.yaml). Сам файл в пути не участвует
     */
    String dir;
    /**
     * Id уникальный в рамках конфигурационного файла
     */
    String id;

    public static FlowFullId of(Path path, String id) {
        AYamlService.verifyIsAYaml(path);
        var parent = path.getParent();
        return new FlowFullId(parent == null ? "" : parent.toString(), id);
    }

    public static FlowFullId ofFlowProcessId(CiProcessId processId) {
        Preconditions.checkArgument(
                CiProcessId.Type.FLOW == processId.getType(),
                "Expected process id of flow, found %s", processId.getType()
        );

        return new FlowFullId(processId.getDir(), processId.getSubId());
    }

    public Path getPath() {
        return AYamlService.dirToConfigPath(dir);
    }

    public String asString() {
        if (Strings.isNullOrEmpty(id)) {
            return dir;
        }
        if (Strings.isNullOrEmpty(dir)) { // Can be empty
            return id;
        }
        return dir + "::" + id;
    }
}
