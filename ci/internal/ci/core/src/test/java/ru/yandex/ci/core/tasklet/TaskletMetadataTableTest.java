package ru.yandex.ci.core.tasklet;

import java.util.Optional;

import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.CoreYdbTestBase;

import static org.assertj.core.api.Assertions.assertThat;

class TaskletMetadataTableTest extends CoreYdbTestBase {

    private static final long SEED = 934711501L;

    private final EnhancedRandom random = new EnhancedRandomBuilder()
            .seed(SEED)
            .overrideDefaultInitialization(true)
            .excludeField(f -> f.getName().equals("descriptorsCache"))
            .build();

    @Test
    public void saveAndLoadSandboxMetadata() {
        var origin = random.nextObject(TaskletMetadata.class);
        assertThat(origin.getId().getSandboxResourceId()).isGreaterThan(0);
        assertThat(origin.getDescriptors()).isNotNull().isNotEmpty();

        save(origin);

        var loaded = load(origin.getId()).orElseThrow();

        assertThat(loaded).isEqualTo(origin);
    }

    @Test
    public void saveAndLoadNotSandboxMetadata() {
        var key = random.nextObject(TaskletMetadata.Id.class, "sandboxResourceId");
        var origin = random.nextObject(TaskletMetadata.class, "key")
                .toBuilder()
                .id(key)
                .build();

        assertThat(origin.getId().getSandboxResourceId()).isEqualTo(0);

        save(origin);

        var loaded = load(origin.getId()).orElseThrow();

        assertThat(loaded).isEqualTo(origin);
    }

    @Test
    public void loadOptionalShouldReturnEmpty() {
        var notExisting = TaskletMetadata.Id.of("not-existing", TaskletRuntime.SANDBOX, 9L);
        var loaded = load(notExisting);
        assertThat(loaded).isEmpty();
    }

    private void save(TaskletMetadata metadata) {
        db.currentOrTx(() ->
                db.taskletMetadata().save(metadata));
    }

    private Optional<TaskletMetadata> load(TaskletMetadata.Id key) {
        return db.currentOrReadOnly(() ->
                db.taskletMetadata().find(key));
    }

}
