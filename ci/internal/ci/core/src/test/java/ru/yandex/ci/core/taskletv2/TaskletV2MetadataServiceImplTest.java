package ru.yandex.ci.core.taskletv2;

import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.client.taskletv2.TaskletV2TestServer;
import ru.yandex.ci.common.grpc.ProtobufTestUtils;
import ru.yandex.ci.core.CoreYdbTestBase;
import ru.yandex.ci.core.tasklet.TaskletMetadataHelper;
import ru.yandex.ci.core.tasklet.TaskletMetadataValidationException;
import ru.yandex.ci.engine.test.schema.PrimitiveInput;
import ru.yandex.ci.engine.test.schema.SimpleData;
import ru.yandex.ci.test.clock.OverridableClock;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class TaskletV2MetadataServiceImplTest extends CoreYdbTestBase {

    @Autowired
    TaskletV2MetadataService taskletV2MetadataService;

    @Autowired
    TaskletV2TestServer taskletV2TestServer;

    @Autowired
    TaskletV2MetadataHelper metadataHelper;

    @Autowired
    OverridableClock clock;

    @Test
    void fetchMetadataEmpty() {
        var key = TaskletV2Metadata.Id.of("b");
        assertThatThrownBy(() -> taskletV2MetadataService.fetchMetadata(key))
                .isInstanceOf(TaskletMetadataValidationException.class)
                .hasMessage("Unable to get Tasklet V2 build: build_id: \"b\"");
    }

    @Test
    void fetchMetadataEmptyCache() {
        var lookupKey = TaskletV2Metadata.Description.of("n", "t", "l");
        assertThatThrownBy(() -> taskletV2MetadataService.getLocalCache().fetchMetadata(lookupKey))
                .isInstanceOf(TaskletMetadataValidationException.class)
                .hasMessage("Unable to get Tasklet V2 label: label: \"l\" tasklet: \"t\" namespace: \"n\"");
    }

    @Test
    void fetchMetadata() {
        clock.stop();
        var id = metadataHelper.registerSchema(PrimitiveInput.getDescriptor(), SimpleData.getDescriptor());

        var metadata = taskletV2MetadataService.fetchMetadata(id);
        assertThat(metadata).isEqualTo(
                TaskletV2Metadata.builder()
                        .id(id)
                        .created(clock.instant())
                        .revision(335)
                        .inputMessage(PrimitiveInput.getDescriptor().getFullName())
                        .outputMessage(SimpleData.getDescriptor().getFullName())
                        .descriptors(TaskletMetadataHelper.descriptorSetFrom(
                                PrimitiveInput.getDescriptor(),
                                SimpleData.getDescriptor()).toByteArray())
                        .build()
        );

        bumpBuild(id);
        assertThat(taskletV2MetadataService.fetchMetadata(id)).isSameAs(metadata); // From Guava cache

        taskletV2MetadataService.clear();

        bumpBuild(id);
        var metadata2 = taskletV2MetadataService.fetchMetadata(id);
        assertThat(metadata2).isNotSameAs(metadata); // From Database cache, reloaded
        assertThat(metadata2).isEqualTo(metadata);

        db.currentOrTx(() -> db.taskletV2Metadata().deleteAll());

        bumpBuild(id);
        assertThat(taskletV2MetadataService.fetchMetadata(id)).isSameAs(metadata2); // From Guava cache

        taskletV2MetadataService.clear();

        bumpBuild(id);
        var metadata3 = taskletV2MetadataService.fetchMetadata(id); // Now requesting it again
        assertThat(metadata3).isNotSameAs(metadata);
        assertThat(metadata3).isNotSameAs(metadata2);

        assertThat(metadata3).isNotEqualTo(metadata2);
        assertThat(metadata3)
                .isEqualTo(metadata2.toBuilder().created(clock.instant()).build());
    }

    @Test
    void fetchLargeMetadata() {
        var key = TaskletV2Metadata.Id.of("b");
        var schema = "hash";
        var type = "tasklet.api.v2.GenericBinary";

        taskletV2TestServer.registerBuild(key.getId(), schema, type, type);
        taskletV2TestServer.registerSchema(schema, "TaskletV2MetadataServiceImplTest/dummy_java_tasklet.schema.zst");

        var metadata = taskletV2MetadataService.fetchMetadata(key);
        assertThat(metadata.getId())
                .isEqualTo(key);
        assertThat(metadata.getInputMessage())
                .isEqualTo(type);
        assertThat(metadata.getOutputMessage())
                .isEqualTo(type);

        var originalDescriptors = ProtobufTestUtils.parseProtoBinary(
                "TaskletV2MetadataServiceImplTest/dummy_java_tasklet.schema.zst",
                FileDescriptorSet.class
        );
        var descriptors = metadata.getDescriptors();
        assertThat(descriptors.length)
                .isLessThan(originalDescriptors.getSerializedSize()); // Optimized

        var metadata2 = taskletV2MetadataService.fetchMetadata(key);
        assertThat(metadata2)
                .isSameAs(metadata);
    }

    @Test
    void fetchMetadataWithCache() {
        var lookupKey = TaskletV2Metadata.Description.of("n", "t", "l");
        var id = metadataHelper.registerSchema(lookupKey, PrimitiveInput.getDescriptor(), SimpleData.getDescriptor());

        var metadata = taskletV2MetadataService.getLocalCache().fetchMetadata(lookupKey);
        assertThat(metadata).isEqualTo(
                TaskletV2Metadata.builder()
                        .id(id)
                        .created(clock.instant())
                        .revision(335)
                        .inputMessage(PrimitiveInput.getDescriptor().getFullName())
                        .outputMessage(SimpleData.getDescriptor().getFullName())
                        .descriptors(TaskletMetadataHelper.descriptorSetFrom(
                                PrimitiveInput.getDescriptor(),
                                SimpleData.getDescriptor()).toByteArray())
                        .build()
        );
    }

    private void bumpBuild(TaskletV2Metadata.Id id) {
        clock.plusSeconds(1);
        taskletV2TestServer.updateBuildTimestamp(id.getId());
    }

}
