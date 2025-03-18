package ru.yandex.ci.engine.tasks.tracker;

import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import ci.tracker.create_issue.CreateIssueOuterClass;
import com.google.common.base.Suppliers;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.text.StringSubstitutor;
import org.apache.commons.text.lookup.StringLookup;

import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.Ticket;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.TimelineCommit;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.utils.UrlService;

@Slf4j
@AllArgsConstructor
public class TrackerIssuesLinker {
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter
            .ofPattern("yyyy-MM-dd HH:mm")
            .withZone(ZoneId.systemDefault());

    @Nonnull
    private final CiDb db;

    @Nonnull
    private final UrlService urlService;

    @Nonnull
    private final CommitFetchService commitFetchService;

    public IssuesLinker getIssuesLinker(CreateIssueOuterClass.Config config, FlowLaunchContext flowLaunchContext) {
        return new IssuesLinker(config, flowLaunchContext);
    }

    public static <T> Map<String, T> toMap(List<T> values, Function<T, String> getValue) {
        return values.stream()
                .collect(Collectors.toMap(
                        getValue,
                        Function.identity(),
                        (o1, o2) -> o1
                ));
    }

    @AllArgsConstructor
    public class IssuesLinker {

        private final CreateIssueOuterClass.Config config;
        private final FlowLaunchContext flowLaunchContext;
        private final DescriptionRenderLookup descriptionLookup = new DescriptionRenderLookup();

        public DescriptionRenderLookup getDescriptionLookup() {
            return descriptionLookup;
        }

        @AllArgsConstructor
        public class DescriptionRenderLookup implements StringLookup {
            public static final String PREFIX = "{{";
            public static final String SUFFIX = "}}";
            public static final char ESCAPE = '\\';

            private final Supplier<List<TimelineCommit>> getCommits = Suppliers.memoize(this::fetchCommits);
            private final Supplier<TreeSet<Ticket>> getAllIssues = Suppliers.memoize(this::fetchIssues);

            public TreeSet<Ticket> getAllIssues() {
                return getAllIssues.get();
            }

            public StringSubstitutor getSubstitutor() {
                return new StringSubstitutor(this, PREFIX, SUFFIX, ESCAPE);
            }

            @Override
            public String lookup(String key) {
                return switch (key) {
                    case "revision" -> renderRevision();
                    case "issues" -> renderIssues();
                    case "commits" -> renderCommits();
                    default -> key;
                };
            }

            private String renderRevision() {
                return getRevisionReference(flowLaunchContext.getTargetRevision());
            }

            private TreeSet<Ticket> fetchIssues() {
                var acceptQueues = Set.copyOf(config.getLink().getQueuesList());
                log.info("Filter issue using accept queue list: {}", acceptQueues);

                var commits = getCommits.get();

                var comparator = Comparator.comparing(Ticket::getQueue).thenComparing(Ticket::getKey);
                var issues = new TreeSet<>(comparator);
                for (var commit : commits) {
                    var dbCommit = commit.getCommit().getCommit();
                    for (var issue : ArcCommitUtils.parseTicketsFully(dbCommit.getMessage())) {
                        if (acceptQueues.isEmpty() || acceptQueues.contains(issue.getQueue())) {
                            log.info("Accept issue: {}", issue);
                            issues.add(issue);
                        } else {
                            log.info("Skip issue: {}", issue);
                        }
                    }
                }
                return issues;
            }

            private String renderIssues() {
                var text = new StringBuilder(1024);
                for (var ticket : getAllIssues.get()) {
                    text.append("- ").append(ticket.getKey()).append("\n");
                }
                return text.toString();
            }

            private String getRevisionReference(OrderedArcRevision revision) {
                var url = revision.hasSvnRevision()
                        ? urlService.toSvnRevision(revision)
                        : urlService.toArcCommit(revision);
                var text = revision.hasSvnRevision()
                        ? ("r" + revision.getNumber())
                        : revision.getCommitId();
                return "((" + url + " " + text + "))";
            }

            private String renderCommits() {
                var commits = getCommits.get();
                var text = new StringBuilder(1024);
                for (var commit : commits) {
                    var dbCommit = commit.getCommit().getCommit();
                    text.append("- ").append(FORMATTER.format(dbCommit.getCreateTime()));
                    text.append(", ").append(getRevisionReference(commit.getCommit().getRevision()));
                    text.append(", кто:").append(dbCommit.getAuthor());
                    text.append("\n").append("<[").append(dbCommit.getMessage()).append("]>");
                    text.append("\n");
                }
                return text.toString();
            }

            private List<TimelineCommit> fetchCommits() {
                var launchId = flowLaunchContext.getLaunchId();
                var offset = CommitFetchService.CommitOffset.empty();
                var limit = config.getLink().getMaxCommits();
                var includeFromActiveReleases = switch (config.getLink().getType()) {
                    case FROM_PREVIOUS_ACTIVE -> false;
                    case FROM_PREVIOUS_STABLE, UNRECOGNIZED -> true;
                };
                var result = db.currentOrReadOnly(() ->
                        commitFetchService.fetchTimelineCommits(launchId, offset, limit, includeFromActiveReleases));
                return result.items();
            }
        }
    }
}
