package ru.yandex.ci.core.taskletv2;

import java.util.Optional;

import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.CoreYdbTestBase;

import static org.assertj.core.api.Assertions.assertThat;

class TaskletV2MetadataTableTest extends CoreYdbTestBase {

    private static final long SEED = 934711501L;

    private final EnhancedRandom random = new EnhancedRandomBuilder()
            .seed(SEED)
            .overrideDefaultInitialization(true)
            .excludeField(f -> f.getName().equals("descriptorsCache"))
            .build();

    @Test
    public void saveAndLoadMetadata() {
        var origin = random.nextObject(TaskletV2Metadata.class);
        assertThat(origin.getDescriptors()).isNotNull().isNotEmpty();

        save(origin);

        var loaded = load(origin.getId()).orElseThrow();
        assertThat(loaded).isEqualTo(origin);
    }

    @Test
    public void loadOptionalShouldReturnEmpty() {
        var notExisting = TaskletV2Metadata.Id.of("12345");
        var loaded = load(notExisting);
        assertThat(loaded).isEmpty();
    }

    private void save(TaskletV2Metadata metadata) {
        db.currentOrTx(() ->
                db.taskletV2Metadata().save(metadata));
    }

    private Optional<TaskletV2Metadata> load(TaskletV2Metadata.Id key) {
        return db.currentOrReadOnly(() ->
                db.taskletV2Metadata().find(key));
    }

}
