package yandex.cloud.dashboard.application;

import com.google.common.base.Charsets;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import lombok.SneakyThrows;
import lombok.extern.log4j.Log4j2;
import yandex.cloud.dashboard.generator.FolderAndDashboard;
import yandex.cloud.dashboard.generator.Generator;
import yandex.cloud.dashboard.integration.grafana.GrafanaDashboardUpdater;
import yandex.cloud.dashboard.model.result.dashboard.Dashboard;
import yandex.cloud.dashboard.model.result.dashboard.DashboardData;
import yandex.cloud.dashboard.model.spec.dashboard.DashboardSpec;
import yandex.cloud.dashboard.util.CmdExec;
import yandex.cloud.dashboard.util.Json;
import yandex.cloud.dashboard.util.Yaml;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.time.ZonedDateTime;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

import static com.google.common.base.MoreObjects.firstNonNull;

/**
 * @author ssytnik
 */
@Log4j2
public class Application {
    private static final boolean SVN_COMMIT_MESSAGE = Boolean.parseBoolean(firstNonNull(System.getenv("SVN_COMMIT_MESSAGE"), "false"));
    private static final boolean GIT_COMMIT_MESSAGE = Boolean.parseBoolean(firstNonNull(System.getenv("GIT_COMMIT_MESSAGE"), "false"));
    private static final boolean WRITE_INPUT = Boolean.parseBoolean(firstNonNull(System.getenv("WRITE_INPUT"), "true"));
    private static final boolean WRITE_OUTPUT = Boolean.parseBoolean(firstNonNull(System.getenv("WRITE_OUTPUT"), "true"));

    private Action action;
    private String in;
    private String out;

    public static void main(String[] args) {
        Application app = new Application();
        app.initParams(args);
        app.run();
    }

    public enum Action {
        /**
         * local run to just check if specification is correct
         * and see the json result
         */
        LOCAL,
        /**
         * dry run: compare local json to remote one, calculate diff,
         * but do not upload the update (Grafana token is required)
         */
        DIFF,
        /**
         * same as previous option, but upload new version
         * to update remote one if there are changes found
         */
        UPLOAD;

        public static Map<String, Action> r = Arrays.stream(Action.values())
                .collect(Collectors.toMap(a -> a.name().toLowerCase(), a -> a));
    }

    private void initParams(String[] args) {
        String envAction = System.getenv("DASHBOARD_ACTION");
        if (envAction != null) {
            log.info("Parameters are found at environment variables");
            action = Preconditions.checkNotNull(Action.r.get(envAction), "Unknown action: %s", envAction);
            in = System.getenv("DASHBOARD_SPEC");
            out = System.getenv("DASHBOARD_RESULT");
        } else if (args.length >= 2 && args.length <= 3) {
            log.info("Parameters are found at command line arguments");
            action = Preconditions.checkNotNull(Action.r.get(args[0]), "Unknown action: %s", args[0]);
            in = args[1];
            out = args.length > 2 ? args[2] : null;
        } else {
            usage();
        }

        if (Strings.isNullOrEmpty(out)) {
            out = in.replaceFirst("[.][^/.]*?$", "") + ".json";
        }
    }

