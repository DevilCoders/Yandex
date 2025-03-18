package ru.yandex.ci.core.flow;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;

import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.config.CiProcessId;

public class FlowUrls {
    private final String ciUrlPrefix;

    public FlowUrls(String ciUrlPrefix) {
        this.ciUrlPrefix = StringUtils.removeEnd(ciUrlPrefix, "/");
    }

    public String toReleasePrefix(String projectId) {
        return "%s/projects/%s/ci/releases".formatted(ciUrlPrefix, projectId);
    }

    public String toActionPrefix(String projectId) {
        return "%s/projects/%s/ci/actions".formatted(ciUrlPrefix, projectId);
    }

    public String toFlowLaunch(String projectId, String dir, String id, int number) {
        return "%s/flow?%s&number=%d".formatted(
                this.toActionPrefix(projectId), encodeProcessId(dir, id), number
        );
    }

    public String encodeProcessId(CiProcessId processId) {
        return encodeProcessId(processId.getDir(), processId.getSubId());
    }

    public String encodeProcessId(String dir, String id) {
        return "dir=%s&id=%s".formatted(encodeParameter(dir), encodeParameter(id));
    }

    public static String encodeParameter(String parameter) {
        return URLEncoder.encode(parameter, StandardCharsets.UTF_8);
    }
}
