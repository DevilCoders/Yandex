package ru.yandex.ci.tools.testenv;

import java.io.File;
import java.io.IOException;
import java.util.List;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.SystemUtils;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.datasource.DriverManagerDataSource;

/**
 *  Утилита для чистки multitenant базы в ТЕ, скорее всего не понадобиться больше никогда.
 *  База - https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-mysql/cluster/mdbg372fq369bpth994
 *  Для запуска нужно прописать в toId testenv_check_id случившийся достаточно давно (полгода назад)
 *  Проще всего найти по табличке multitenancy_revisions.
 *
 *  Пароль брать в секрете
 *  https://yav.yandex-team.ru/secret/sec-01et88det5evkwjnhd999c19gs/explore/version/ver-01fajycymcnyhfgg61vpg9w2rp
 *  из блока mysql.multitenant.master
 *
 *  Вместе в optimize всех таблиц скрипт бежит очень долго, так что лучше запускать на ночь.
 *  Если его регулярно перезапускать, лучше ставить forceOptimize=false
 */
@Slf4j
public class CleanupMultitenantDb {

    public static void main(String[] args) {

        var jdbcTemplate = jdbcTemplate();

        List<String> tables = jdbcTemplate.queryForList("SHOW TABLES", String.class);

        String toId = "5m5nn";

        log.info("Got {} tables to cleanup", tables.size());
        for (int i = 1; i <= tables.size(); i++) {
            var table = tables.get(i - 1);
            log.info("Cleaning table {}/{}: {}", i, tables.size(), table);
            cleanTable(jdbcTemplate, table, toId, 100000, true);
        }
    }

    private static void cleanTable(JdbcTemplate jdbcTemplate, String table, String toId, int rowsPerDelete,
                                   boolean forceOptimize) {
        int totalDeleted = 0;
        String query = "DELETE FROM " + table + " WHERE tenant_id <= ? LIMIT ?";

        while (true) {
            int deleted = jdbcTemplate.update(query, toId, rowsPerDelete);
            totalDeleted += deleted;
            log.info(
                    "Deleted {} rows (total deleted {}m) from table {}",
                    deleted, toMln(totalDeleted), table
            );
            if (deleted == 0) {
                break;
            }
        }
        if (totalDeleted == 0 && !forceOptimize) {
            log.info("Nothing deleted from table {}. Skipping optimize.", table);
            return;
        }
        log.info(
                "Table {} cleanup finished. Deleted {}m rows. Optimizing table...",
                table, toMln(totalDeleted)
        );
        var optimizeResult = jdbcTemplate.queryForList("OPTIMIZE TABLE " + table);
        jdbcTemplate.query("ANALYZE NO_WRITE_TO_BINLOG TABLE " + table, rch -> {
        });
        log.info("Table {} optimized: {}", table, optimizeResult);
    }

    private static double toMln(int count) {
        return count / 1_000_000.;
    }

    private static JdbcTemplate jdbcTemplate() {
        var dataSource = new DriverManagerDataSource();
        dataSource.setDriverClassName("com.mysql.jdbc.Driver");
        dataSource.setUrl("jdbc:mysql://c-mdbg372fq369bpth994n.rw.db.yandex.net:3306/testenv?useSSL=true");
        dataSource.setUsername("testenv");
        dataSource.setPassword(getPassword());

        return new JdbcTemplate(dataSource);
    }

    private static String getPassword() {
        try {
            return Files.asCharSource(
                    new File(SystemUtils.getUserHome(), "/.ci/te-multitenant-db-password"),
                    Charsets.UTF_8
            ).read().trim();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
