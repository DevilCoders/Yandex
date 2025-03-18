package ru.yandex.ci.core.pr;

import java.util.Optional;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcBranch;

import static org.assertj.core.api.Assertions.assertThat;

class RevisionNumberTableTest extends CommonYdbTestBase {

    @Test
    void fetchLastKnown() {
        db.currentOrTx(() -> {
            db.revisionNumbers().save(number("trunk", "commit-1", 1));
            db.revisionNumbers().save(number("releases/one", "commit-11", 11));
            db.revisionNumbers().save(number("releases/one", "commit-12", 12));
            db.revisionNumbers().save(number("trunk", "commit-2", 2));
        });

        Optional<RevisionNumber> number = db.currentOrTx(() -> {
            ArcBranch branch = ArcBranch.ofBranchName("releases/one");
            return db.revisionNumbers().findLastKnown(branch);
        });

        assertThat(number).contains(number("releases/one", "commit-12", 12));
    }

    private RevisionNumber number(String trunk, String s, int i) {
        return new RevisionNumber(RevisionNumber.Id.of(trunk, s), i, 0L);
    }
}
