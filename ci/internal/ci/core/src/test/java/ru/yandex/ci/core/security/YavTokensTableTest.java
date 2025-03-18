package ru.yandex.ci.core.security;

import java.nio.file.Path;
import java.time.Instant;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.test.TestData;

public class YavTokensTableTest extends CommonYdbTestBase {

    @Test
    public void saveTokenTest() {

        String uid = "id42";

        YavToken yavToken = yavToken(uid, TestData.CONFIG_PATH_ABC, TestData.REVISION);

        saveToken(yavToken);

        var id = YavToken.Id.of(uid);
        YavToken actual = db.currentOrReadOnly(() ->
                db.yavTokensTable().get(id));

        Assertions.assertEquals(yavToken, actual);
    }

    @Test
    void findExisting() {
        YavToken token1 = yavToken("findExisting-1", TestData.CONFIG_PATH_ABE, TestData.REVISION);
        saveToken(token1);
        YavToken token2 = yavToken("findExisting-2", TestData.CONFIG_PATH_ABE, TestData.REVISION);
        saveToken(token2);

        YavToken actual = db.currentOrReadOnly(() ->
                db.yavTokensTable().findExisting(TestData.CONFIG_PATH_ABE, TestData.REVISION))
                .orElseThrow();

        Assertions.assertEquals(actual, token2);
    }

    private void saveToken(YavToken token) {
        db.currentOrTx(() -> db.yavTokensTable().save(token));
    }

    private YavToken yavToken(String uiid, Path configPath, ArcRevision revision) {
        return YavToken.builder()
                .id(YavToken.Id.of(uiid))
                .configPath(configPath.toString())
                .secretUuid("sec-42")
                .token("token")
                .delegationCommitId(revision.getCommitId())
                .delegatedBy("andreevdm")
                .abcService("ci")
                .created(Instant.ofEpochMilli(System.currentTimeMillis()))
                .build();
    }

}
