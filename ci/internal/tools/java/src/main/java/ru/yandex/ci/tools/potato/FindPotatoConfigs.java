package ru.yandex.ci.tools.potato;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.fasterxml.jackson.dataformat.yaml.YAMLFactory;
import com.fasterxml.jackson.dataformat.yaml.YAMLGenerator;
import com.google.common.collect.Maps;
import com.google.common.collect.Sets;
import com.google.common.hash.Hashing;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import one.util.streamex.StreamEx;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVPrinter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.tools.potato.config.Handler;
import ru.yandex.ci.tools.potato.config.PotatoConfig;

public class FindPotatoConfigs {
    private static final boolean ACTUAL_ONLY = false;
    private static final boolean SKIP_CACHE = false;

    private static final Logger log = LoggerFactory.getLogger(FindPotatoConfigs.class);
    private static final Path CACHE_PATH = Paths.get(System.getProperty("user.home"), "Desktop", "potato-configs");
    private static final Path CSV_PATH = Paths.get(System.getProperty("user.home"), "Desktop", "potato.csv");
    private static final String EXT = ".json";

    private final Gson gson;
    private final ObjectMapper yamlMapper;
    private final ObjectMapper jsonMapper;

    public static void main(String[] args) throws IOException {
        FindPotatoConfigs app = new FindPotatoConfigs();
        app.excel();
//        app.analyze();
//        app.findInactive();
    }

    private void excel() throws IOException {
        Map<String, ConfigRef> actual = StreamEx.of(load(true, true))
                .toMap(ConfigRef::getPath, Function.identity());

        Map<String, ConfigRef> known = StreamEx.of(load(SKIP_CACHE, false))
                .toMap(ConfigRef::getPath, Function.identity());

        var difference = Maps.difference(actual, known);
        log.info("Unknown actual: {}, actual known: {}, not actual: {}",
                difference.entriesOnlyOnLeft().size(),
                difference.entriesInCommon().size(),
                difference.entriesOnlyOnRight().size()
        );

        String[] header = {"path", "actual"};
        try (var printer = new CSVPrinter(Files.newBufferedWriter(CSV_PATH), CSVFormat.EXCEL.withHeader(header))) {
            for (ConfigRef config : known.values()) {
                boolean isActual = actual.containsKey(config.getPath());
                printer.printRecord(config.getPath(), isActual);
            }
        }
    }

    private FindPotatoConfigs() {
        gson = new GsonBuilder().setPrettyPrinting().create();

        YAMLFactory yamlFactory = new YAMLFactory()
                .disable(YAMLGenerator.Feature.WRITE_DOC_START_MARKER)
                .enable(YAMLGenerator.Feature.MINIMIZE_QUOTES);

        yamlMapper = configure(new ObjectMapper(yamlFactory));
        jsonMapper = configure(new ObjectMapper());

    }

