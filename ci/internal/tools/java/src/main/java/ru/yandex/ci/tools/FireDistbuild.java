package ru.yandex.ci.tools;

import java.io.File;
import java.io.FileWriter;
import java.time.Duration;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.google.common.base.Charsets;
import com.google.common.base.Preconditions;
import com.google.common.io.Files;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetryPolicies;

@SuppressWarnings("DefaultCharset")
public class FireDistbuild {

    static ExecutorService executorService = Executors.newFixedThreadPool(100);


    static TeClient teClient;
    static ArcanumClientImpl arcanumClient;
    static List<String> builds;
    static FileWriter usedWriter;
    static FileWriter launchedWriter;
    static String namespace;

    @SuppressWarnings("JdkObsolete")
    public static void main(String[] stringArgs) throws Exception {

        Args args = new Args();
        JCommander jCommander = new JCommander(args);
        jCommander.parse(stringArgs);
        if (args.help) {
            jCommander.usage();
            System.exit(0);
        }

        Preconditions.checkArgument(
                args.buildPerOur > 0 && args.buildPerOur <= 1000,
                "buildPerOur must be > 0 and <= 1000"
        );

        var properties = HttpClientProperties.builder()
                .endpoint(Misc.TESTENV_API_URL)
                .retryPolicy(RetryPolicies.retryWithSleep(10, Duration.ofMillis(100)))
                .build();

//        arcanumClient = new ArcanumClient("https://a.yandex-team.ru/api", "TODO");

        teClient = new TeClient(properties);

        int intervalMillis = (3600 * 1000 / args.buildPerOur);

        System.out.println("Interval ms " + intervalMillis);
        System.out.println("Namespace " + args.namespace);
        namespace = args.namespace;

        builds = Files.readLines(new File(args.ammo),
                Charsets.UTF_8);

        File usedFile = new File(args.used);

        Set<String> launched;
        if (usedFile.exists()) {
            launched = new HashSet<>(
                    Files.readLines(usedFile, Charsets.UTF_8)
            );
        } else {
            launched = Set.of();
        }


        builds = builds.stream().filter(b -> !launched.contains(b)).collect(Collectors.toList());
        builds = new LinkedList<>(builds);

        usedWriter = new FileWriter(usedFile, true);
        launchedWriter = new FileWriter(args.launched, true);


        int count = 0;
        while (true) {
            count++;
            runNext();
            TimeUnit.MILLISECONDS.sleep(intervalMillis);
            if (count >= 1000) {
                System.out.println("Exiting after 100 checks (failsafe) ...");
                System.exit(42);
            }
        }
    }

    private static synchronized void runNext() {
        if (builds.isEmpty()) {
            System.out.println("No more ammo left. Exiting...");
            System.exit(42);
        }
        String build = builds.remove(0);
        executorService.execute(new Runner(build, usedWriter));

    }


    public static class Runner implements Runnable {
        final String id;
        final FileWriter writer;


        public Runner(String id, FileWriter writer) {
            this.id = id;
            this.writer = writer;
        }

        @Override
        public void run() {
            try {
                System.out.println("launching " + id);


//                String newId = arcanumClient.restartCheck(id);
                String newId = teClient.restartCheck(id, namespace);
                System.out.println("restarted " + id + ", launch id " + newId);


                synchronized (writer) {
                    writer.write(id);
                    writer.write('\n');
                    writer.flush();
                }

                synchronized (launchedWriter) {
                    launchedWriter.write(System.currentTimeMillis() + "\t" + id + "\t" + newId + "\n");
                    launchedWriter.flush();
                }

            } catch (Exception e) {
                e.printStackTrace();
                System.out.println("failed to run " + id + ", running next");
                runNext();
            }

        }
    }

    public static class Args {


        @Parameter(names = {"-b", "--build-per-hour"}, required = true)
        private Integer buildPerOur;

        @Parameter(names = {"-a", "--ammo"}, description = "File with build ids", required = true)
        private String ammo;

        @Parameter(names = {"-u", "--useed-ammo"}, description = "Use build ids file (use for deduplication), will be" +
                " created if not exists", required = true)
        private String used;

        @Parameter(names = {"-l", "--launched"}, description = "Launched build ids file, will be created if not " +
                "exists", required = true)
        private String launched;

        @SuppressWarnings("FieldMayBeFinal")
        @Parameter(names = {"-n", "--namespace"}, description = "DistBuild namespace")
        private String namespace = "FR";

        @Parameter(names = {"-h", "--help"}, help = true)
        private boolean help;


        public Args(Integer buildPerOur, String ammo, String used, String launched) {
            this.buildPerOur = buildPerOur;
            this.ammo = ammo;
            this.used = used;
            this.launched = launched;
            this.help = false;
        }

        public Args() {
        }

    }

}
