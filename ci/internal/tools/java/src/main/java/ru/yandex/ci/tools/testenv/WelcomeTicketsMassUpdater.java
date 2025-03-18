package ru.yandex.ci.tools.testenv;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.function.Predicate;
import java.util.function.Supplier;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import javax.annotation.PostConstruct;

import com.google.common.base.Suppliers;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.engine.spring.clients.TrackerClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.OurGuys;
import ru.yandex.startrek.client.Session;
import ru.yandex.startrek.client.model.CollectionUpdate;
import ru.yandex.startrek.client.model.Comment;
import ru.yandex.startrek.client.model.CommentCreate;
import ru.yandex.startrek.client.model.Issue;
import ru.yandex.startrek.client.model.IssueUpdate;
import ru.yandex.startrek.client.model.UserRef;

import static java.util.function.Predicate.not;

@SuppressWarnings({"SameParameterValue", "UnusedReturnValue", "unused"})
@Slf4j
@Configuration
@Import(TrackerClientConfig.class)
public class WelcomeTicketsMassUpdater extends AbstractSpringBasedApp {

    private static final String NOT_CLOSED = "Queue: CIWELCOME AND Status: !Закрыт";
    private static final String DOCS_SUMMONED_TAG = "docs-summoned";

    private static final Pattern PATH_TO_DOCS_PATTERN = Pattern.compile(
            "Куда положить файл `a.yaml` в аркадии: `([^`]+)`"
    );

    @Autowired
    private TrackerClient client;

    // https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b
    @Value("${ci.startrek.oauth}")
    private String trackerToken;

    private Session tracker;

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

    @PostConstruct
    public void setup() {
        tracker = client.getSession(trackerToken);
    }

    @Override
    protected void run() throws Exception {
        var openedDocsIssues = openedDocs();
        int n = 0;
        for (var openedDocsIssue : openedDocsIssues) {
            summonToDocs(openedDocsIssue);
            log.info("{} of {}", ++n, openedDocsIssues.size());
        }
    }

