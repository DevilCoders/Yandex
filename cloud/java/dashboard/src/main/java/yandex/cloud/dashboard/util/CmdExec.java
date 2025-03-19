package yandex.cloud.dashboard.util;

import com.google.common.base.Strings;
import lombok.extern.log4j.Log4j2;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.stream.Collectors;

/**
 * @author ssytnik
 */
@Log4j2
public class CmdExec {

    private static String exec(String... cmd) {
        try {
            Process p = Runtime.getRuntime().exec(cmd);
            try (BufferedReader br = new BufferedReader(new InputStreamReader(p.getInputStream()))) {
                return Strings.emptyToNull(br.lines().collect(Collectors.joining("\n")).trim());
            }
        } catch (Exception e) {
            log.error(e);
            return null;
        }
    }

    private static String execAndLogResult(String format, String... cmd) {
        String result = exec(cmd);
        log.info(String.format(format, result));
        return result;
    }

    public static String getSvnLastCommitMessage() {
        return execAndLogResult("Found svn commit message = '%s'",
                "/bin/sh", "-c", "svn log -l 1 | tail -n 2 | head -n 1");
    }

    public static String getGitLastCommitHashAndMessage() {
        return execAndLogResult("Found git commit message = '%s'",
                "git", "log", "-1", "--pretty=%h - %s");
    }
}
