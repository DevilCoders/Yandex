package ru.yandex.ci.ydb;

import java.util.List;
import java.util.stream.IntStream;

import org.apache.commons.lang3.StringUtils;
import org.junit.jupiter.api.Test;

import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

import ru.yandex.ci.core.CoreYdbTestBase;
import ru.yandex.ci.ydb.service.CounterEntity;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class KikimrTableCiTest extends CoreYdbTestBase {

    private static final String NS = "test";

    @Test
    void testFind() {
        var counters = IntStream.range(0, 3010)
                .mapToObj(i -> CounterEntity.of(CounterEntity.Id.of(NS, key(i)), i))
                .toList();
        db.currentOrTx(() -> db.counter().save(counters));

        test(5, counters.subList(3005, 3010),
                gte(3005));

        test(5, counters.subList(0, 5),
                gte(0), YqlLimit.top(5));

        test(5, counters.subList(5, 10),
                gte(5), YqlLimit.top(5));

        test(5, counters.subList(10, 15),
                gte(5), YqlLimit.range(5, 10));

        //

        test(5, counters.subList(3005, 3010),
                gte(3005), YqlLimit.top(10000)); // top is ignored

        //

        test(1005, counters.subList(2005, 3010),
                gte(2005));

        test(2005, counters.subList(1005, 3010),
                gte(1005));

        test(3010, counters,
                gte(0));

        //

        test(2000, counters.subList(10, 2010),
                gte(0), YqlLimit.range(10, 2010));

        test(2000, counters.subList(15, 2015),
                gte(5), YqlLimit.range(10, 2010));
    }

    @Test
    void testExtraLargeFetch() {
        var counters = IntStream.range(0, 16385)
                .mapToObj(i -> CounterEntity.of(CounterEntity.Id.of(NS, key(i)), i))
                .toList();
        db.currentOrTx(() -> db.counter().save(counters));

        test(2000, counters.subList(10, 2010),
                gte(0), YqlLimit.range(10, 2010));

        test(85, counters.subList(16300, 16385),
                gte(1000), YqlLimit.range(15300, 15386));

        assertThatThrownBy(() -> find(gte(0)))
                .hasMessage("Fetched too much rows: 16385. If you need more rows, use scan() or readTable()");
    }

    @Test
    void testSelectTooMuch() {
        assertThatThrownBy(() -> find(gte(0), YqlLimit.top(16385)))
                .hasMessage("Select limit cannot exceed 16384. If you need more rows, use scan() or readTable()");
    }

    private void test(
            int expectSize,
            List<CounterEntity> expect,
            YqlStatementPart<?> part,
            YqlStatementPart<?>... otherParts) {
        var value = find(part, otherParts);
        assertThat(value)
                .hasSize(expectSize)
                .isEqualTo(expect);
    }

    private List<CounterEntity> find(YqlStatementPart<?> part, YqlStatementPart<?>... otherParts) {
        return db.currentOrReadOnly(() -> db.counter().find(part, otherParts));
    }

    private YqlStatementPart<?> gte(int id) {
        return YqlPredicate.where("id.ns").eq(NS).and("id.key").gte(key(id));
    }

    private static String key(int id) {
        return StringUtils.leftPad(String.valueOf(id), 10, '0');
    }

}
