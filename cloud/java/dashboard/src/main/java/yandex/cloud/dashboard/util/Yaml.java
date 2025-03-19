package yandex.cloud.dashboard.util;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.yaml.YAMLFactory;
import com.google.common.base.Charsets;
import com.google.common.base.Preconditions;
import com.google.common.io.CharStreams;
import lombok.SneakyThrows;
import lombok.experimental.UtilityClass;
import lombok.extern.log4j.Log4j2;
import yandex.cloud.util.Strings;

import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import static com.fasterxml.jackson.annotation.JsonAutoDetect.Visibility.ANY;
import static com.fasterxml.jackson.annotation.JsonAutoDetect.Visibility.NONE;
import static java.util.regex.Pattern.MULTILINE;

@Log4j2
@UtilityClass
public class Yaml {
    private static final ObjectMapper mapper = new ObjectMapper(new YAMLFactory()) {{
        setVisibility(getSerializationConfig().getDefaultVisibilityChecker()
                .withFieldVisibility(ANY)
                .withGetterVisibility(NONE)
                .withIsGetterVisibility(NONE)
                .withSetterVisibility(NONE)
        );
    }};
    private static final Pattern includePattern = Pattern.compile("^(\\s*)!include (.*)$", MULTILINE);

    @SneakyThrows
    public static <T> T loadFromFile(Class<T> clazz, String file, boolean writeToLog) {
        return parse(clazz, loadFile(file), writeToLog);
    }


    @SneakyThrows
    public static <T> T parse(Class<T> clazz, String content, boolean writeToLog) {
        T result = mapper.readerFor(clazz).readValue(content);

        if (writeToLog) {
            log.info("Specification for type {} is loaded:\n{}",
                    clazz.getSimpleName(),
                    mapper.writerWithDefaultPrettyPrinter().writeValueAsString(result));
        }

        return result;
    }

    static String loadFile(String file) {
        return loadFile(file, List.of());
    }

    @SneakyThrows
    private static String loadFile(String file, List<String> params) {
        URL url = new URL("file:" + file);
        try (InputStream inputStream = url.openStream()) {
            String content = CharStreams.toString(new InputStreamReader(inputStream, Charsets.UTF_8));
            for (int i = 0; i < params.size(); i++) {
                content = content.replace("@" + (i + 1), params.get(i));
            }
            return expandIncludes(Paths.get(file), content);
        }
    }

    private static String expandIncludes(Path path, String content) {
        return Strings.replace(content, includePattern, m -> {
            String indent = m.group(1);
            String params = m.group(2);
            List<String> tokens = Strings.quoteTokenize(params);
            Preconditions.checkArgument(!tokens.isEmpty(), "Wrong !include params: " + params);
            String file = tokens.get(0);
            List<String> args = tokens.subList(1, tokens.size());

            log.info("* processing include (indent = {}, file = {}, args = {}", indent.length(), file, args);
            String innerContent = loadFile(path.resolveSibling(file).normalize().toString(), args);

            return Arrays.stream(innerContent.split("\\n"))
                    .map(line -> indent + line)
                    .collect(Collectors.joining("\n"));
        });
    }

    @SneakyThrows
    public static void writeToFile(Object o, String file) {
        mapper.writeValue(new File(file), o);
    }
}
