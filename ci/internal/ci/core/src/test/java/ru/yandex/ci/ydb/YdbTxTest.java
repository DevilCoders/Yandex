package ru.yandex.ci.ydb;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BiConsumer;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import ru.yandex.ci.core.CoreYdbTestBase;
import ru.yandex.ci.core.launch.versioning.Slot;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.launch.versioning.Versions;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
public class YdbTxTest extends CoreYdbTestBase {

    @SuppressWarnings("BusyWait")
    @Timeout(10)
    @Test
    void txInParallel() {
        var id = Versions.Id.of("c1", "b1", 1);

        db.currentOrTx(() -> db.versions().save(Versions.builder()
                .id(id)
                .slots(List.of())
                .build()));

        final AtomicInteger casStep = new AtomicInteger();
        BiConsumer<Integer, Integer> cas = (oldValue, newValue) -> {
            while (!casStep.compareAndSet(oldValue, newValue)) {
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    log.error("Interrupted", e);
                    break; // ---
                }
            }
        };

        //

        var v1 = Versions.builder()
                .id(id)
                .slots(List.of(Slot.builder().version(Version.major("1")).build()))
                .build();
        var v2 = Versions.builder()
                .id(id)
                .slots(List.of(Slot.builder().version(Version.major("2")).build()))
                .build();

        var t1 = new Thread(() -> {
            db.currentOrTx(() -> {
                log.info("START #1");
                cas.accept(11, 20); // 2
                db.versions().get(id);
                cas.accept(20, 21); // 2

                cas.accept(21, 30); // 3
                db.versions().save(v1);
            });
            cas.accept(30, 31);
        }, "#t1");

        var t2 = new Thread(() -> {
            db.currentOrTx(() -> {
                log.info("START #2");

                if (casStep.get() != 40) {
                    cas.accept(0, 10); // 1
                }
                db.versions().get(id);
                if (casStep.get() != 40) {
                    cas.accept(10, 11); // 1
                }

                if (casStep.get() != 40) {
                    cas.accept(31, 40); // 4
                }
                db.versions().save(v2);
            });
            cas.accept(40, 41); // 4
        }, "#t2");

        t1.start();
        t2.start();

        cas.accept(41, 42);
        var actual = db.currentOrReadOnly(() -> db.versions().get(id));
        assertThat(actual).isEqualTo(v2);

    }
}
