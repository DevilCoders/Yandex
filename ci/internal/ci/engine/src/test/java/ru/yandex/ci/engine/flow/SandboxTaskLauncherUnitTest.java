package ru.yandex.ci.engine.flow;

import java.nio.file.Path;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Stream;

import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.ArgumentCaptor;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.util.unit.DataSize;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxResponse;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskGroupStatus;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.JobAttemptsSandboxConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.config.registry.sandbox.SandboxConfig;
import ru.yandex.ci.core.config.registry.sandbox.SandboxSemaphoreAcquireConfig;
import ru.yandex.ci.core.config.registry.sandbox.SandboxSemaphoresConfig;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority.PriorityClass;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority.PrioritySubclass;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobInstanceTable;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.job.JobResources;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletMetadataHelper;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.engine.test.schema.OtherData;
import ru.yandex.ci.engine.test.schema.Simple;
import ru.yandex.ci.engine.test.schema.SimpleData;
import ru.yandex.ci.engine.test.schema.SimpleTasklet;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.gson.GsonPreciseDeserializer;
import ru.yandex.ci.util.gson.JsonObjectBuilder;

import static java.util.Map.entry;
import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

class SandboxTaskLauncherUnitTest extends CommonTestBase {

    private static final String SANDBOX_URL = "sandbox-url";
    private static final String OWNER = "task-owner";
    private static final String SECRET_UID = "secred-uid";
    private static final long SANDBOX_RESOURCE_ID = 89;
    private static final CiProcessId DEFAULT_PROCESS_ID = CiProcessId.ofFlow(Path.of("ci/a.yaml"), "release-something");
    private static final int DEFAULT_LAUNCH_NUMBER = 7;

    private static final Instant NOW = Instant.now();

    private SandboxTaskLauncher sandboxTaskLauncher;
    private long sandboxTaskId = 9;

    @MockBean
    private CiDb db;
    @MockBean
    private JobInstanceTable jobInstanceTable;
    @MockBean
    private SecurityAccessService securityAccessService;
    @MockBean
    private SandboxClient sandboxClient;
    @MockBean
    private TaskBadgeService taskBadgeService;

    private SandboxTaskService sandboxTaskService;

    @BeforeEach
    public void setUp() {
        TestCiDbUtils.mockToCallRealTxMethods(db);

        var properties = HttpClientProperties.builder()
                .endpoint("")
                .retryPolicy(SandboxClient.defaultRetryPolicy())
                .build();
        var sandboxClientFactory = new SandboxClientFactoryImpl("http://localhost", "http://localhost/v2", properties);

        var urlService = new UrlService("https://a-test.yandex-team.ru/");
        var schemaService = new SchemaService();
        sandboxTaskService = new SandboxTaskService(sandboxClient, OWNER, SECRET_UID);
        sandboxTaskLauncher = new SandboxTaskLauncher(
                schemaService,
                db,
                securityAccessService,
                sandboxClientFactory,
                urlService,
                SANDBOX_URL,
                taskBadgeService,
                new TaskletContextProcessor(urlService)
        );
    }

    @Test
    void sandboxContextForTasklet() {
        var taskletImplementation = "TestImplementationPy";
        var taskletMetadata = TaskletMetadataHelper.metadataFor(taskletImplementation, SANDBOX_RESOURCE_ID,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                OtherData.getDescriptor());

        var id = TaskletMetadata.Id.of("", TaskletRuntime.SANDBOX, 0);
        var taskletExecutorContext = TaskletExecutorContext.of(id, SchemaOptions.defaultOptions());
        var launchContext = createLaunchContext(DEFAULT_PROCESS_ID);

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        var testSimpleString = "простая строка";
        var inputResource = Simple.newBuilder()
                .setSimpleString(testSimpleString)
                .build();

        var jobInstance = sandboxTaskLauncher.runSandboxTasklet(
                taskletExecutorContext,
                sandboxTaskService,
                launchContext,
                taskletMetadata,
                JobResources.of(JobResource.regular(inputResource)));

        assertThat(jobInstance.getId())
                .isEqualTo(launchContext.toJobInstanceId());

        var taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());


        // proto-объект SimpleData (input), сконвертированный в JSON
        var expectInput = JsonObjectBuilder.builder()
                .startMap("simple_data_field")
                .withProperty("simple_string", testSimpleString)
                .end()
                .build();

