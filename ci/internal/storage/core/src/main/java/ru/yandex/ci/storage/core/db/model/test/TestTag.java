package ru.yandex.ci.storage.core.db.model.test;

import java.util.Collection;
import java.util.Set;

// https://wiki.yandex-team.ru/yatool/test/#tags
public class TestTag {
    public static final String YA_EXTERNAL = "ya:external";
    public static final String YA_DIRTY = "ya:dirty";
    public static final String YA_FAT = "ya:fat";
    public static final String YA_FORCE_DISTBUILD = "ya:force_distbuild";
    public static final String YA_FORCE_SANDBOX = "ya:force_sandbox";
    public static final String YA_MANUAL = "ya:manual";
    public static final String YA_NO_RESTART = "ya:norestart";
    public static final String YA_NOT_AUTOCHECK = "ya:not_autocheck";
    public static final String YA_NO_RETRIES = "ya:noretries";
    public static final String YA_TRACE_OUTPUT = "ya:trace_output";
    public static final String YA_PRIVILEGED = "ya:privileged";
    public static final String YA_SYS_INFO = "ya:sys_info";
    public static final String YA_FULL_LOGS = "ya:full_logs";
    public static final String YA_HUGE_LOGS = "ya:huge_logs";
    public static final String YA_NO_TAGS = "ya:notags";
    public static final String YA_NO_GRACEFUL_SHUTDOWN = "ya:no_graceful_shutdown";

    public static final Collection<String> EXTERNAL = Set.of(YA_EXTERNAL, YA_DIRTY);

    private TestTag() {

    }

    public static boolean isExternal(Set<String> tags) {
        for (var tag : EXTERNAL) {
            if (tags.contains(tag)) {
                return true;
            }
        }
        return false;
    }
}
