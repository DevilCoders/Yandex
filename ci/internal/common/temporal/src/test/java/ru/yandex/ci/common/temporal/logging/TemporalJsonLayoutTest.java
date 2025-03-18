package ru.yandex.ci.common.temporal.logging;

import java.io.StringWriter;
import java.time.Duration;
import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import com.google.common.base.Splitter;
import lombok.extern.slf4j.Slf4j;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.appender.WriterAppender;
import org.apache.logging.log4j.core.time.MutableInstant;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.core.env.AbstractEnvironment;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.TemporalTestBase;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleActivity;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestWorkflow;
import ru.yandex.ci.util.ResourceUtils;

import static org.assertj.core.api.Assertions.assertThat;

class TemporalJsonLayoutTest extends TemporalTestBase {

    private static final ObjectMapper MAPPER = new ObjectMapper();
    private static final String APP_NAME = "test-app";
    private static final String WORKFLOW_ID = "workflow-id";

    @Override
    @BeforeEach
    public void setUp() {
        System.setProperty(AbstractEnvironment.ACTIVE_PROFILES_PROPERTY_NAME, CiProfile.TESTING_PROFILE);
        super.setUp();
    }

    @Override
    protected List<Class<?>> workflowImplementationTypes() {
        return List.of(TemporalJsonLayoutTest.LoggingWorkflow.class);
    }

    @Override
    protected List<Object> activityImplementations() {
        return List.of(new TemporalJsonLayoutTest.LoggingActivity());
    }

    @Test
    void logFormat() {
        var writer = createAppender();
        var execution = temporalService.startDeduplicated(
                SimpleTestWorkflow.class, wf -> wf::run, SimpleTestId.of(WORKFLOW_ID)
        );
        awaitCompletion(execution, Duration.ofMinutes(1));

        List<JsonNode> lines = Splitter.on('\n')
                .trimResults()
                .omitEmptyStrings()
                .splitToList(writer.toString())
                .stream()
                .map(TemporalJsonLayoutTest::toJsonNode)
                .toList();

        replaceFields(lines);

        assertThat(lines).map(JsonNode::toString).containsExactly(
                ResourceUtils.textResource("expected-logs.txt").split("\n")
        );
    }

    @Test
    void nonTemporalSkipped() {
        var writer = createAppender();
        LogManager.getLogger().info("Non temporal message");
        assertThat(writer.toString()).isEmpty();
    }

    /**
     * Меняем те поля, которые динамические и сложно проверяемые
     */
    private void replaceFields(List<JsonNode> lines) {
        for (JsonNode line : lines) {
            var node = (ObjectNode) line;

            replaceFiled(node, "__record_id", "TEST_UUID");
            replaceFiled(node, "time", "2022-03-15T18:02:27.241823Z");
            replaceFiled(node, "thread", "thread");
            replaceFiled(node, "activity_id", "activityId21", false);
            replaceFiled(node, "run_id", "rudId42");
            replaceFiled(node, "hostname", "host");

            String message = node.get("message").asText();
            if (message.length() > 100) {
                node.set("message", TextNode.valueOf(message.substring(0, 100)));
            }
        }
    }

    private void replaceFiled(ObjectNode node, String field, String replacement) {
        replaceFiled(node, field, replacement, true);
    }

    private void replaceFiled(ObjectNode node, String field, String replacement, boolean required) {
        if (required) {
            assertThat(node.has(field)).isTrue();
        }
        if (node.has(field)) {
            node.set(field, TextNode.valueOf(replacement));
        }
    }

    private static JsonNode toJsonNode(String string) {
        try {
            return MAPPER.readTree(string);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    @Test
    void toTime() {
        TemporalJsonLayout layout = TemporalJsonLayout.createLayout(APP_NAME);

        MutableInstant log4jInstant = new MutableInstant();
        long seconds = 1647367347;
        int nanos = 241823000;

        log4jInstant.initFromEpochSecond(seconds, nanos);

        assertThat(layout.toTime(log4jInstant))
                .isEqualTo("2022-03-15T18:02:27.241823Z");
    }

    private StringWriter createAppender() {

        String name = "TemporalLoggingFormatTest";

        var ctx = (LoggerContext) LogManager.getContext(false);
        var config = ctx.getConfiguration();
        var loggerConfig = config.getLoggerConfig(LogManager.ROOT_LOGGER_NAME);

        loggerConfig.removeAppender(name);

        var writer = new StringWriter();

        var appender = WriterAppender.newBuilder()
                .setLayout(TemporalJsonLayout.createLayout(APP_NAME))
                .setTarget(writer)
                .setName(name)
                .build();

        appender.start();

        loggerConfig.addAppender(appender, Level.INFO, TemporalLogFilter.createFilter());
        return writer;
    }

    @Slf4j
    public static class LoggingWorkflow implements SimpleTestWorkflow {

        private final SimpleActivity activity = TemporalService.createActivity(
                SimpleActivity.class,
                Duration.ofMinutes(2)
        );

        @Override
        public void run(SimpleTestId input) {
            log.info("Workflow info");
            activity.runActivity();
            log.error("Workflow error", new RuntimeException("Workflow exception"));
        }
    }

    @Slf4j
    public static class LoggingActivity implements SimpleActivity {

        @Override
        public void runActivity() {
            log.warn("Activity warn {}", 42);
            log.error("Activity error", new RuntimeException("Activity exception"));
        }
    }

}