        var task = taskCaptor.getValue();
        assertThat(task.getCustomFields()).isEqualTo(sandboxTaskletCustomFields(taskletImplementation, expectInput));
        assertThat(task.getContext()).isEqualTo(sandboxContextFromLaunchContext(launchContext));
        assertThat(task.getHints()).isEqualTo(List.of(
                "Custom-hint",
                "LAUNCH:launch-id",
                "PR:92"
        ));
        assertThat(task.getTags()).isEqualTo(List.of(
                "USER_CUSTOM_TAG",
                "ANOTHER-USER_CUSTOM-TAG",
                "CI",
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING",
                "JOB-ID:DO-PREPARE"
        ));
    }

    @Test
    void sandboxContextForTaskletWithParameters() {
        var taskletImplementation = "TestImplementationPy";
        var taskletMetadata = TaskletMetadataHelper.metadataFor(taskletImplementation, SANDBOX_RESOURCE_ID,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                OtherData.getDescriptor());

        var id = TaskletMetadata.Id.of("", TaskletRuntime.SANDBOX, 0);
        var taskletExecutorContext = TaskletExecutorContext.of(id, SchemaOptions.defaultOptions());
        var launchContext = createLaunchContext(DEFAULT_PROCESS_ID);

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        int int1 = 12345;
        JsonObject jsonParent1 = JsonObjectBuilder.builder()
                .withProperty("id", int1)
                .build();

        Map<String, JsonObject> upstreamTasks = Map.of(
                "upstream1", JsonObjectBuilder.builder()
                        .startArray("resources")
                        .withValue(jsonParent1)
                        .end()
                        .build()
        );

        var inputResource = Simple.newBuilder()
                .setSimpleString("${to_string(tasks.upstream1.resources[0].id)}")
                .build();

        var jobInstance = sandboxTaskLauncher.runSandboxTasklet(
                taskletExecutorContext,
                sandboxTaskService,
                launchContext,
                taskletMetadata,
                JobResources.of(List.of(JobResource.regular(inputResource)), () -> upstreamTasks));

        assertThat(jobInstance.getId())
                .isEqualTo(launchContext.toJobInstanceId());

        var taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());


        // proto-объект SimpleData (input), сконвертированный в JSON
        var expectInput = JsonObjectBuilder.builder()
                .startMap("simple_data_field")
                .withProperty("simple_string", "12345")
                .end()
                .build();

        var task = taskCaptor.getValue();
        assertThat(task.getCustomFields()).isEqualTo(sandboxTaskletCustomFields(taskletImplementation, expectInput));
        assertThat(task.getContext()).isEqualTo(sandboxContextFromLaunchContext(launchContext));
    }

    @Test
    void sandboxContextForRawTask() {
        var jobInstance = executeSandboxTask(
                DEFAULT_PROCESS_ID,
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );
        verify(jobInstanceTable)
                .find(jobInstance.getId());
        verify(jobInstanceTable, never())
                .find(jobInstance.getId().withNumber(DEFAULT_LAUNCH_NUMBER - 1));
    }

    @Test
    void sandboxContextForRawLargeTask() {
        executeSandboxTask(
                VirtualCiProcessId.toVirtual(DEFAULT_PROCESS_ID, VirtualType.VIRTUAL_LARGE_TEST),
                "LARGE-TEST::CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );
    }

    @Test
    void sandboxContextForLargeTest() {
        var processId = VirtualCiProcessId.toVirtual(
                CiProcessId.ofFlow(
                        Path.of("large-test@yabs/server/cs/test/cs_import_minimal/a.yaml"),
                        "default-linux-x86_64-release-msan@py3test"
                ),
                VirtualType.VIRTUAL_LARGE_TEST
        );
        executeSandboxTask(
                processId,
                "LARGE-TEST::YABS/SERVER/CS/TEST/CS_IMPORT_MINIMAL::DEFAULT-LINUX-X86_64-RELEASE-MSAN::PY3TEST",
                "ACTION:DEFAULT-LINUX-X86_64-RELEASE-MSAN::PY3TEST"
        );
    }

    @Test
    void sandboxContextForRawNativeBuild() {
        executeSandboxTask(
                VirtualCiProcessId.toVirtual(DEFAULT_PROCESS_ID, VirtualType.VIRTUAL_NATIVE_BUILD),
                "NATIVE-BUILD::CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );
    }

    @Test
    void sandboxContextForNativeBuild() {
        var processId = VirtualCiProcessId.toVirtual(
                CiProcessId.ofFlow(
                        Path.of("native-build@yabs/server/cs/test/cs_import_minimal/a.yaml"),
                        "default-linux-x86_64-release-msan@py3test"
                ),
                VirtualType.VIRTUAL_NATIVE_BUILD
        );
        executeSandboxTask(
                processId,
                "NATIVE-BUILD::YABS/SERVER/CS/TEST/CS_IMPORT_MINIMAL::DEFAULT-LINUX-X86_64-RELEASE-MSAN::PY3TEST",
                "ACTION:DEFAULT-LINUX-X86_64-RELEASE-MSAN::PY3TEST"
        );
    }

    @Test
    void sandboxContextForSomeUnexpectedChars() {
        var processId = CiProcessId.ofFlow(
                Path.of("ci/тест1/тест2/a.yaml"),
                "action-значение 1"
        );
        executeSandboxTask(
                processId,
                "CI/-1/-2::ACTION--1",
                "ACTION:ACTION--1"
        );
    }

    @Test
    void sandboxContextForVerLongValue() {
        var s10 = "0123456789";
        String actionId = s10.repeat(30);

        var processId = CiProcessId.ofFlow(
                Path.of("ci/a.yaml"),
                actionId
        );
        executeSandboxTask(
                processId,
                "CI::" + actionId.substring(0, 255 - "CI::".length()),
                "ACTION:" + actionId.substring(0, 255 - "ACTION:".length())
        );
    }

    @Test
    void requirements() {
        FlowLaunchContext launchContext = launchContext()
                .requirements(RequirementsConfig.builder()
                        .withCores(4)
                        .withDisk(DataSize.ofGigabytes(14))
                        .withRam(DataSize.ofGigabytes(4))
                        .withTmp(DataSize.ofMegabytes(512))
                        .startSandbox()
                        .clientTags("!WINDOWS")
                        .containerResource("98")
                        .portoLayers(List.of(12345L))
                        .semaphores(new SandboxSemaphoresConfig(
                                List.of(new SandboxSemaphoreAcquireConfig("test-${context.flow_triggered_by}", 1L, 2L)),
                                List.of("A", "B")
                        ))
                        .cpuModel("AMD64")
                        .host("market-64")
                        .dns(SandboxConfig.Dns.LOCAL)
                        .platform("windows")
                        .privileged(true)
                        .setTcpdumpArgs("dump 443")
                        .end()
                        .build())
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
        sandboxTaskLauncher.runSandboxTask(
                sandboxTask,
                sandboxTaskService,
                launchContext,
                JobResources.of(emptyResourceFor(sandboxTask))
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        SandboxTaskRequirements requirements = taskCaptor.getValue().getRequirements();

        assertThat(TestUtils.readJson(requirements))
                .isEqualTo(TestUtils.readJsonFromString("""
                        {
                          "cores": 4,
                          "host": "market-64",
                          "platform": "windows",
                          "privileged": true,
                          "ram": 4294967296,
                          "semaphores": {
                            "acquires": [
                              {
                                "name": "test-user42",
                                "weight": 1,
                                "capacity": 2
                              }
                            ],
                            "release": [
                              "A",
                              "B"
                            ]
                          },
                          "client_tags": "!WINDOWS",
                          "container_resource": 98,
                          "cpu_model": "AMD64",
                          "disk_space": 15032385536,
                          "dns": "LOCAL",
                          "ramdrive": {
                            "size": 536870912,
                            "type": "tmpfs"
                          },
                          "porto_layers": [
                            12345
                          ],
                          "tcpdump_args": "dump 443"
                        }
                        """));
    }

    @Test
    void sandboxTaskParameters() {
        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");

        String stringValue = "string";
        int intValue = 1116539399;
        long longValue = 42424242422121212L;
        double doubleValue = 42.21;

        JsonObject jsonObject = JsonObjectBuilder.builder()
                .withProperty("string", stringValue)
                .withProperty("true", true)
                .withProperty("false", false)
                .withProperty("int", intValue)
                .withProperty("long", longValue)
                .withProperty("double", doubleValue)
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        var jobResources = JobResource.optional(JobResourceType.ofSandboxTask(sandboxTask), jsonObject);
        sandboxTaskLauncher.runSandboxTask(
                SandboxExecutorContext.of("YA_PACKAGE"),
                sandboxTaskService,
                launchContext().build(),
                JobResources.of(jobResources)
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        List<SandboxCustomField> customFields = taskCaptor.getValue().getCustomFields();

        assertThat(customFields).containsExactly(
                new SandboxCustomField("string", stringValue),
                new SandboxCustomField("true", true),
                new SandboxCustomField("false", false),
                new SandboxCustomField("int", intValue),
                new SandboxCustomField("long", longValue),
                new SandboxCustomField("double", doubleValue)
        );
    }

    @Test
    void sandboxTaskParametersWithResolution() {
        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");

        String string1 = "s1";
        int int1 = 1116539399;
        long long1 = 42424242422121212L;
        double double1 = 42.21;
        boolean boolean1 = true;

        String string2 = "s2";
        int int2 = 1116539499;
        long long2 = 42424242422121222L;
        double double2 = 42.41;
        boolean boolean2 = false;

        assertThat(string1).isNotEqualTo(string2);
        assertThat(int1).isNotEqualTo(int2);
        assertThat(long1).isNotEqualTo(long2);
        assertThat(double1).isNotEqualTo(double2);
        assertThat(boolean1).isNotEqualTo(boolean2);

        var int2s = String.valueOf(int2);
        var long2s = String.valueOf(long2);
        var double2s = String.valueOf(double2);

        JsonObject jsonParent1 = JsonObjectBuilder.builder()
                .withProperty("string", string1)
                .withProperty("boolean", boolean1)
                .withProperty("int", int1)
                .withProperty("long", long1)
                .withProperty("double", double1)
                .build();

        JsonObject jsonParent2 = JsonObjectBuilder.builder()
                .withProperty("string", string2)
                .withProperty("boolean", boolean2)
                .withProperty("int", new JsonPrimitive(int2s).getAsNumber())
                .withProperty("long", new JsonPrimitive(long2s).getAsNumber())
                .withProperty("double", new JsonPrimitive(double2s).getAsNumber())
                .build();

        Map<String, JsonObject> upstreamTasks = Map.of(
                // Sandbox задача
                "upstream1", JsonObjectBuilder.builder()
                        .startArray("resources")
                        .withValue(jsonParent1)
                        .end()
                        .build(),
                // Tasklet задача
                "upstream2", jsonParent2
        );


        JsonObject jsonObject = JsonObjectBuilder.builder()
                .withProperty("string1", "${tasks.upstream1.resources[0].string}")
                .withProperty("string2", "${tasks.upstream2.string}")
                .withProperty("boolean1", "${tasks.upstream1.resources[0].boolean}")
                .withProperty("boolean2", "${tasks.upstream2.boolean}")
                .withProperty("int1", "${tasks.upstream1.resources[0].int}")
                .withProperty("int2", "${tasks.upstream2.int}")
                .withProperty("long1", "${tasks.upstream1.resources[0].long}")
                .withProperty("long2", "${tasks.upstream2.long}")
                .withProperty("double1", "${tasks.upstream1.resources[0].double}")
                .withProperty("double2", "${tasks.upstream2.double}")
                .withProperty("len", "${length(tasks.*)}")
                .withProperty("lazyNumberInt", new JsonPrimitive(int2s).getAsNumber())
                .withProperty("lazyNumberLong", new JsonPrimitive(long2s).getAsNumber())
                .withProperty("lazyNumberDouble", new JsonPrimitive(double2s).getAsNumber())
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        var jobResources = List.of(JobResource.optional(JobResourceType.ofSandboxTask(sandboxTask), jsonObject));
        sandboxTaskLauncher.runSandboxTask(
                SandboxExecutorContext.of("YA_PACKAGE"),
                sandboxTaskService,
                launchContext().build(),
                JobResources.of(jobResources, () -> upstreamTasks)
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        List<SandboxCustomField> customFields = taskCaptor.getValue().getCustomFields();

        assertThat(customFields).containsExactly(
                new SandboxCustomField("string1", string1),
                new SandboxCustomField("string2", string2),
                new SandboxCustomField("boolean1", boolean1),
                new SandboxCustomField("boolean2", boolean2),
                new SandboxCustomField("int1", int1),
                new SandboxCustomField("int2", int2),
                new SandboxCustomField("long1", long1),
                new SandboxCustomField("long2", long2),
                new SandboxCustomField("double1", double1),
                new SandboxCustomField("double2", double2),
                new SandboxCustomField("len", 2L), // Всего 2 таски
                new SandboxCustomField("lazyNumberInt", int2),
                new SandboxCustomField("lazyNumberLong", long2),
                new SandboxCustomField("lazyNumberDouble", double2)
        );
    }

    @Test
    void sandboxTaskContextParameters() {
        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");

        String string1 = "s1";
        int int1 = 1116539399;
        long long1 = 42424242422121212L;
        double double1 = 42.21;
        boolean boolean1 = true;

        String string2 = "s2";
        int int2 = 1116539499;
        long long2 = 42424242422121222L;
        double double2 = 42.41;
        boolean boolean2 = false;

        JsonObject jsonParent1 = JsonObjectBuilder.builder()
                .withProperty("string", string1)
                .withProperty("boolean", boolean1)
                .withProperty("int", int1)
                .withProperty("long", long1)
                .withProperty("double", double1)
                .build();

        JsonObject jsonParent2 = JsonObjectBuilder.builder()
                .withProperty("string", string2)
                .withProperty("boolean", boolean2)
                .withProperty("int", int2)
                .withProperty("long", long2)
                .withProperty("double", double2)
                .build();

        Map<String, JsonObject> upstreamTasks = Map.of(
                "upstream1", jsonParent1,
                "upstream2", jsonParent2
        );

        JsonObject jsonObjectInput = JsonObjectBuilder.builder()
                .withProperty("string", "${tasks.upstream1.string}")
                .withProperty("boolean", "${tasks.upstream1.boolean}")
                .withProperty("int", "${tasks.upstream1.int}")
                .withProperty("long", "${tasks.upstream1.long}")
                .withProperty("double", "${tasks.upstream1.double}")
                .build();

        JsonObject jsonObjectContext = JsonObjectBuilder.builder()
                .withProperty("string", "${tasks.upstream2.string}")
                .withProperty("boolean", "${tasks.upstream2.boolean}")
                .withProperty("int", "${tasks.upstream2.int}")
                .withProperty("long", "${tasks.upstream2.long}")
                .withProperty("double", "${tasks.upstream2.double}")
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        sandboxTaskLauncher.runSandboxTask(
                SandboxExecutorContext.of("YA_PACKAGE"),
                sandboxTaskService,
                launchContext().build(),
                JobResources.of(
                        List.of(
                                JobResource.optional(JobResourceType.ofSandboxTask(sandboxTask),
                                        jsonObjectInput),
                                JobResource.optional(JobResourceType.ofSandboxTaskContext(sandboxTask),
                                        jsonObjectContext)
                        ),
                        () -> upstreamTasks
                )
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        List<SandboxCustomField> customFields = taskCaptor.getValue().getCustomFields();
        assertThat(customFields).containsExactly(
                new SandboxCustomField("string", string1),
                new SandboxCustomField("boolean", boolean1),
                new SandboxCustomField("int", int1),
                new SandboxCustomField("long", long1),
                new SandboxCustomField("double", double1)
        );

        assertThat(taskCaptor.getValue().getContext())
                .containsAllEntriesOf(
                        Map.of("string", string2,
                                "boolean", boolean2,
                                "int", int2,
                                "long", long2,
                                "double", double2
                        )
                );
    }

    @Test
    void priorityDefault() {
        FlowLaunchContext launchContext = launchContext()
                .requirements(RequirementsConfig.builder()
                        .startSandbox()
                        .priority(new TaskPriority(TaskPriority.PriorityClass.USER, TaskPriority.PrioritySubclass.HIGH))
                        .end()
                        .build())
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
        sandboxTaskLauncher.runSandboxTask(
                sandboxTask,
                sandboxTaskService,
                launchContext,
                JobResources.of(emptyResourceFor(sandboxTask))
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        var priority = taskCaptor.getValue().getPriority();
        assertThat(priority)
                .isNotNull()
                .isEqualTo(new SandboxTaskPriority(
                        SandboxTaskPriority.PriorityClass.USER, SandboxTaskPriority.PrioritySubclass.HIGH));
    }

    @Test
    void priorityOverride() {
        FlowLaunchContext launchContext = launchContext()
                .requirements(RequirementsConfig.builder()
                        .startSandbox()
                        .priority(new TaskPriority(PriorityClass.USER, PrioritySubclass.HIGH))
                        .end()
                        .build())
                .runtimeConfig(RuntimeConfig.of(
                        RuntimeSandboxConfig.builder()
                                .priority(new TaskPriority(PriorityClass.SERVICE, PrioritySubclass.NORMAL))
                                .build()))
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
        sandboxTaskLauncher.runSandboxTask(
                sandboxTask,
                sandboxTaskService,
                launchContext,
                JobResources.of(emptyResourceFor(sandboxTask))
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        var priority = taskCaptor.getValue().getPriority();
        assertThat(priority)
                .isNotNull()
                .isEqualTo(new SandboxTaskPriority(
                        SandboxTaskPriority.PriorityClass.SERVICE, SandboxTaskPriority.PrioritySubclass.NORMAL));
    }

    @Test
    void priorityOverrideExpression() {
        FlowLaunchContext launchContext = launchContext()
                .requirements(RequirementsConfig.builder()
                        .startSandbox()
                        .priority(new TaskPriority(PriorityClass.USER, PrioritySubclass.HIGH))
                        .end()
                        .build())
                .runtimeConfig(RuntimeConfig.of(
                        RuntimeSandboxConfig.builder()
                                .priority(new TaskPriority("${'SERVICE:NORMAL'}"))
                                .build()))
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
        sandboxTaskLauncher.runSandboxTask(
                sandboxTask,
                sandboxTaskService,
                launchContext,
                JobResources.of(emptyResourceFor(sandboxTask))
        );

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        var priority = taskCaptor.getValue().getPriority();
        assertThat(priority)
                .isNotNull()
                .isEqualTo(new SandboxTaskPriority(
                        SandboxTaskPriority.PriorityClass.SERVICE, SandboxTaskPriority.PrioritySubclass.NORMAL));
    }

    @Test
    void makeDescriptionFromTrunk() {
        var launchContext = launchContext()
                .jobTitle("Job title")
                .targetRevision(OrderedArcRevision.fromHash("sha123", ArcBranch.trunk(), 4010, 305))
                .build();
        var description = sandboxTaskLauncher.makeDescription(launchContext, "http://url-to-ci-launch",
                "http://url-to-job-in-ci-launch");
        assertThat(description).isEqualTo("""
                Job title\
                <br>CI job: <a href="http://url-to-job-in-ci-launch">do-prepare [launchNumber=7]</a>\
                <br>CI launch: <a href="http://url-to-ci-launch">Тест #1</a> \
                [flowId=ci::release-something, flowLaunchId=launch-id, type=flow]\
                <br>Arcadia Commit: <a href="https://a-test.yandex-team.ru/arc_vcs/commit/sha123">sha123</a>\
                 (<a href="https://a-test.yandex-team.ru/arc/commit/r4010">r4010</a>)\
                <br>Pull request: <a href="https://a-test.yandex-team.ru/review/92">92</a>"""
        );
    }

    @Test
    void makeDescriptionFromBranch() {
        var launchContext = launchContext()
                .jobTitle("Job title")
                .targetRevision(OrderedArcRevision.fromHash("sha123", ArcBranch.ofBranchName("ci/123"), 4010, 305))
                .build();
        var description = sandboxTaskLauncher.makeDescription(launchContext, "http://url-to-ci-launch",
                "http://url-to-job-in-ci-launch");
        assertThat(description).isEqualTo("""
                Job title\
                <br>CI job: <a href="http://url-to-job-in-ci-launch">do-prepare [launchNumber=7]</a>\
                <br>CI launch: <a href="http://url-to-ci-launch">Тест #1</a> \
                [flowId=ci::release-something, flowLaunchId=launch-id, type=flow]\
                <br>Arcadia Commit: <a href="https://a-test.yandex-team.ru/arc_vcs/commit/sha123">sha123</a>\
                <br>Pull request: <a href="https://a-test.yandex-team.ru/review/92">92</a>"""
        );
    }

    @Test
    void makeDescriptionFromPr() {
        var launchContext = launchContext()
                .jobTitle("Job title")
                .jobDescription("Job description")
                .targetRevision(OrderedArcRevision.fromHash("sha123", ArcBranch.ofPullRequest(123456), 123456, 123456))
                .build();
        var description = sandboxTaskLauncher.makeDescription(launchContext, "http://url-to-ci-launch",
                "http://url-to-job-in-ci-launch");
        assertThat(description).isEqualTo("""
                Job title\
                <br>Job description\
                <br>CI job: <a href="http://url-to-job-in-ci-launch">do-prepare [launchNumber=7]</a>\
                <br>CI launch: <a href="http://url-to-ci-launch">Тест #1</a> \
                [flowId=ci::release-something, flowLaunchId=launch-id, type=flow]\
                <br>Arcadia Commit: <a href="https://a-test.yandex-team.ru/arc_vcs/commit/sha123">sha123</a>\
                <br>Pull request: <a href="https://a-test.yandex-team.ru/review/92">92</a>"""
        );
    }

    @Test
    void makeDescription_whenParametersAreNullAndPullRequestInfoIsNull() {
        var launchContext = launchContext()
                .targetRevision(OrderedArcRevision.fromHash("sha123", ArcBranch.ofPullRequest(123456), 123456, 123456))
                .launchPullRequestInfo(null)
                .build();
        var description = sandboxTaskLauncher.makeDescription(launchContext, "", "");
        assertThat(description).isEqualTo("""
                CI job: do-prepare [launchNumber=7]\
                <br>CI launch: Тест #1 [flowId=ci::release-something, flowLaunchId=launch-id, type=flow]\
                <br>Arcadia Commit: <a href="https://a-test.yandex-team.ru/arc_vcs/commit/sha123">sha123</a>"""
        );
    }

    @Test
    void testSandboxTaskWithRestoreOnFirstAttempt() {
        var jobAttemptsConfig = JobAttemptsConfig.builder()
                .sandboxConfig(JobAttemptsSandboxConfig.builder()
                        .reuseTasks(true)
                        .build())
                .build();
        var prevJobInstance = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .jobLaunchNumber(1)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );
        verify(jobInstanceTable)
                .find(prevJobInstance.getId());
        verify(jobInstanceTable, never())
                .find(prevJobInstance.getId().withNumber(0));

        reset(sandboxClient);

        var jobInstance = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .jobLaunchNumber(2)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );

        // No actual jobInstance for previous run
        assertThat(prevJobInstance.getSandboxTaskId())
                .isNotEqualTo(jobInstance.getSandboxTaskId());
    }

    @ParameterizedTest
    @MethodSource("statusesWithContinueExecution")
    void testSandboxTaskWithContinue(SandboxTaskStatus taskStatus) {
        var jobAttemptsConfig = JobAttemptsConfig.builder()
                .sandboxConfig(JobAttemptsSandboxConfig.builder()
                        .reuseTasks(true)
                        .build())
                .build();

        var prevJobInstance = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );

        reset(sandboxClient);

        var launchContext = launchContext(DEFAULT_PROCESS_ID)
                .jobAttemptsConfig(jobAttemptsConfig)
                .jobLaunchNumber(DEFAULT_LAUNCH_NUMBER + 1)
                .build();

        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(jobInstanceTable.find(prevJobInstance.getId()))
                .thenReturn(Optional.of(prevJobInstance));

        when(sandboxClient.getTaskStatus(eq(prevJobInstance.getSandboxTaskId())))
                .thenReturn(SandboxResponse.of(taskStatus, 1, 1));

        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
        JobInstance jobInstance = sandboxTaskLauncher.runSandboxTask(
                sandboxTask,
                sandboxTaskService,
                launchContext,
                JobResources.of(emptyResourceFor(sandboxTask))
        );

        assertThat(jobInstance.getId())
                .isEqualTo(launchContext.toJobInstanceId());

        verify(sandboxClient, never()).createTask(any(SandboxTask.class));
        verify(sandboxClient, never()).startTask(anyLong(), anyString());

        assertThat(jobInstance.getSandboxTaskId())
                .isEqualTo(prevJobInstance.getSandboxTaskId());
    }

    @ParameterizedTest
    @MethodSource("statusesWithRestart")
    void testSandboxTaskWithRestartExistingTask(SandboxTaskStatus taskStatus) {

        var jobAttemptsConfig = JobAttemptsConfig.builder()
                .sandboxConfig(JobAttemptsSandboxConfig.builder()
                        .reuseTasks(true)
                        .build())
                .build();

        var prevJobInstance = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );

        int launchNumber = DEFAULT_LAUNCH_NUMBER;

        for (int i = 0; i <= 6; i++) {
            reset(sandboxClient);

            launchNumber++;
            var launchContext = launchContext(DEFAULT_PROCESS_ID)
                    .jobAttemptsConfig(jobAttemptsConfig)
                    .jobLaunchNumber(launchNumber)
                    .build();

            when(db.jobInstance()).thenReturn(jobInstanceTable);
            when(jobInstanceTable.find(prevJobInstance.getId()))
                    .thenReturn(Optional.of(prevJobInstance));

            when(sandboxClient.getTaskStatus(eq(prevJobInstance.getSandboxTaskId())))
                    .thenReturn(SandboxResponse.of(taskStatus, 1, 1));

            when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

            SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
            JobInstance jobInstance = sandboxTaskLauncher.runSandboxTask(
                    sandboxTask,
                    sandboxTaskService,
                    launchContext,
                    JobResources.of(emptyResourceFor(sandboxTask))
            );

            assertThat(jobInstance.getId())
                    .isEqualTo(launchContext.toJobInstanceId());

            verify(sandboxClient, never()).createTask(any(SandboxTask.class));
            verify(sandboxClient).startTask(eq(jobInstance.getSandboxTaskId()), anyString());

            assertThat(jobInstance.getSandboxTaskId())
                    .isEqualTo(prevJobInstance.getSandboxTaskId()); // Always reuse a task

            prevJobInstance = jobInstance;
        }
    }

    @ParameterizedTest
    @MethodSource("statusesWithRestart")
    void testSandboxTaskWithRestartExistingTaskLimitedRuns(SandboxTaskStatus taskStatus) {

        var jobAttemptsConfig = JobAttemptsConfig.builder()
                .sandboxConfig(JobAttemptsSandboxConfig.builder()
                        .reuseTasks(true)
                        .useAttempts(3)
                        .build())
                .build();


        // First attempt
        var prevJobInstance = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );

        when(db.jobInstance()).thenReturn(jobInstanceTable);

        var jobInstances = new ArrayList<JobInstance>();
        jobInstances.add(prevJobInstance);

        int launchNumber = DEFAULT_LAUNCH_NUMBER;

        for (int i = 0; i <= 6; i++) {
            reset(sandboxClient);

            launchNumber++;
            var launchContext = launchContext(DEFAULT_PROCESS_ID)
                    .jobAttemptsConfig(jobAttemptsConfig)
                    .jobLaunchNumber(launchNumber)
                    .build();

            when(jobInstanceTable.find(prevJobInstance.getId()))
                    .thenReturn(Optional.of(prevJobInstance));

            when(sandboxClient.getTaskStatus(eq(prevJobInstance.getSandboxTaskId())))
                    .thenReturn(SandboxResponse.of(taskStatus, 1, 1));

            when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);
            when(sandboxClient.createTask(any(SandboxTask.class))).thenReturn(taskOutput());

            SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
            JobInstance jobInstance = sandboxTaskLauncher.runSandboxTask(
                    sandboxTask,
                    sandboxTaskService,
                    launchContext,
                    JobResources.of(emptyResourceFor(sandboxTask))
            );

            assertThat(jobInstance.getId())
                    .isEqualTo(launchContext.toJobInstanceId());

            verify(sandboxClient).startTask(eq(jobInstance.getSandboxTaskId()), anyString());


            jobInstances.add(jobInstance);
            prevJobInstance = jobInstance;

            // max attempts = 3
            // reuseCount = 0, jobInstances[0]
            // i = 0, reuseCount = 1, jobInstances[1]
            // i = 1, reuseCount = 2, jobInstances[2]
            // i = 2, reuseCount = 3 -> restart, jobInstances[3], reuseCount = 0
            // i = 3, reuseCount = 1, jobInstances[4]
            // i = 4, reuseCount = 2, jobInstances[5]
            // i = 5, reuseCount = 3 -> restart, jobInstances[6], reuseCount = 0
            // i = 6, reuseCount = 1 -> jobInstances[7]

            var expectRecreate = i == 2 || i == 5;
            if (expectRecreate) {
                verify(sandboxClient).createTask(any(SandboxTask.class));
            } else {
                verify(sandboxClient, never()).createTask(any(SandboxTask.class));
            }

            var expectIndex =
                    switch (i) {
                        case 0, 1 -> 0;
                        case 2, 3, 4 -> 3;
                        case 5, 6 -> 6;
                        default -> throw new IllegalStateException("Unsupported index: " + i);
                    };

            assertThat(jobInstance.getSandboxTaskId())
                    .describedAs("Current job at index " + i + " must be equals with expected index " + expectIndex)
                    .isEqualTo(jobInstances.get(expectIndex).getSandboxTaskId());

        }
    }

    @ParameterizedTest
    @MethodSource("statusesWithRecreate")
    void testSandboxTaskWithRecreateTask(SandboxTaskStatus taskStatus) {

        var jobAttemptsConfig = JobAttemptsConfig.builder()
                .sandboxConfig(JobAttemptsSandboxConfig.builder()
                        .reuseTasks(true)
                        .build())
                .build();
        var prevJobInstance = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );
        verify(jobInstanceTable)
                .find(prevJobInstance.getId());
        verify(jobInstanceTable)
                .find(prevJobInstance.getId().withNumber(DEFAULT_LAUNCH_NUMBER - 1));

        reset(sandboxClient);
        reset(jobInstanceTable);

        when(jobInstanceTable.find(prevJobInstance.getId()))
                .thenReturn(Optional.of(prevJobInstance));

        when(sandboxClient.getTaskStatus(eq(prevJobInstance.getSandboxTaskId())))
                .thenReturn(SandboxResponse.of(taskStatus, 1, 1));

        var jobInstance2 = executeSandboxTask(
                launchContext(DEFAULT_PROCESS_ID)
                        .jobAttemptsConfig(jobAttemptsConfig)
                        .jobLaunchNumber(DEFAULT_LAUNCH_NUMBER + 1)
                        .build(),
                "CI::RELEASE-SOMETHING",
                "ACTION:RELEASE-SOMETHING"
        );

        assertThat(prevJobInstance.getSandboxTaskId())
                .isNotEqualTo(jobInstance2.getSandboxTaskId());
    }

    JobInstance executeSandboxTask(
            CiProcessId ciProcessId,
            String expectFlow,
            String expectAction
    ) {
        return executeSandboxTask(createLaunchContext(ciProcessId), expectFlow, expectAction);
    }

    JobInstance executeSandboxTask(
            FlowLaunchContext launchContext,
            String expectFlow,
            String expectAction
    ) {
        when(db.jobInstance()).thenReturn(jobInstanceTable);
        when(sandboxClient.createTask(any())).thenReturn(taskOutput());
        when(sandboxClient.startTask(anyLong(), any())).thenReturn(BatchResult.EMPTY);

        SandboxExecutorContext sandboxTask = SandboxExecutorContext.of("YA_PACKAGE");
        JobInstance jobInstance = sandboxTaskLauncher.runSandboxTask(
                sandboxTask,
                sandboxTaskService,
                launchContext,
                JobResources.of(emptyResourceFor(sandboxTask))
        );

        assertThat(jobInstance.getId())
                .isEqualTo(launchContext.toJobInstanceId());

        ArgumentCaptor<SandboxTask> taskCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(taskCaptor.capture());

        assertThat(taskCaptor.getValue().getContext()).isEqualTo(
                sandboxContextFromLaunchContext(launchContext)
        );
        assertThat(taskCaptor.getValue().getHints()).isEqualTo(List.of(
                "Custom-hint",
                "LAUNCH:launch-id",
                "PR:92"
        ));
        assertThat(taskCaptor.getValue().getTags()).isEqualTo(List.of(
                "USER_CUSTOM_TAG",
                "ANOTHER-USER_CUSTOM-TAG",
                "CI",
                expectFlow,
                expectAction,
                "JOB-ID:DO-PREPARE"
        ));

        return jobInstance;
    }


    private SandboxTaskOutput taskOutput() {
        return new SandboxTaskOutput("YA_PACKAGE", sandboxTaskId++, SandboxTaskStatus.SUCCESS);
    }

    private static JobResource emptyResourceFor(SandboxExecutorContext sandboxTask) {
        return JobResource.optional(JobResourceType.ofSandboxTask(sandboxTask), new JsonObject());
    }

    private static List<SandboxCustomField> sandboxTaskletCustomFields(String taskletImplementation,
                                                                       JsonObject taskletInput) {
        return List.of(
                new SandboxCustomField("__tasklet_name__", taskletImplementation),
                new SandboxCustomField("__tasklet_input__", GsonPreciseDeserializer.toMap(taskletInput)),
                new SandboxCustomField("__tasklet_secret__", "secred-uid#ci.token")
        );
    }

    private static Map<String, Map<String, Object>> sandboxContextFromLaunchContext(FlowLaunchContext launchContext) {
        var jobId = launchContext.getJobId();
        var launchNumber = launchContext.getJobLaunchNumber();
        var launchId = launchContext.getLaunchId();
        var processId = launchId.getProcessId();
        Map<String, Object> ciContext = new HashMap<>(Map.ofEntries(
                entry("job_instance_id", Map.of(
                        "flow_launch_id", launchContext.getFlowLaunchId().asString(),
                        "job_id", jobId,
                        "number", launchNumber
                )),
                entry("target_revision", Map.of(
                        "hash", TestData.TRUNK_R2.getCommitId(),
                        "number", TestData.TRUNK_R2.getNumber(),
                        "pull_request_id", TestData.TRUNK_R2.getPullRequestId()
                )),
                entry("target_commit", Map.of(
                        "author", TestData.TRUNK_COMMIT_2.getAuthor(),
                        "date", TestData.TRUNK_COMMIT_2.getCreateTime().toString(),
                        "message", TestData.TRUNK_COMMIT_2.getMessage(),
                        "revision", Map.of(
                                "hash", TestData.TRUNK_R2.getCommitId(),
                                "number", TestData.TRUNK_R2.getNumber(),
                                "pull_request_id", TestData.TRUNK_R2.getPullRequestId()
                        )
                )),
                entry("secret_uid", SECRET_UID),
                entry("config_info", Map.of(
                        "path", processId.getPath().toString(),
                        "dir", processId.getDir(),
                        "id", processId.getSubId())),
                entry("launch_number", 100500),
                entry("launch_pull_request_info", Map.of(
                        "pull_request", Map.of(
                                "id", TestData.TRUNK_R2.getPullRequestId(),
                                "author", TestData.CI_USER,
                                "summary", "Some Title",
                                "description", "Some Description"
                        ),
                        "diff_set", Map.of(
                                "id", 10L
                        ),
                        "vcs_info", Map.of(
                                "merge_revision_hash", "merge_rev",
                                "upstream_revision_hash", "upstream_rev",
                                "upstream_branch", "upstream_branch",
                                "feature_revision_hash", "feature_rev",
                                "feature_branch", "feature_branch"
                        ),
                        "merge_requirement_id", Map.of(
                                "system", "system",
                                "type", "type"
                        ),
                        "issues", List.of(
                                Map.of("id", "CI-1"),
                                Map.of("id", "CI-2")
                        ),
                        "labels", List.of("label1", "label2")
                )),
                entry("version", "103.17"),
                entry("version_info", Map.of("full", "103.17", "major", "103", "minor", "17")),
                entry("flow_triggered_by", TestData.USER42),
                entry("flow_type", "DEFAULT"),
                entry("title", "Тест #1"),
                entry("branch", "releases/ci/release-component-2"),
                entry("flow_started_at", NOW.toString()),
                entry("job_started_at", NOW.toString())
        ));

        var project = launchContext.getProjectId();
        var dir = FlowUrls.encodeParameter(processId.getDir());
        var id = FlowUrls.encodeParameter(processId.getSubId());
        ciContext.put("ci_job_url", String.format("https://a-test.yandex-team.ru/projects/%s/ci/actions/flow" +
                        "?dir=%s&id=%s&number=%s&selectedJob=%s&launchNumber=%s",
                project, dir, id, launchId.getNumber(), jobId, launchNumber));
        ciContext.put("ci_url", String.format("https://a-test.yandex-team.ru/projects/%s/ci/actions/flow" +
                        "?dir=%s&id=%s&number=%s",
                project, dir, id, launchId.getNumber()));

        return Map.of("__CI_CONTEXT", ciContext);
    }

    private static FlowLaunchContext createLaunchContext(CiProcessId ciProcessId) {
        return launchContext(ciProcessId).build();
    }

    private static FlowLaunchContext.Builder launchContext() {
        return launchContext(CiProcessId.ofFlow(Path.of("ci/a.yaml"), "release-something"));
    }

    private static FlowLaunchContext.Builder launchContext(CiProcessId ciProcessId) {
        FlowLaunchId flowLaunchId = FlowLaunchId.of("launch-id");
        String jobId = "do-prepare";

        FlowFullId flowFullId = FlowFullId.ofFlowProcessId(ciProcessId);
        return FlowLaunchContext.builder()
                .flowStarted(NOW)
                .flowId(flowFullId)
                .jobId(jobId)
                .jobStarted(NOW)
                .jobLaunchNumber(DEFAULT_LAUNCH_NUMBER)
                .flowLaunchId(flowLaunchId)
                .targetRevision(TestData.TRUNK_R2)
                .selectedBranch(TestData.RELEASE_BRANCH_2)
                .launchId(LaunchId.of(ciProcessId, 100500))
                .launchInfo(LaunchInfo.of(Version.majorMinor("103", "17")))
                .sandboxOwner("CI")
                .yavTokenUid(YavToken.Id.of("tkn-13"))
                .triggeredBy(TestData.USER42)
                .title("Тест #1")
                .runtimeConfig(RuntimeConfig.of(
                        RuntimeSandboxConfig.builder()
                                .hint("Custom hint")
                                .tags(List.of("USER_CUSTOM_TAG", "", "Another user_custom Tag"))
                                .build()
                ))
                .launchPullRequestInfo(new LaunchPullRequestInfo(
                        TestData.TRUNK_R2.getPullRequestId(),
                        10L,
                        TestData.CI_USER,
                        "Some Title",
                        "Some Description",
                        ArcanumMergeRequirementId.of("system", "type"),
                        new PullRequestVcsInfo(
                                ArcRevision.of("merge_rev"),
                                ArcRevision.of("upstream_rev"),
                                ArcBranch.ofBranchName("upstream_branch"),
                                ArcRevision.of("feature_rev"),
                                ArcBranch.ofBranchName("feature_branch")
                        ),
                        List.of("CI-1", "CI-2"),
                        List.of("label1", "label2"),
                        null
                ))
                .projectId("some-project-id")
                .targetCommit(TestData.TRUNK_COMMIT_2);
    }

    private static List<SandboxTaskStatus> statusesWithContinueExecution() {
        return Stream.of(SandboxTaskStatus.values())
                .filter(task -> task.getTaskGroupStatus() == SandboxTaskGroupStatus.EXECUTE ||
                        task.getTaskGroupStatus() == SandboxTaskGroupStatus.QUEUE ||
                        task.getTaskGroupStatus() == SandboxTaskGroupStatus.DRAFT ||
                        task.getTaskGroupStatus() == SandboxTaskGroupStatus.WAIT)
                .toList();
    }

    private static List<SandboxTaskStatus> statusesWithRestart() {
        return Stream.of(SandboxTaskStatus.values())
                .filter(task -> task.getTaskGroupStatus() == SandboxTaskGroupStatus.BREAK)
                .toList();
    }

    private static List<SandboxTaskStatus> statusesWithRecreate() {
        return Stream.of(SandboxTaskStatus.values())
                .filter(task -> task.getTaskGroupStatus() == SandboxTaskGroupStatus.FINISH)
                .toList();
    }

}
