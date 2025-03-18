package ru.yandex.ci.flow.engine.definition.context.impl;

import javax.annotation.Nullable;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@SuppressWarnings("LineLength")
public enum SupportType {
    NONE("Проверьте конфигурацию джобы, правильность заполнения полей ресурсов джобы, производят ли предыдущие джобы нужные ресурсы и т. п"),
    TSUM("Ошибка на стороне ЦУМа", "https://h.yandex-team.ru/?https%3A%2F%2Ftelegram.me%2Fjoinchat%2FA2HOPz96yg7uaYbnEjVrOQ"),
    NANNY("Ошибка на стороне Nanny", "https://h.yandex-team.ru/?https%3A%2F%2Ft.me%2Fjoinchat%2FBe0kOD50fVxMoi_8hPvG6Q"),
    CONDUCTOR("Ошибка на стороне Conductor", "https://h.yandex-team.ru/?https%3A%2F%2Ft.me%2Fjoinchat%2FCEYDED-v6qFQ2qyBaZ7JPw"),
    SANDBOX("Ошибка на стороне Sandbox", "http://h.yandex.net/?https%3A//t.me/joinchat/BLQp10XNMTrbGctZrctfGg"),
    MARKET_CSADMIN("Ошибка, с которой поможет группа эксплуатации", "https://h.yandex-team.ru/?https%3A%2F%2Ftelegram.me%2Fjoinchat%2FByTNYD79mq1FOGJkLYsq5A"),
    ARCADIA("Ошибка на стороне Аркадии", "mailto:devtools@yandex-team.ru"),
    PUNCHER("Ошибка на стороне Puncher", "mailto:puncher@yandex-team.ru"),
    JUGGLER("Ошибка на стороне Juggler", "http://h.yandex.net/?https%3A//t.me/joinchat/BvdM3T8-64_Wmd4f5vvKOA"),
    MDB("Ошибка на стороне MDB", "https://h.yandex-team.ru/?https%3A%2F%2Ft.me%2Fjoinchat%2FAAAAAEDje_JvGnMnVZxVvA"),
    YA_VAULT("Ошибка на стороне Секретницы", "https://forms.yandex-team.ru/surveys/14159/"),
    QLOUD("Ошибка на стороне Qloud", "https://t.me/joinchat/B5RVvwyTSRY167iH9nAlCA"),
    YANDEX_DEPLOY("Ошибка на стороне Яндекс.Деплоя", "https://h.yandex-team.ru/?https%3A%2F%2Ft.me%2Fjoinchat%2FBe0kOD50fVxMoi_8hPvG6Q"),
    MARKET_FRONT("Ошибка в джобе, относящейся к инфраструктуре маркетного фронта", "https://t.me/joinchat/AA_Bgwua1LcDgfn7G5ggFQ"),
    TEAMCITY("Ошибка на стороне Teamcity", "https://t.me/joinchat/AAAAAECtIetSv3Z_sqhQrQ");

    private final String helpMessage;
    @Nullable
    private final String supportLink;

    SupportType(String helpMessage) {
        this(helpMessage, null);
    }

    SupportType(String helpMessage, @Nullable String supportLink) {
        this.helpMessage = helpMessage;
        this.supportLink = supportLink;
    }

    public String getHelpMessage() {
        return helpMessage;
    }

    @Nullable
    public String getSupportChat() {
        return supportLink;
    }

    public String getType() {
        return toString();
    }
}
