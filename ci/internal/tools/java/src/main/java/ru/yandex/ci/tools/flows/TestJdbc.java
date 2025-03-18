package ru.yandex.ci.tools.flows;

import java.sql.SQLException;
import java.util.Properties;

import com.yandex.ydb.jdbc.YdbDriver;
import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
public class TestJdbc extends AbstractSpringBasedApp {

    @Override
    protected void run() throws SQLException {
        var jdbc = new YdbDriver();

        var url = "jdbc:ydb:ydb-ru.yandex.net:2135/ru/ci/stable/ci?token=~/.ydb/token";
        try (var conn = jdbc.connect(url, new Properties())) {
            conn.setReadOnly(true);
            var rs = conn.createStatement().executeQuery("""
                    DECLARE $pred_0_id_taskId AS Utf8;
                    DECLARE $pred_2_scheduleTime AS Timestamp;
                    $pred_0_id_taskId = 'launchCleanup';
                    $pred_2_scheduleTime = DateTime::MakeTimestamp(DateTime::ParseIso8601('2021-10-20T15:42:57.940Z'));

                    SELECT status, (case when status in ('FAILED', 'EXPIRED') then 1 else 0 end) as total_failed, sum(1) as total
                    FROM `bazinga/OnetimeJobComplete` VIEW `IDX_TASKID_STIME`
                    WHERE (`id_taskId` = $pred_0_id_taskId) AND (`scheduleTime` >= $pred_2_scheduleTime)
                    group by status;
                    """);
            log.info("Executed...");
            while (rs.next()) {
                log.info("{} total_failed = {}, total = {}",
                        rs.getString("status"), rs.getLong("total_failed"), rs.getLong("total"));
            }
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