    private List<IssueData> toDelete11April() {
        var issues = issues(NOT_CLOSED).stream()
                .filter(
                        withLastComment(
                                havingMessage("база будет полностью удалена").and(from("pochemuto"))
                        )
                )
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));

        return issues;
    }

    private List<IssueData> openedDocs() {
        var issues = issues(NOT_CLOSED + " AND Tags: docs AND Tags: !" + DOCS_SUMMONED_TAG).stream()
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));

        return issues;
    }

    private void summonToDocs(IssueData issueData) {
        var issueId = issueData.getIssue().getKey();
        log.info("Summoning to {}", issueId);

        var commentWithSummon = issueData.getCommentsFromLastToFirst().stream()
                .filter(havingSummon(not(OurGuys::isOurGuy)))
                .findFirst();

        if (commentWithSummon.isEmpty()) {
            throw new RuntimeException("Not found comment with summon, issue " + issueData.getIssue().getKey());
        }

        var users = commentWithSummon.get().getSummonees().stream()
                .map(UserRef::getLogin)
                .filter(not(OurGuys::isOurGuy))
                .toList();

        var paths = PATH_TO_DOCS_PATTERN.matcher(issueData.getIssue().getDescription().get())
                .results()
                .map(r -> r.group(1))
                .map(path -> "`" + path + "`")
                .collect(Collectors.joining(" и "));

        var job = issueData.getIssue().getTags()
                .stream()
                .filter(t -> t.startsWith("docs:"))
                .map(t -> StringUtils.removeStart(t, "docs:"))
                .findFirst()
                .orElseThrow(() -> new RuntimeException("cannot get db in " + issueId));

        var message = """
                Мы понимаем, что сейчас тяжелая ситуация и готовы немного подождать, но точно не готовы ждать вечно,\
                 а поддержка ТЕ расходует много нашего времени и ресурсов.

                Hard deadline по этому проекту - 1.5 месяца. То есть 6 августа публикация документации\
                 через TestEnv будет остановлена.
                Пожалуйста, отреагируйте на этот тикет. Если вы не можете найти время по каким-либо причинам,\
                 приходите к нам, мы что-нибудь придумаем.

                Мы подготовили для вас утилиту, которая поможет вам быстрее мигрировать\
                 `ya project create docs --rewrite`. Она проведет вас по шагам и сгенерирует a.yaml.\
                 Задача займет 10 минут, в случае если у вас уже есть робот. Если его нет,\
                 ((https://wiki.yandex-team.ru/tools/support/zombik/ заведите его заранее)), выполнение заявки\
                 занимает некоторое время.

                Чтобы воспользоваться утилитой, выполните в чистом репозитории\
                 в папке %3$s команды:
                ```bash
                arc pull && arc checkout -b %1$s
                ya project create docs --rewrite
                # ответьте на все 6 вопросов. Если какие-то будут не понятны, приходите в тикет или личку
                arc add a.yaml # важно закоммитить только a.yaml, потому что --rewrite перепишет в том числе и\
                 другие файлы документации
                arc commit -m '%1$s migrate %2$s'
                arc pr create --push --publish --merge
                ```
                                
                <{пример работы утилиты
                ```
                @yandex arc:(trunk) ~/arcadia/junk/pochemuto/test-doc ✗ ya project create docs --rewrite
                Docs project name will be used in url link
                [1/6] Enter docs project name: ciwelcome
                                
                ABC service slug you can get from url of your service
                For instance https://abc.yandex-team.ru/services/meta_infra/ has meta_infra abc slug
                Users from this service will have permissions to deploy docs and modify generated a.yaml\
                 without confirmation
                [2/6] Enter ABC service: cidemo
                                
                Documentation deployment will be executed via your robot. If you don't have robot yet create it\
                 https://wiki.yandex-team.ru/tools/support/zombik/
                Obtain oauth token for your robot using link\
                 https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5c2eb9ec7cc74dcd960f400ff32b7b38
                Make sure you logged as a robot when obtaining token!
                Store the token in a yav secret https://yav.yandex-team.ru/ with the key ci.token.\
                 Grant robot read access to the secret.
                These steps are described in details here https://docs.yandex-team.ru/ci/quick-start-guide#get-token
                YAV secret has 'sec-XXXX..' format
                [3/6] Enter YAV secret where ci.token is stored: sec-01e8agdtdcs61v6emr05h5q1ek
                                
                Deployment process uses sandbox group's quota. Default quota will be more than enough.
                If you don't have sandbox group yet create it https://sandbox.yandex-team.ru/admin/groups
                All created sandbox groups already have default quota.
                Add your robot to the sandbox group.
                [4/6] Enter Sandbox group: CI
                                
                When documentation files changed in PR ci builds it and deploys to the temporary environment.
                This option allows adding link to the deployed documentation in PR comments
                [5/6] Comment pr with a link to the deployed doc [Y/n]: y
                                
                When documentation have changes in trunk ci deploys it to testing environment and waits\
                 confirmation to deploy to stable.
                Users from ABC-service have to click button in CI interface to deploy docs in stable.
                [6/6] Confirm deploy to stable [Y/n]: y
                                
                Docs for your project is ready, for details go to https://docs.yandex-team.ru/docstools
                After merging generated a.yaml to trunk documentation release will be accessible via link:\
                 https://a.yandex-team.ru/projects/cidemo/ci/releases/timeline\
                ?dir=junk%%2Fpochemuto%%2Ftest-doc&id=release-docs
                Have fun:)
                ```
                }>
                """.formatted(issueId, job, paths);

        var comment = CommentCreate.builder()
                .comment(message)
                .summonees(users.toArray(String[]::new))
                .build();

        log.info(message);
        var description = issueData.getIssue().getDescription().get();
        var issueUpdate = IssueUpdate
                .deadline(org.joda.time.LocalDate.parse("2022-07-04"))
                .tags(Cf.list(DOCS_SUMMONED_TAG), Cf.list())
                .update("followers", CollectionUpdate.add("arivkin"))
                .build();

        tracker.comments().create(issueId, comment);
        tracker.issues().update(issueId, issueUpdate);

        log.info("Done {} {}", issueData.getIssue().getKey(), users);
    }

    private List<IssueData> hardDeadlineWithoutReaction() {
        var issues = issues(NOT_CLOSED).stream()
                .filter(
                        withLastComment(
                                havingMessage("Hard deadline по этому проекту - 2 месяца.").and(from("pochemuto"))
                        )
                )
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));

        return issues;
    }

    private void finalCallWithReaction() {
        var issues = issues(NOT_CLOSED).stream()
                .filter(
                        withCommentAndAfterIt(
                                havingMessage("Ваша база будет остановлена завтра").and(from("pochemuto")),
                                anyComment(not(from("pochemuto")))
                        )
                )
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));
    }

    private void finalCallWithoutReaction() {
        var issues = issues(NOT_CLOSED).stream()
                .filter(
                        withCommentAndAfterIt(
                                havingMessage("Ваша база будет остановлена завтра").and(from("pochemuto")),
                                allComments(from("pochemuto"))
                        )
                )
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));
    }

    private void databaseStopped() {
        var issues = issues(NOT_CLOSED).stream()
                .filter(
                        withLastComment(havingMessage("База остановлена").and(from("pochemuto")))
                )
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));
    }

    private void databaseWillBeStopped() {
        var issues = issues(NOT_CLOSED).stream()
                .filter(
                        withLastComment(
                                havingMessage("Отсутствие активности в тикете будет означать," +
                                        " что проект брошен и не используется.")
                                        .and(from("pochemuto")))
                )
                .toList();

        log.info("Found {} issues:\n{}", issues.size(), issueLogFrom(issues));
    }

    private static String issueLogFrom(Collection<IssueData> issues) {
        return issues.stream()
                .map(IssueData::getIssue)
                .map(i -> "https://st.yandex-team.ru/%-15s %s".formatted(i.getKey(), i.getSummary()))
                .collect(Collectors.joining("\n"));
    }

    private List<IssueData> issues(String query) {
        var issues = tracker.issues().find(query).toList();
        return issues.stream()
                .map(IssueData::new)
                .toList();
    }

    public static Predicate<List<Comment>> allComments(Predicate<Comment> predicate) {
        return comments -> comments.stream().allMatch(predicate);
    }

    public static Predicate<List<Comment>> anyComment(Predicate<Comment> predicate) {
        return comments -> comments.stream().anyMatch(predicate);
    }

    public static Predicate<Comment> havingMessage(String text) {
        return comment -> comment.getText().map(m -> m.contains(text)).orElse(false);
    }

    public static Predicate<Comment> havingSummon() {
        return comment -> !comment.getSummonees().isEmpty();
    }

    public static Predicate<Comment> havingSummon(Predicate<String> summonedPredicate) {
        return comment -> comment.getSummonees().stream().map(UserRef::getLogin).anyMatch(summonedPredicate);
    }

    public static Predicate<Comment> from(String author) {
        return comment -> comment.getCreatedBy().getLogin().equals(author);
    }

    public static Predicate<IssueData> withLastComment(Predicate<Comment> commentPredicate) {
        return issueData -> {
            var comments = issueData.getComments();
            return comments.size() > 0 && commentPredicate.test(comments.get(comments.size() - 1));
        };
    }

    public static Predicate<IssueData> withComment(Predicate<Comment> commentPredicate) {
        return issueData -> issueData.getComments().stream().anyMatch(commentPredicate);
    }

    public static Predicate<IssueData> withCommentAndAfterIt(Predicate<Comment> commentPredicate,
                                                             Predicate<List<Comment>> commentsAfter) {
        return issueData -> {
            var comments = issueData.getComments();
            Integer last = null;
            for (int i = 0; i < comments.size(); i++) {
                var current = comments.get(i);
                if (commentPredicate.test(current)) {
                    last = i;
                }
            }
            if (last == null) {
                return false;
            }

            return commentsAfter.test(comments.subList(last + 1, comments.size()));
        };
    }

    private static class IssueData {
        @Getter
        private final Issue issue;

        private final Supplier<List<Comment>> comments;

        public IssueData(Issue issue) {
            this.issue = issue;
            this.comments = Suppliers.memoize(() -> issue.getComments().toList());
        }

        public List<Comment> getComments() {
            return comments.get();
        }

        public List<Comment> getCommentsFromLastToFirst() {
            var commentList = new ArrayList<>(comments.get());
            Collections.reverse(commentList);
            return commentList;
        }
    }
}