    private void usage() {
        log.error("\n" +
                "Usage\n" +
                "=====\n" +
                "\n" +
                "Application can be run from command line directly (IDE usage is convenient here) or using docker.\n" +
                "\n" +
                "Parameters:\n" +
                "- action: required, can be ‘local’, ‘diff’ or ‘upload’:\n" +
                "  - 'local' run is to check if specification is correct and see the json result;\n" +
                "  - 'diff' compares local json to remote one and prints difference, without uploading;\n" +
                "  - 'upload' will update remote json if there are non-empty differences found.\n" +
                "  Latter two options require Grafana OAuth token.\n" +
                "  To specify Grafana OAuth token, please set up $GRAFANA_OAUTH_TOKEN environment variable.\n" +
                "  Token can be obtained here:\n" +
                "  https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cfa5d75ea95f4ae594acbdaf8ca6770c\n" +
                "  If connection to Grafana is not established, please consider using the following java option: -Djava.net.preferIPv6Addresses=true\n" +
                "- input yaml specification filename: required, /path/to/dashboard.yaml\n" +
                "- [output json filename: optional, defaults to path/to/dashboard.json]\n" +
                "\n" +
                "Parameters can be passed using two ways:\n" +
                "1) environment variables: $DASHBOARD_ACTION, $DASHBOARD_SPEC, [$DASHBOARD_RESULT];\n" +
                "2) command line arguments: <application> <action> <input yaml spec> [<output json>].\n" +
                "\n" +
                "With docker, directory containing spec is mounted as 'data' so application is able to save output json there.\n" +
                "Docker run examples:\n" +
                "\n" +
                "$ docker run --rm -it -v $(pwd)/src/test/resources/dashboard/:/data/ registry.yandex.net/cloud/platform/dashboard:latest java -jar build/java-dashboard.jar local /data/demo.yaml\n" +
                "$ docker run --rm -it -v $(pwd)/src/test/resources/dashboard/:/data/ cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest java -jar build/java-dashboard.jar local /data/demo.yaml\n" +
                "\n" +
                "$ docker run --rm -it -v $(pwd)/src/test/resources/dashboard/:/data/ -e GRAFANA_OAUTH_TOKEN=\"$GRAFANA_OAUTH_TOKEN\" -e DASHBOARD_ACTION=\"upload\" -e DASHBOARD_SPEC=\"/data/demo.yaml\" registry.yandex.net/cloud/platform/dashboard:latest\n" +
                "$ docker run --rm -it -v $(pwd)/src/test/resources/dashboard/:/data/ -e GRAFANA_OAUTH_TOKEN=\"$GRAFANA_OAUTH_TOKEN\" -e DASHBOARD_ACTION=\"upload\" -e DASHBOARD_SPEC=\"/data/demo.yaml\" cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest\n"
        );
        System.exit(-1);
    }

    @SneakyThrows
    private void run() {
        log.info("[{}] Generating dashboard from '{}' to '{}'...", action, in, (out == null ? null : out + "[.i]"));

        Preconditions.checkArgument(!Objects.equals(in, out), "Input and output files should differ");
        Preconditions.checkArgument(new File(in).canRead(), "Cannot read from input file");

        DashboardSpec dashboardSpec = Yaml.loadFromFile(DashboardSpec.class, in, WRITE_INPUT);

        log.info("Generator started");
        List<FolderAndDashboard> folderAndDashboardList = new Generator(dashboardSpec).generateAll();
        int count = folderAndDashboardList.size();
        log.info("Generator finished (dashboard count = {})", count);

        for (int i = 0; i < count; i++) {
            FolderAndDashboard folderAndDashboard = folderAndDashboardList.get(i);
            Long folderId = folderAndDashboard.getFolderId();
            Dashboard dashboard = folderAndDashboard.getDashboard();

            log.info("Dashboard #{} ('{}'):", i, dashboard.getTitle());
            if (WRITE_OUTPUT) {
                String dashboardString = Json.toJson(dashboard);
                log.info("Result json #{} (for '{}'):", i, dashboard.getTitle());
                log.info(dashboardString);

                if (!Strings.isNullOrEmpty(out)) {
                    String curOut = out + (count > 1 ? "." + i : "");
                    Files.write(Paths.get(curOut), dashboardString.getBytes(Charsets.UTF_8));
                }
            }

            if (action == Action.LOCAL) {
                log.info("Local run -- diff to remote Grafana dashboard content will not be performed");
            } else {
                log.info("Calculating diff to remote dashboard content at Grafana...");
                String message = GIT_COMMIT_MESSAGE ? CmdExec.getGitLastCommitHashAndMessage() :
                        SVN_COMMIT_MESSAGE ? CmdExec.getSvnLastCommitMessage() : null;
                new GrafanaDashboardUpdater().createUpdate(new DashboardData(folderId, dashboard, message), action == Action.DIFF);
            }

        }
        log.info("Finished at " + ZonedDateTime.now() + " [" + new File(in).getName() + "]");
    }

}