    private ObjectMapper configure(ObjectMapper mapper) {
        return mapper
                .configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false)
                .setPropertyNamingStrategy(PropertyNamingStrategies.KEBAB_CASE)
                .setSerializationInclusion(JsonInclude.Include.NON_EMPTY);
    }

    @SuppressWarnings("UnusedMethod")
    private void analyze() throws IOException {
        List<ConfigRef> refs = load(SKIP_CACHE, ACTUAL_ONLY);

        Counters counters = new Counters();
        Counters handlers = new Counters();
        Counters tagGroups = new Counters();
        for (ConfigRef ref : refs) {
            try {
                if (ref.getContent().isBlank()) {
                    counters.increment("empty");
                    continue;
                }
                PotatoConfig config = parse(ref);

                if (config.getHandlers() == null || config.getHandlers().isEmpty()) {
                    counters.increment("no handlers");
                    log.info("No handler: {}", ref.getPath());
                }

                if (config.getHandlers() != null) {
                    config.getHandlers().stream()
                            .map(Handler::getName)
                            .forEach(handlers::increment);

                    if (config.getHandlers().size() == 1 && config.getHandlers().get(0).getName().equals("describe")) {
                        counters.increment("describe only");
                    }


                    String tagGroup = config.getHandlers()
                            .stream()
                            .map(Handler::getName)
                            .map(Handlers::byName)
                            .map(Handlers::getTags)
                            .flatMap(Collection::stream)
                            .sorted()
                            .distinct()
                            .collect(Collectors.joining(","));
                    tagGroups.increment(tagGroup);
                }

            } catch (JsonProcessingException e) {
                log.error("Cannot serialize {} ({}):", ref.getPath(),
                        contentPath(ref, cacheName(ref)).toAbsolutePath());
                counters.increment("parse");
            }
        }

        log.info("Total: {}, {}", refs.size(), counters);

        String handlersStats = handlers.entries()
                .map(e -> e.getValue() + ": " + Handlers.byName(e.getKey()))
                .collect(Collectors.joining("\n"));

        log.info("Handlers: {}, {}", refs.size(), handlersStats);

        log.info("Tag groups: {}", tagGroups);
    }

    @SuppressWarnings("UnusedMethod")
    private void findInactive() throws IOException {
        Set<String> all = load(true, false)
                .stream()
                .map(ConfigRef::getPath)
                .collect(Collectors.toSet());
        log.info("Found {} total", all.size());
        Set<String> active = loadFromServices(true)
                .stream()
                .map(ConfigRef::getPath)
                .collect(Collectors.toSet());
        log.info("Found {} active", active.size());


        Set<String> inactive = Sets.difference(all, active);
        log.info("Inactive configs:\n{}", inactive.stream().sorted().collect(Collectors.joining("\n")));

        Set<String> notFound = Sets.difference(active, all);
        log.info("Not found configs:\n{}", String.join("\n", notFound));

        log.info("{}/{} inactive", inactive.size(), all.size());
    }

    private static class Counters {
        private final Map<String, Integer> values = new TreeMap<>();

        public int increment(String counter) {
            return values.compute(counter, (key, value) -> {
                if (value == null) {
                    return 1;
                }
                return value + 1;
            });
        }

        @Override
        public String toString() {
            var byValue = Map.Entry.<String, Integer>comparingByValue();
            return "Counters: \n" + values.entrySet()
                    .stream().sorted(byValue.reversed())
                    .map(e -> "%s: %s".formatted(e.getValue(), e.getKey()))
                    .collect(Collectors.joining("\n"));
        }

        public Stream<Map.Entry<String, Integer>> entries() {
            var byValue = Map.Entry.<String, Integer>comparingByValue();
            return values.entrySet()
                    .stream()
                    .sorted(byValue.reversed());
        }
    }

    private PotatoConfig parse(ConfigRef configRef) throws JsonProcessingException {
        String content = configRef.getContent();
        ObjectMapper mapper = switch (configRef.getType()) {
            case YAML -> yamlMapper;
            case JSON -> jsonMapper;
        };

        PotatoConfig config = mapper.readValue(content, PotatoConfig.class);
        return config;
    }

    private List<ConfigRef> load(boolean skipCache, boolean actualOnly) throws IOException {
        if (Files.exists(CACHE_PATH) && !skipCache) {
            return loadFromCache();
        }

        List<ConfigRef> configs = loadFromServices(actualOnly);
        for (ConfigRef config : configs) {
            save(config);
        }

        return configs;
    }

    @SuppressWarnings("StreamResourceLeak")
    private List<ConfigRef> loadFromCache() throws IOException {
        List<ConfigRef> configs = new ArrayList<>();
        List<Path> files = Files.walk(CACHE_PATH)
                .filter(f -> f.getFileName().toString().length() == 64 + EXT.length())
                .collect(Collectors.toList());

        Gson gson = new Gson();
        for (Path file : files) {
            configs.add(gson.fromJson(Files.readString(file), ConfigRef.class));
        }

        log.info("Loaded {} files from {}", configs.size(), CACHE_PATH);
        return configs;
    }

    private void save(ConfigRef config) throws IOException {
        Files.createDirectories(CACHE_PATH);

        String pathHash = cacheName(config);
        Path serializedPath = CACHE_PATH.resolve(pathHash + EXT);
        Path contentPath = contentPath(config, pathHash);

        String serialized = gson.toJson(config);
        Files.writeString(serializedPath, serialized);
        Files.writeString(contentPath, config.getContent());
        log.info("Config saved to {}, source: {}", serializedPath.toAbsolutePath(), config.getPath());
    }

    private Path contentPath(ConfigRef config, String pathHash) {
        return CACHE_PATH.resolve(pathHash + ".content" + config.getType().getExt());
    }

    private String cacheName(ConfigRef config) {
        return Hashing.sha256().hashString(config.getPath(), StandardCharsets.UTF_8).toString();
    }

    private List<ConfigRef> loadFromServices(boolean actualOnly) {
        List<ConfigRef> configs = new ArrayList<>();

        ArcadiaFetcher arcadia = new ArcadiaFetcher();
        GithubFetcher github = new GithubFetcher();
        BitbucketFetcher bitbucket = new BitbucketFetcher();

        if (actualOnly) {
            PotatoFetcher fetcher = new PotatoFetcher(github, arcadia, bitbucket);
            configs.addAll(fetcher.findAll());
        } else {
            configs.addAll(github.findAll());
            configs.addAll(arcadia.findAll());
            configs.addAll(bitbucket.findAll());
        }

        log.info("Found {} configs", configs.size());
        return configs;
    }
}
