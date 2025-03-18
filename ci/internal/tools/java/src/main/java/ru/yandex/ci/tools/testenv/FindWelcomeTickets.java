package ru.yandex.ci.tools.testenv;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import com.beust.jcommander.Parameter;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVParser;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.engine.spring.clients.TrackerClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.OurGuys;
import ru.yandex.startrek.client.model.Comment;
import ru.yandex.startrek.client.model.CommentCreate;
import ru.yandex.startrek.client.model.Issue;
import ru.yandex.startrek.client.model.IssueUpdate;
import ru.yandex.startrek.client.model.Ref;
import ru.yandex.startrek.client.model.SearchRequest;
import ru.yandex.startrek.client.model.UserRef;

import static java.util.function.Predicate.not;

@Slf4j
@Configuration
@Import(TrackerClientConfig.class)
public class FindWelcomeTickets extends AbstractSpringBasedApp {

    @Autowired
    private TrackerClient client;

    @Value("${ci.startrek.oauth}")
    private String trackerConfig;

    @Parameter(names = "--issues-output")
    private Path issuesOutputPath;

    @Parameter(names = "--to-summon")
    private Path toSummonPath;

    @Parameter(names = "--do")
    private boolean doSummon;

    @Parameter(names = "--do-update-description")
    private boolean doUpdateDescription;

    private static final Set<String> FORMER_RESPONSIBLE = Set.of(
            "robot-ci",
            "fizzzgen"
    );

    @Override
    protected void run() throws IOException {
        summonToIssues();
//        exportIssues();
    }

    @SuppressWarnings("unused")
    private void summonToIssues() throws IOException {
        var comment = """
                Привет. Testenv [неминуемо закрывается](https://clubs.at.yandex-team.ru/testenv/118).\
                 Мы считаем, что ваш проект (база) готов к переезду в CI.

                Если у вас есть сложности с переездом, пожалуйста, сообщите нам об этом, мы с удовольствием\
                 вам поможем мигрировать в CI. В случае, если вы считаете, что чего-либо не хватает в текущем CI,\
                 напишите об этом в тикете. Если проект уже не нужен, также сообщите об этом, мы её просто удалим.

                Пожалуйста, отреагируйте на этот тикет. Отсутствие активности в тикете будет означать,\
                 что проект брошен и не используется. Мы остановим его 28 февраля. В любом случае при отсутствии\
                 объективных блокеров для переезда в CI крайний срок для переезда — 28 марта.\
                 После этого проект будет удален.

                По вопросам можете писать в тикет или в личку кому:pochemuto.
                                """;
        var toSummons = parseToSummon();

        log.info("Going to create {} comments", toSummons.size());
        var session = client.getSession(trackerConfig);
        for (var toSummon : toSummons) {
            var summonees = toSummon.getLogins().toArray(new String[0]);
            var commentCreate = CommentCreate.builder()
                    .comment(comment)
                    .summonees(summonees)
                    .build();

            var issueId = toSummon.getTicket();
            if (doUpdateDescription) {
                log.info("Updating description for {}", issueId);

                var originalMessage = """
                        Призывайте в тикет кого:fizzzgen, кого:miroslav2, кого:andreevdm или\
                         ((https://docs.yandex-team.ru/ci/#support приходите к нам в один из каналов поддержки)).
                        Мы с удовольствием поможем.""";
                var updatedMessage = """
                        Призывайте в тикет кого:pochemuto. Мы с удовольствием поможем.""";

                var issue = session.issues().get(issueId);
                var originalDescription = issue.getDescription().orElse("");
                var description = originalDescription.replace(originalMessage, updatedMessage);
                if (!description.equals(originalDescription)) {
                    var issueUpdate = IssueUpdate.description(description).build();
                    issue.update(issueUpdate);
                } else {
                    log.info("Not found original message to update");
                }
            }

            log.info("Creating comment for {} with summonees {}", issueId, summonees);
            if (doSummon) {
                session.comments().create(issueId, commentCreate);

                var issue = session.issues().get(issueId);
                var newTags = issue
                        .getTags()
                        .plus("summoned-1");
                var issueUpdate = IssueUpdate.tags(newTags).build();
                issue.update(issueUpdate);
            }
        }
    }

