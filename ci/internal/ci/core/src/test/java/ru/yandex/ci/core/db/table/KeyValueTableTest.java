package ru.yandex.ci.core.db.table;

import java.util.Objects;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.db.model.KeyValue;

import static org.assertj.core.api.Assertions.assertThat;

public class KeyValueTableTest extends CommonYdbTestBase {

    @Test
    void testObject() {
        KeyValueTableTest.Obj obj = new KeyValueTableTest.Obj(42);
        db.currentOrTx(() -> db.keyValue().setValue("ns1", "o1", obj));

        db.currentOrTx(() -> assertThat(db.keyValue().findObject("ns1", "o1", Obj.class).orElseThrow()).isEqualTo(obj));
        db.currentOrTx(() -> assertThat(db.keyValue().findObject("o1", Obj.class).isEmpty()).isTrue());
        db.currentOrTx(() -> assertThat(db.keyValue().findObject("ns2", "o1", Obj.class).isEmpty()).isTrue());
    }

    private static class Obj {
        int i;

        private Obj(int i) {
            this.i = i;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof KeyValueTableTest.Obj)) {
                return false;
            }
            KeyValueTableTest.Obj obj = (KeyValueTableTest.Obj) o;
            return i == obj.i;
        }

        @Override
        public int hashCode() {
            return Objects.hash(i);
        }
    }

    @Test
    void booleanValue() {
        db.currentOrTx(() -> db.keyValue().setValue("ns1", "o1", true));
        db.currentOrTx(() -> assertThat(db.keyValue().getBoolean("ns1", "o1")).isEqualTo(true));
        db.currentOrTx(() -> assertThat(db.keyValue().getBoolean("ns1", "o1", false)).isEqualTo(true));

        db.currentOrTx(() -> db.keyValue().setValue("o1", false));
        db.currentOrTx(() -> assertThat(db.keyValue().getBoolean("o1")).isEqualTo(false));

        db.currentOrTx(() -> assertThat(db.keyValue().getBoolean("ns1", "unexisting", false)).isEqualTo(false));
        db.currentOrTx(() -> assertThat(db.keyValue().getBoolean("ns1", "unexisting", true)).isEqualTo(true));

    }

    @Test
    void findValues() {
        db.currentOrTx(() -> {
            db.keyValue().setValue("ns", "bool", true);
            db.keyValue().setValue("ns", "str", "some string value");
            db.keyValue().setValue("ns", "int", 1);
        });
        db.currentOrTx(() ->
                assertThat(db.keyValue().findValues("ns")).containsExactlyInAnyOrder(
                        new KeyValue(KeyValue.Id.of("ns", "bool"), "true"),
                        new KeyValue(KeyValue.Id.of("ns", "str"), "some string value"),
                        new KeyValue(KeyValue.Id.of("ns", "int"), "1")
                )
        );
    }
}
