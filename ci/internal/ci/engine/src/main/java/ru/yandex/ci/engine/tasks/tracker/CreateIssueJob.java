package ru.yandex.ci.engine.tasks.tracker;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import ci.tracker.create_issue.CreateIssueOuterClass;
import ci.tracker.create_issue.CreateIssueOuterClass.Rules.OnDuplicate;
import com.google.common.base.Preconditions;
import com.google.common.base.Suppliers;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.ListF;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.startrek.client.Session;
import ru.yandex.startrek.client.model.ChecklistItem;
import ru.yandex.startrek.client.model.Component;
import ru.yandex.startrek.client.model.Issue;
import ru.yandex.startrek.client.model.IssueCreate;
import ru.yandex.startrek.client.model.IssueRef;
import ru.yandex.startrek.client.model.IssueType;
import ru.yandex.startrek.client.model.SearchRequest;
import ru.yandex.startrek.client.model.ServiceRef;
import ru.yandex.startrek.client.model.Version;
import ru.yandex.startrek.client.model.VersionCreate;

// TODO: Temporary, until supporting Java tasklets in Sandbox
@Slf4j
@RequiredArgsConstructor
@ExecutorInfo(
        title = "Create ticket in Yandex Tracker",
        description = "Создание релизного тикета в Трекере"
)
@Consume(name = "config", proto = CreateIssueOuterClass.Config.class)
@Consume(name = "rules", proto = CreateIssueOuterClass.Rules.class)
@Consume(name = "template", proto = CreateIssueOuterClass.Template.class)
@Consume(name = "update_template", proto = CreateIssueOuterClass.UpdateTemplate.class)
@Produces(single = {CreateIssueOuterClass.Issue.class, CreateIssueOuterClass.Config.class})
public class CreateIssueJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("03909df8-bf4d-40ad-a103-7dea7a732dd3");

    private final TrackerSessionSource trackerSessionSource;
    private final TrackerIssuesLinker trackerIssuesLinker;
    private final AbcService abcService;

    @Override
    public void execute(JobContext context) throws Exception {
        var config = context.resources().consume(CreateIssueOuterClass.Config.class);
        var rules = context.resources().consume(CreateIssueOuterClass.Rules.class);
        var template = context.resources().consume(CreateIssueOuterClass.Template.class);
        var updateTemplate = context.resources().consume(CreateIssueOuterClass.UpdateTemplate.class);

        log.info("Creating issue using config: {}", config);
        log.info("Using rules: {}", rules);
        log.info("Using template: {}", template);
        log.info("Using update template: {}", updateTemplate);

        var flowLaunchContext = context.createFlowLaunchContext();

        var session = trackerSessionSource.getSession(config.getSecret(), flowLaunchContext.getYavTokenUid());
        var linker = trackerIssuesLinker.getIssuesLinker(config, flowLaunchContext);

        new IssueProcessor(
                context,
                config,
                rules,
                template,
                updateTemplate,
                session,
                linker
        ).process();
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    protected void produceResources(
            JobContext context,
            CreateIssueOuterClass.Config config,
            CreateIssueOuterClass.Issue issue
    ) {
        // Emulate tasklet output
        // See ci/tasklet/registry/common/tracker/create_issue/proto/create_issue.proto#L150
        context.resources().produce(Resource.of(issue, "issue"));
        context.resources().produce(Resource.of(config, "config"));
    }

    static void sendIssueBadge(
            TrackerSessionSource trackerSessionSource,
            JobContext context,
            CreateIssueOuterClass.Issue issue
            ) {
        var url = "%s/%s".formatted(trackerSessionSource.getTrackerUrl(), issue.getIssue());
        var taskBadge = TaskBadge.of(issue.getIssue(), "STARTREK", url, TaskBadge.TaskStatus.SUCCESSFUL);
        context.progress().updateTaskState(taskBadge);
    }

    static CreateIssueOuterClass.Issue toProtoIssue(String key, boolean isNew) {
        return CreateIssueOuterClass.Issue.newBuilder().setIssue(key).setIsNew(isNew).build();
    }


    @AllArgsConstructor
    private class IssueProcessor {
        private final JobContext context;
        private final CreateIssueOuterClass.Config config;
        private final CreateIssueOuterClass.Rules rules;
        private final CreateIssueOuterClass.Template template;
        private final CreateIssueOuterClass.UpdateTemplate updateTemplate;
        private final Session session;
        private final TrackerIssuesLinker.IssuesLinker linker;

        void process() {
            var version = lookupVersion();

            var onDuplicate = rules.getOnDuplicate();
            if (onDuplicate == OnDuplicate.FAIL || onDuplicate == OnDuplicate.UPDATE) {
                if (template.getFixVersion().isEmpty()) {
                    throw new RuntimeException("template.fix_version is mandatory when template.on_duplicate is " +
                            rules.getOnDuplicate());
                }
                if (version != null) {
                    var foundIssue = lookupIssue(version);
                    if (foundIssue.isPresent()) {
                        if (onDuplicate == OnDuplicate.FAIL) {
                            throw new RuntimeException(String.format("Found duplicate issue for version %s: %s",
                                    version.getName(), foundIssue.get().getKey()));
                        } else {
                            updateIssue(foundIssue.get());
                            return; // ---
                        }
                    }
                }
            }
            createIssue(version);
        }

        @Nullable
        private Version lookupVersion() {
            if (template.getFixVersion().isEmpty()) {
                return null;
            }

            // TODO: use suggest
            //  v2/versions/_suggest?input=versionname&queue=TEST,STARTREK
            var versions = session.versions().getAll(template.getQueue()).toList();
            versions.forEach(value -> log.info("{}", value));

            var versionMap = TrackerIssuesLinker.toMap(versions, Version::getName);
            return versionMap.get(template.getFixVersion());
        }

        private IssueType getType() {
            var types = session.types().getAll(template.getQueue()).toList();
            types.forEach(value -> log.info("{}", value));

            var typeMap = TrackerIssuesLinker.toMap(types, IssueType::getName);
            var type = typeMap.get(template.getType());
            Preconditions.checkState(type != null,
                    "Unable to find issue with type %s", template.getType());
            return type;
        }

        private Optional<Issue> lookupIssue(Version version) {
            var type = getType();

            var request = SearchRequest.builder()
                    .filter("queue", template.getQueue())
                    .filter("fixVersions", version.getId())
                    .filter("type", type.getId())
                    .build();
            log.info("Searching existing issue with request: {}", request);
            var issues = session.issues().find(request);
            if (issues.hasNext()) {
                var issue = issues.next();
                log.info("Matched first issue: {}", issue);
                return Optional.of(issue);
            } else {
                log.info("No issues matched");
                return Optional.empty();
            }
        }

        private void updateIssue(IssueRef issueRef) {
            var protoIssue = toProtoIssue(issueRef.getKey(), false);
            sendBadge(protoIssue);

            var updater = new LinkedIssuesUpdater(linker, session);
            updater.updateIssue(updateTemplate, issueRef);
            updater.updateAndTransitLinkedIssues(updateTemplate);

            produceResources(protoIssue);
        }

        private void createIssue(@Nullable Version version) {
            var issue = new IssueBuilder(createVersionIfNeeded(version)).createIssue();
            log.info("Issue to register: {}", issue);
            log.info("Links: {}", issue.getLinks());

            var newIssue = session.issues().create(issue);
            log.info("Created issue: {}", newIssue);

            var protoIssue = toProtoIssue(newIssue.getKey(), true);
            sendBadge(protoIssue);

            new LinkedIssuesUpdater(linker, session).updateAndTransitLinkedIssues(template);

            produceResources(protoIssue);
        }

        private void sendBadge(CreateIssueOuterClass.Issue issue) {
            sendIssueBadge(trackerSessionSource, context, issue);
        }

        private void produceResources(CreateIssueOuterClass.Issue issue) {
            CreateIssueJob.this.produceResources(context, config, issue);
        }

        @Nullable
        private Version createVersionIfNeeded(@Nullable Version version) {
            if (template.getFixVersion().isEmpty()) {
                Preconditions.checkState(version == null, "Version must be null but %s", version);
                return null;
            }
            if (version != null) {
                return version;
            }
            var queue = session.queues().get(template.getQueue());
            var newFixVersion = new VersionCreate.Builder()
                    .queue(queue)
                    .name(template.getFixVersion())
                    .build();
            log.info("Version to create: {}", newFixVersion);

            var newVersion = session.versions().create(newFixVersion);
            log.info("Created fix version: {}", newVersion);

            return newVersion;
        }


        @AllArgsConstructor
        private class IssueBuilder {
            @Nullable
            private final Version version;
            private final IssueCreate.Builder builder = IssueCreate.builder();
            private final Supplier<Map<String, Component>> getComponents = Suppliers.memoize(() -> {
                var components = session.components().getAll(template.getQueue()).toList();
                components.forEach(value -> log.info("{}", value));
                return TrackerIssuesLinker.toMap(components, Component::getName);
            });

            IssueCreate createIssue() {
                set(template::getQueue, builder::queue);
                set(template::getType, this::type);
                set(template::getSummary, builder::summary);
                set(template::getDescription, this::description);
                set(template::getAssignee, builder::assignee);
                set(template::getPriority, builder::priority);
                setList(template::getFollowersList, builder::followers);
                setList(template::getTagsList, builder::tags);
                set(template::getParent, builder::parent);
                set(template::getEpic, builder::epic);
                setListF(template::getChecklistList, this::checklist, builder::checklistItems);
                setListF(template::getComponentsList, this::component, builder::components);
                if (version != null) {
                    builder.fixVersions(version);
                }
                var services = abcServices();
                if (!services.isEmpty()) {
                    builder.set("abcService", services);
                }
                return builder.build();
            }

            private List<ServiceRef> abcServices() {
                if (template.getAbcServicesList().isEmpty()) {
                    return List.of();
                }

                var bySlug = new ArrayList<String>();
                var byId = new ArrayList<Long>();
                for (var item : template.getAbcServicesList()) {
                    try {
                        var id = Long.parseLong(item);
                        byId.add(id);
                    } catch (NumberFormatException nfe) {
                        bySlug.add(item);
                    }
                }

                if (!bySlug.isEmpty()) {
                    var map = abcService.getServices(bySlug);
                    for (var slug : bySlug) {
                        var project = map.get(slug);
                        Preconditions.checkState(project != null,
                                "Unable to find project by slug %s", slug);
                        byId.add((long) project.getId().getId());
                    }
                }

                return byId.stream()
                        .map(ServiceRef::new)
                        .toList();
            }

            private Component component(String component) {
                var components = getComponents.get();
                var matched = components.get(component);
                Preconditions.checkState(matched != null,
                        "Unable to find component %s", component);
                return matched;
            }

            private ChecklistItem checklist(CreateIssueOuterClass.Checklist item) {
                return new ChecklistItem.Builder()
                        .text(item.getText())
                        .checked(item.getChecked())
                        .build();
            }

            private void type(String value) {
                var issueType = session.types().getAll(template.getQueue()).stream()
                        .filter(type -> Objects.equals(value, type.getName()))
                        .findFirst()
                        .orElseThrow(() -> new RuntimeException("Unable to find issue type: " + value));
                builder.type(issueType);
            }

            private void description(String value) {
                builder.description(linker.getDescriptionLookup().getSubstitutor().replace(value));
            }

            private void set(Supplier<String> from, Consumer<String> to) {
                var value = from.get();
                if (!value.isEmpty()) {
                    to.accept(value);
                }
            }

            private void setList(Supplier<Collection<String>> from, Consumer<String[]> to) {
                var value = from.get();
                if (!value.isEmpty()) {
                    to.accept(value.toArray(new String[0]));
                }
            }

            private <S, T> void setListF(Supplier<List<S>> from, Function<S, T> mapper, Consumer<ListF<T>> to) {
                var value = from.get();
                if (!value.isEmpty()) {
                    var mapped = value.stream().map(mapper).toList();
                    to.accept(Cf.x(mapped));
                }
            }

        }
    }
}
