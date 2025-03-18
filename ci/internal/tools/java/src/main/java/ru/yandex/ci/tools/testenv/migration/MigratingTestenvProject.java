package ru.yandex.ci.tools.testenv.migration;


import com.google.common.base.Strings;
import lombok.Builder;
import lombok.Data;

@Data
@Builder(toBuilder = true)
public class MigratingTestenvProject {
    private final String name;
    private final String dbProfile;
    private String ciTicket;
    private String rmTicket;
    private String rmComponent;
    private MigrationStatus migrationStatus;


    public boolean isRm() {
        return dbProfile.equals("release_machine");
    }

    public enum MigrationStatus {
        NONE,
        MISSING, //Надо разобраться с судьбой компонента
        TODO, //Надо либо улучшать автоматику, либо разбирать что тут происходит
        INCONSISTENT, //Статус миграционного тикета расходиться со статусом базы
        BLOCKED,
        WAITING_USER,
        DEADLINE_EXCEEDED,
        SKIP;

        public static MigrationStatus of(String string) {
            if (Strings.isNullOrEmpty(string)) {
                return NONE;
            }
            return MigrationStatus.valueOf(string);
        }


    }
}