    @SuppressWarnings("unused")
    private void exportIssues() throws IOException {
        var issues = fetchIssues();
        log.info("Found {} issues", issues.size());

        exportIssues(issues);
    }

    private void exportIssues(List<IssueData> issues) throws IOException {
        var csv = CSVFormat.DEFAULT;
        var sb = new StringBuilder();
        sb.append(csv.format(
                "name",
                "ticket",
                "status",
                "resolved",
                "summoned",
                "replied",
                "followers",
                "linkedToCi"
        )).append("\n");
        for (IssueData issue : issues) {
            sb.append(csv.format(
                    issue.getTestenvDb(),
                    issue.getKey(),
                    issue.getStatus(),
                    boolCsv(issue.isResolved()),
                    boolCsv(issue.isHasSummon()),
                    boolCsv(issue.isHasReply()),
                    boolCsv(issue.isHasFollowers()),
                    boolCsv(issue.isHasLinkToCi())
            )).append("\n");
        }
        Files.writeString(issuesOutputPath, sb);

        log.info("Issues dumped to {}", issuesOutputPath);
    }

    private int boolCsv(boolean value) {
        return value ? 1 : 0;
    }

    private List<IssueData> fetchIssues() {
        var session = client.getSession(trackerConfig);

        var ciwelcome = session.issues().find(SearchRequest.builder()
                .queue("CIWELCOME")
                .build());

        var issues = new ArrayList<IssueData>();

        while (ciwelcome.hasNext()) {
            var issue = ciwelcome.next();
            if (issue.getResolution().map(Ref::getDisplay).containsTs("Дубликат")) {
                log.info("Skip {} is duplicate", issue.getKey());
                continue;
            }
            issues.add(checkIssue(issue));
        }
        return issues;
    }

    private IssueData checkIssue(Issue issue) {
        log.info("Checking issue {}", issue.getKey());

        var comments = issue.getComments();
        var hasFollowers = issue.getFollowers()
                .stream()
                .map(UserRef::getLogin)
                .anyMatch(not(FindWelcomeTickets::isOurGuy));

        var hasSummon = false;
        var hasReply = false;
        while (comments.hasNext()) {
            Comment comment = comments.next();
            hasSummon |= isOurGuy(comment.getCreatedBy().getLogin())
                    && comment.getSummonees()
                    .stream()
                    .map(UserRef::getLogin)
                    .anyMatch(not(FindWelcomeTickets::isOurGuy));

            hasReply |= !isOurGuy(comment.getCreatedBy().getLogin());
        }

        var testenvDb = issue.getTags().stream()
                .filter(t -> t.startsWith("DB:"))
                .map(t -> t.substring("DB:".length()))
                .findFirst()
                .orElse(null);

        var hasLinkToCi = issue.getLinks()
                .stream()
                .map(l -> l.getObject().getKey())
                .anyMatch(key -> key.startsWith("CI-"));

        return new IssueData(issue.getKey(),
                issue.getStatus().getDisplay(),
                issue.getResolution().isPresent(),
                testenvDb,
                hasSummon,
                hasReply,
                hasFollowers,
                hasLinkToCi
        );
    }

    private List<ToSummon> parseToSummon() throws IOException {
        var parsed = CSVParser.parse(toSummonPath, StandardCharsets.UTF_8, CSVFormat.DEFAULT.withFirstRecordAsHeader());
        var header = parsed.getHeaderMap();
        return parsed.getRecords().stream()
                .map(r -> new ToSummon(
                        r.get(header.get("name")),
                        r.get(header.get("ticket")),
                        r.get(header.get("status")),
                        r.get(header.get("is_rm")).equals("1"),
                        r.get(header.get("followed")).equals("1"),
                        Arrays.stream(r.get(header.get("logins")).split(",")).toList()
                ))
                .toList();
    }

    private static boolean isOurGuy(String login) {
        return OurGuys.isOurGuy(login) || FORMER_RESPONSIBLE.contains(login);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

    @lombok.Value
    private static class IssueData {
        String key;
        String status;
        boolean resolved;
        @Nullable
        String testenvDb;
        boolean hasSummon;
        boolean hasReply;
        boolean hasFollowers;
        boolean hasLinkToCi;
    }

    @lombok.Value
    private static class ToSummon {
        String name;
        String ticket;
        String status;
        boolean rm;
        boolean followed;
        List<String> logins;
    }

}
