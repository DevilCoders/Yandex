package ru.yandex.ci.tools.potato;

import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;

public enum Handlers {
    ISSUE_CROWDTEST_INTEGRATION("issue-crowdtest-integration",
            "NOT_CI,st->testpalm,testpalm->st,hitman->st,st->hitman",
            "Запуск тестов в TestPalm и Hitman для задачи в Startrek"
    ),
    NOTIFICATIONS("notifications",
            "*->telegram",
            "Отправляет уведомления при наступлении указанного события."
    ),
    BITBUCKET_TEAMCITY_BUILD("bitbucket-teamcity-build",
            "CI,bb->teamcity,bb.review->teamcity",
            "Запуск сборки в TeamCity для пулл-реквеста в Bitbucket"
    ),
    PULL_REQUEST_ISSUE_STATUS_TRANSITION("pull-request-issue-status-transition",
            "st->review,review->st",
            "Алиас к PULL_REQUEST_ISSUE_STATUS, Cоздает билд-статус в пулл-реквесте и переводит его в нужное " +
                    "состояние при наступлении указанных переходов (статусов) Cтартрека"
    ),
    BITBUCKET_CUSTOM_REVIEW("bitbucket-custom-review",
            "NOT_CI, ARCANUM, bb->bb, review->review",
            "Данный обработчик реагирует на команды в комментариях к пулл-реквесту " +
                    "и создает билд-статус с результатами ревью."
    ),
    PULL_REQUEST_ISSUE_DESCRIPTION("pull-request-issue-description",
            "NOT_CI,ARCANUM,review->review",
            "создает комментарий к пулл-реквесту с названием и описанием задачи из Стартрека"
    ),
    BITBUCKET_MERGE_CONFLICTS("bitbucket-merge-conflicts",
            "NOT_CI,ARCANUM, bb->bb, review->review",
            "Обработчик реагирует на комментарии к пулл-реквесту и создает билд-статус," +
                    " отображающий наличие или отсутствие мердж-конфиликтов"
    ),
    ISSUE_TEAMCITY_BUILD_STATUS_COMMENT(
            "issue-teamcity-build-status-comment",
            "CI,teamcity->st",
            "Оставляет в связанной задаче комментарий с результатами сборки проекта в Teamcity"
    ),
    PULL_REQUEST_DELETE_MERGED_BRANCH(
            "pull-request-delete-merged-branch",
            "ARCANUM+,review->review",
            "удаляет ветку после того, как пулл-реквест был вмерджен в базовую ветку"
    ),
    MERGE_QUEUE("merge-queue",
            "ARCANUM+",
            "Сортирует открытые пулл-реквесты в порядке статуса и приоритета; делает доступным UI в Potato " +
                    "для просмотра очереди и поиску по ней. Опционально может автоматически " +
                    "сливать самый первый (наиболее успешный) пулл-реквест в очереди."
    ),
    PULL_REQUEST_ISSUE_ASSIGN_DEVELOPER(
            "pull-request-issue-assign-developer",
            "ARCANUM+,review->st",
            "При открытии пулл-реквеста в GitHub или Bitbucket обновляет поле \"Разработчик\" в связанной задаче " +
                    "Стартрека и указывает в данном поле автора пулл-реквеста. Также есть возможность обновить поле " +
                    "\"Разработчик\" в задачах, которые указаны в заголовке, описании и в сообщениях коммитов " +
                    "пулл-реквеста."
    ),
    ISSUE_SET_FIELDS("issue-set-fields",
            "CI,review->st",
            "позволяет автоматически заполнять поля задачи в Стартреке"
    ),
    PULL_REQUEST_RELATED_ISSUES("pull-request-related-issues",
            "ARCANUM",
            "ищет связанные с пулл-реквестом задачи Стартрека, " +
                    "и добавляет ссылки на задачи в комментарии к пулл-реквесту"
    ),
    PULL_REQUEST_ISSUE_STATUS(
            "pull-request-issue-status",
            "st->review,review->st",
            "Cоздает билд-статус в пулл-реквесте и переводит его в нужное состояние при наступлении указанных " +
                    "переходов (статусов) Cтартрека"
    ),
    BITBUCKET_TRACKER_INTEGRATION(
            "bitbucket-tracker-integration",
            "ARCANUM,bb->st,bb.review->st",
            "изменяет статус задачи в Стартреке при открытии пулл-реквеста в Bitbucket и отправляет " +
                    "комментарий к задаче при открытии, закрытии или мерже пулл-реквеста"
    ),
    GITHUB_CUSTOM_REVIEW(
            "github-custom-review",
            "ARCANUM,github->github,review->review",
            "Данный обработчик реагирует на команды в комментариях к пулл-реквесту," +
                    " проверяет аппрувы и создает билд-статус с результатами ревью."
    ),
    TEAMCITY_BUILD_TRIGGER(
            "teamcity-build-trigger",
            "*->teamcity",
            "запускает сборку проекта в TeamCity при наступлении указанного события"
    ),
    ISSUE_LINKED_STATUS_TRANSITION(
            "issue-linked-status-transition",
            "*->st",
            "переводит в новый статус все связанные с задачей задачи при наступлении указанного события"
    ),
    ISSUE_TEAMCITY_ARTIFACT_YANDEX_DISK_LINK(
            "issue-teamcity-artifact-yandex-disk-link",
            "",
            "Обработчик добавляет комментарий к пулл-реквесту со ссылкой на файл Яндекс.Диска. " +
                    "Комментарий со ссылкой добавляется после сборки проекта в TeamCity"
    ),
    PULL_REQUEST_ISSUE_ASSIGN_QA_ENGINEER(
            "pull-request-issue-assign-qa-engineer",
            "review->st",
            "Назначение тестировщика в задаче Стартрека"
    ),
    PULL_REQUEST_ISSUE_SYNC_REVIEWERS(
            "pull-request-issue-sync-reviewers",
            "review->st",
            "Копирует ревьюеров пулл-реквеста в поле \"Reviewers\" в связанной задаче и синхронизирует при изменении " +
                    "(только в одну сторону — из пулл-реквеста в задачу)"
    ),
    ISSUE_RELATED_PULL_REQUEST(
            "issue-related-pull-request",
            "review->st",
            "При открытии пулл-реквеста в GitHub или Bitbucket создает связь с задачей (отображается в блоке " +
                    "\"Связанные\" в Startrek)"
    ),
    ISSUE_STATUS_TRANSITION(
            "issue-status-transition",
            "*->st",
            "Обработчик переводит задачу в новый статус при наступлении" +
                    " указанного события с учетом произвольного условия"
    ),
    PULL_REQUEST_FIELD_VALIDATION(
            "pull-request-field-validation",
            "review->review",
            "проверяет указанное поле нового пулл-реквеста на соответствие " +
                    "заданному шаблону и создает билд-статус в пулл-реквесте с результатами проверки"
    ),
    DEFAULT(
            "default",
            "github->st,qloud->teamcity,conductor->teamcity,st->github",
            "закрывает тикеты по ревью, запускает регрессию в тимсити после кондуктора или qloud"
    ),
    PULL_REQUEST_SOURCE_BRANCH_VALIDATION(
            "pull-request-source-branch-validation",
            "review->review",
            "При открытии пулл-реквеста проверяет имя source-ветки на соответствие регулярному " +
                    "выражению и создает билд-статус с результатом проверки"
    ),
    BITBUCKET_CROWDTEST_INTEGRATION(
            "bitbucket-crowdtest-integration",
            "bb->testpalm,bb->disk,bb->hitman,hitman->bb",
            "Запуск тестов в TestPalm и Hitman для пулл-реквестов в Bitbucket"
    ),
    ISSUE_RELATED_STATUS_TRANSITION(
            "issue-related-status-transition",
            "*->st",
            "alias к ssue-linked-status-transition, " +
                    "переводит в новый статус все связанные с задачей задачи при наступлении указанного события"
    ),
    PULL_REQUEST_TITLE_LABEL_STATUS(
            "pull-request-title-label-status",
            "review->review",
            "Обработчик изменяет название пулл-реквеста в ответ на оставленный коментарий"
    ),
    ISSUE_CREATE_COMMENT(
            "issue-create-comment",
            "*->st",
            "Обработчик создает в задаче комментарий при наступлении указанного события"
    ),
    BITBUCKET_YM_LISA_BUILD_STATUS(
            "bitbucket-ym-lisa-build-status",
            "bb->lisa,bb->sqs,sqs->bs",
            ""
    ),
    GITHUB_TEAMCITY_BUILD(
            "github-teamcity-build",
            "github->teamcity,teamcity->github",
            "запускает указанную сборку в TeamCity при создании нового пулл-реквеста или при" +
                    " добавлении к пулл-реквесту новых коммитов, если пулл-реквест не имеет конфликтов"
    ),
    YA_MUSIC_FRONTEND_BUILD(
            "ya-music-frontend-build",
            "bb->teamcity",
            "Запускает сборку в тимсити из pr"
    ),
    YA_MUSIC_FRONTEND_WORKFLOW(
            "ya-music-frontend-workflow",
            "conductor->st",
            "Меняет статус тикета при деплое в кондукторе"
    ),
    BITBUCKET_QLOUD_ENVIRONMENT_STATUS(
            "bitbucket-qloud-environment-status",
            "bb->bb,qloud->bb,qloud->st",
            "создает билд-статусы и комментарии, отражающие статус выкатки окружения в Qloud. " +
                    "Если к пулл-реквесту была привязана задача из Стартрека," +
                    " то обработчик также создаст комментарий со ссылкой на окружение"
    ),
    GITHUB_QLOUD_ENVIRONMENT_STATUS(
            "github-qloud-environment-status",
            "",
            ""
    ),
    ISSUE_QLOUD_ENVIRONMENT_STATUS(
            "issue-qloud-environment-status",
            "qloud->st",
            "отслеживает выкатку окружения в Qloud и редактирует свойства задачи в Стартреке"
    ),
    KP(
            "kp",
            "teamcity->github,github->github,",
            "Отслеживает teamcity, проставляет статусы в pr"
    ),
    PULL_REQUEST_ISSUE_SET_DEVELOPER_FIELD(
            "pull-request-issue-set-developer-field",
            "ARCANUM+,review->st",
            "алиас к pull-request-issue-assign-developer. " +
                    "При открытии пулл-реквеста в GitHub или Bitbucket обновляет поле \"Разработчик\" в связанной " +
                    "задаче " +
                    "Стартрека и указывает в данном поле автора пулл-реквеста. Также есть возможность обновить поле " +
                    "\"Разработчик\" в задачах, которые указаны в заголовке, описании и в сообщениях коммитов " +
                    "пулл-реквеста."
    ),
    ISSUE_RELEASE_TAG(
            "issue-release-tag",
            "github->st",
            "позволяет добавить тег из пулл-реквеста в связанную задачу в Стартреке"
    ),
    PULL_REQUEST_CHANGES_ISSUE_REPORTER(
            "pull-request-changes-issue-reporter",
            "",
            ""
    ),
    ISSUE_CREATE(
            "issue-create",
            "",
            "создает задачи в startrek"
    ),
    DESCRIBE(
            "describe",
            "",
            ""
    );
    private final String name;
    private final String description;
    @SuppressWarnings("ImmutableEnumChecker")
    private final Set<String> tags;

    Handlers(String name, String description) {
        this(name, "", description);
    }

    Handlers(String name, String tags, String description) {
        this.name = name;
        this.description = description;
        tags = tags.trim();
        if (tags.isEmpty()) {
            this.tags = Set.of();
        } else {
            this.tags = Set.of(tags.split("\s*,\s*"));
        }
    }

    public String getName() {
        return name;
    }

    public String getDescription() {
        return description;
    }

    public Set<String> getTags() {
        return tags;
    }

    public static Handlers byName(String key) {
        List<Handlers> handlers = Arrays.stream(values())
                .filter(h -> h.name.equals(key))
                .collect(Collectors.toList());
        Preconditions.checkState(handlers.size() > 0, "Not found handler %s", key);
        Preconditions.checkState(handlers.size() == 1, "Found more than one handler", handlers);
        return handlers.get(0);
    }

    @Override
    public String toString() {
        return "Handlers{" +
                "name='" + name + '\'' +
                ", description='" + description + '\'' +
                ", tags=" + tags +
                '}';
    }
}
