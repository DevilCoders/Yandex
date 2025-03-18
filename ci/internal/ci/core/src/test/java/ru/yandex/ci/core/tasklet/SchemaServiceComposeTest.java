package ru.yandex.ci.core.tasklet;

import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.gson.JsonObject;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.test.schema.Couple;
import ru.yandex.ci.core.test.schema.CoupleTasklet;
import ru.yandex.ci.core.test.schema.JudgeTasklet;
import ru.yandex.ci.core.test.schema.Mortgage;
import ru.yandex.ci.core.test.schema.MortgageTasklet;
import ru.yandex.ci.core.test.schema.NestedInputSingleTasklet;
import ru.yandex.ci.core.test.schema.Party;
import ru.yandex.ci.core.test.schema.PartyTasklet;
import ru.yandex.ci.core.test.schema.Person;
import ru.yandex.ci.core.test.schema.SinglePersonTasklet;
import ru.yandex.ci.core.test.schema.WrappedPerson;
import ru.yandex.ci.util.gson.JsonObjectBuilder;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.core.tasklet.SchemaOptions.defaultOptions;

class SchemaServiceComposeTest {

    private JobResourceType personDescriptor;
    private SchemaService schemaService;

    @BeforeEach
    public void setUp() {
        personDescriptor = JobResourceType.ofDescriptor(Person.getDescriptor());
        schemaService = new SchemaService();
    }

    @Test
    public void composeSingleInputSingleResource() {
        TaskletMetadata metadata = metadataFor(SinglePersonTasklet.getDescriptor(), Person.getDescriptor());

        JsonObject inputValue = person("Nickole", 42);
        JobResource singleInput = JobResource.any("ci.test.Person", inputValue);


        var composed = schemaService.composeInput(metadata, singleInput(), List.of(singleInput));


        assertThat(composed).isSameAs(inputValue);
    }

    private SchemaOptions singleInput() {
        return SchemaOptions.builder().singleInput(true).build();
    }

    private SchemaOptions singleOutput() {
        return SchemaOptions.builder().singleOutput(true).build();
    }

    @Test
    public void composeSingleInputSingleRoot() {
        TaskletMetadata metadata = metadataFor(SinglePersonTasklet.getDescriptor(), Person.getDescriptor());

        JsonObject inputValue = person("Lola", 15);
        JobResource singleInput = JobResource.mandatory(JobResourceType.of("ci.test.Person"), inputValue);

        var composed = schemaService.composeInput(metadata, singleInput(), List.of(singleInput));

        assertThat(composed).isSameAs(inputValue);
    }

    @Test
    public void composeSingleInputOptionalResource() {
        TaskletMetadata metadata = metadataFor(SinglePersonTasklet.getDescriptor(), Person.getDescriptor());

        JsonObject inputValue = person("Nickole", 42);

        JobResource singleInput = JobResource.any("ci.test.Person", inputValue);
        JobResource optionalExtraInput = JobResource.optional(
                JobResourceType.of("ci.test.SomeOptional"),
                new JsonObject()
        );


        var composed = schemaService.composeInput(
                metadata, singleInput(),
                List.of(optionalExtraInput, singleInput)
        );


        assertThat(composed).isSameAs(inputValue);
    }

    @Test
    public void composeSingleInputExtraResource() {
        TaskletMetadata metadata = metadataFor(SinglePersonTasklet.getDescriptor(), Person.getDescriptor());

        JsonObject inputValue = person("Nickole", 42);

        JobResource singleInput = JobResource.any("ci.test.Person", inputValue);
        JobResource extraInput = JobResource.any("ci.test.SomeOptional", new JsonObject());


        assertThatThrownBy(() ->
                schemaService.composeInput(metadata, singleInput(), List.of(extraInput, singleInput))
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessageContaining("found non optional resources")
                .hasMessageContaining("ci.test.SomeOptional");
    }

    @Test
    public void composeSingleInputZeroResource() {
        TaskletMetadata metadata = metadataFor(SinglePersonTasklet.getDescriptor(), Person.getDescriptor());

        assertThatThrownBy(() ->
                schemaService.composeInput(
                        metadata,
                        singleInput(),
                        Collections.emptyList()
                )
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessageContaining("resource with type ci.test.Person not found in inputs");
    }

    @Test
    public void composeSingleWrappedInput() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JsonObject somePerson = person("Nickole", 42);
        JobResource singleInput = JobResource.any("ci.test.Person", somePerson);


        var composed = schemaService.composeInput(metadata, defaultOptions(), List.of(singleInput));


        JsonObject expected = new JsonObject();
        expected.add("person_in_jail", somePerson);

        assertThat(composed).isEqualTo(expected);
    }

    @Test
    public void composeSingleWrappedInputWithRoot() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JsonObject lola = person("Lola", 15);
        JobResource lolaResource = JobResource.withParentField(
                JobResourceType.of("ci.test.Person"),
                lola,
                "person_in_jail"
        );

        var composed = schemaService.composeInput(metadata, defaultOptions(), List.of(lolaResource));

        assertThat(composed).isEqualTo(JsonObjectBuilder.builder().withProperty("person_in_jail", lola).build());
    }

    @Test
    public void composeSingleWrappedInputWithRootCamelCase_shouldInjectToSnakeCase() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JsonObject lola = person("Lola", 15);
        JobResource lolaResource = JobResource.withParentField(
                JobResourceType.of("ci.test.Person"),
                lola,
                "personInJail"
        );

        var composed = schemaService.composeInput(metadata, defaultOptions(), List.of(lolaResource));

        assertThat(composed).isEqualTo(JsonObjectBuilder.builder().withProperty("person_in_jail", lola).build());
    }

    @Test
    public void parentFieldWithDifferentCasesShouldRaiseError() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JsonObject lola = person("Lola", 15);
        JobResourceType personType = JobResourceType.of("ci.test.Person");

        assertThatThrownBy(() -> schemaService.composeInput(metadata, defaultOptions(),
                List.of(
                        JobResource.withParentField(personType, lola, "personInJail"),
                        JobResource.withParentField(personType, lola, "person_in_jail")
                )
        ));
    }

    @Test
    public void composeSingleWrappedInputWithRootExtraFields() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JobResource lola = JobResource.withParentField(
                JobResourceType.of("ci.test.Person"),
                person("Lola", 15),
                "personInJail"
        );
        JobResource donna = JobResource.withParentField(
                JobResourceType.of("ci.test.Person"),
                person("Donna", 76),
                "extraField"
        );

        assertThatThrownBy(() ->
                schemaService.composeInput(metadata, defaultOptions(), List.of(lola, donna))
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessageContaining("found unused fields")
                .hasMessageContaining("extraField")
                .hasMessageContaining("Donna")
                .hasMessageNotContaining("Lola");
    }

    @Test
    public void composeSingleWrappedInputWithRootAndExtra() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JobResource lola = JobResource.withParentField(
                JobResourceType.of("ci.test.Person"),
                person("Lola", 15),
                "personInJail"
        );

        JsonObject extra = person("Lilu", 17);

        JobResource extraResource = JobResource.any("ci.test.Person", extra);

        assertThatThrownBy(() ->
                schemaService.composeInput(metadata, defaultOptions(), List.of(extraResource, lola))
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessageContaining("found non optional resources")
                .hasMessageContaining("ci.test.Person")
                .hasMessageContaining("Lilu");
    }

    @Test
    public void composeMultipleExtraResource() {
        TaskletMetadata metadata = metadataFor(NestedInputSingleTasklet.getDescriptor(), WrappedPerson.getDescriptor());

        JsonObject somePerson = person("Nickole", 42);

        JobResource singleInput = JobResource.any("ci.test.Person", somePerson);
        JobResource extraInput = JobResource.any("ci.test.Person", somePerson);


        assertThatThrownBy(() ->
                schemaService.composeInput(metadata, defaultOptions(), List.of(singleInput, extraInput))
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessageContaining("found non optional resources")
                .hasMessageContaining("ci.test.Person");
    }

    @Test
    public void composeWithMultipleInput() {
        TaskletMetadata metadata = metadataFor(CoupleTasklet.getDescriptor(), Couple.getDescriptor());

        JsonObject benAffleck = person("Ben Affleck", 42);
        JsonObject mattDamon = person("Matt Damon", 42);

        JobResource benResource = JobResource.any("ci.test.Person", benAffleck);
        JobResource mattResource = JobResource.any("ci.test.Person", mattDamon);


        var composed = schemaService.composeInput(metadata, defaultOptions(), List.of(
                benResource,
                mattResource
        ));


        JsonObject expected = new JsonObject();
        expected.add("he", benAffleck);
        expected.add("she", mattDamon);

        assertThat(composed).isEqualTo(expected);
    }

    @Test
    public void composeWithMultipleInputOneRoot() {
        TaskletMetadata metadata = metadataFor(CoupleTasklet.getDescriptor(), Couple.getDescriptor());

        JsonObject benAffleck = person("Ben Affleck", 42);
        JsonObject mattDamon = person("Matt Damon", 42);

        JobResource benResource = JobResource.withParentField(JobResourceType.of("ci.test.Person"), benAffleck, "he");
        JobResource mattResource = JobResource.any("ci.test.Person", mattDamon);


        var composed = schemaService.composeInput(metadata, defaultOptions(), List.of(
                mattResource,
                benResource
        ));


        JsonObject expected = new JsonObject();
        expected.add("he", benAffleck);
        expected.add("she", mattDamon);

        assertThat(composed).isEqualTo(expected);
    }

    @Test
    public void composeWithMultipleInputNotEnoughInput() {
        TaskletMetadata metadata = metadataFor(CoupleTasklet.getDescriptor(), Couple.getDescriptor());

        JsonObject benAffleck = person("Ben Affleck", 42);

        JobResource benResource = JobResource.any("ci.test.Person", benAffleck);

        assertThatThrownBy(() ->
                schemaService.composeInput(metadata, defaultOptions(), List.of(benResource))
        )
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessageContaining("resource with type ci.test.Person not found in inputs");
    }

    @Test
    public void composeRepeated() {
        TaskletMetadata metadata = metadataFor(PartyTasklet.getDescriptor(), Party.getDescriptor());

        JsonObject joe = person("Joe", 32);
        JsonObject angela = person("Angela", 16);
        JsonObject nikole = person("Nikole", 18);
        JsonObject mike = person("Mike", 15);

        List<JobResource> resources = Stream.of(joe, angela, nikole, mike)
                .map(p -> JobResource.mandatory(personDescriptor, p))
                .collect(Collectors.toList());

        var composed = schemaService.composeInput(metadata, defaultOptions(), resources);

        JsonObject expected = JsonObjectBuilder.builder()
                .withProperty("owner", joe)
                .startArray("guests").withValues(angela, nikole, mike).end()
                .build();

        assertThat(composed).isEqualTo(expected);

        List<JobResource> outputResources = schemaService.extractOutput(metadata, defaultOptions(), composed);
        assertThat(outputResources).containsExactlyInAnyOrderElementsOf(resources);
    }

    @Test
    void extractUsingOriginalProtobufNames() {
        TaskletMetadata metadata = metadataFor(
                JudgeTasklet.getDescriptor(), Person.getDescriptor(), WrappedPerson.getDescriptor()
        );

        SchemaOptions options = SchemaOptions.builder()
                .singleInput(true)
                .singleOutput(false)
                .build();

        JsonObject joe = person("Joe", 32);
        JsonObject output = JsonObjectBuilder.builder()
                .withProperty("person_in_jail", joe)
                .build();

        List<JobResource> resources = schemaService.extractOutput(metadata, options, output);

        assertThat(resources)
                .containsExactly(JobResource.mandatory(personDescriptor, joe));
    }

    @Test
    public void composeRepeatedZero() {
        TaskletMetadata metadata = metadataFor(PartyTasklet.getDescriptor(), Party.getDescriptor());

        JsonObject joe = person("Joe", 32);

        List<JobResource> resources = List.of(
                JobResource.mandatory(personDescriptor, joe)
        );

        var composed = schemaService.composeInput(metadata, defaultOptions(), resources);

        JsonObject expected = JsonObjectBuilder.builder()
                .withProperty("owner", joe)
                .startArray("guests").end()
                .build();

        assertThat(composed).isEqualTo(expected);
    }

    @Test
    public void composeRepeatedWithRoot() {
        TaskletMetadata metadata = metadataFor(PartyTasklet.getDescriptor(), Party.getDescriptor());

        JsonObject steve = person("Steve", 41);
        JobResource steveResource = JobResource.withParentField(JobResourceType.of("ci.test.Person"), steve, "owner");

        JsonObject joe = person("Joe", 32);
        JsonObject angela = person("Angela", 16);
        JsonObject nikole = person("Nikole", 18);
        JsonObject mike = person("Mike", 15);

        List<JobResource> resources =
                Stream.concat(
                        Stream.of(joe, angela, nikole, mike)
                                .map(p -> JobResource.mandatory(personDescriptor, p)),
                        Stream.of(steveResource)
                ).collect(Collectors.toList());

        var composed = schemaService.composeInput(metadata, defaultOptions(), resources);

        JsonObject expected = JsonObjectBuilder.builder()
                .withProperty("owner", steve)
                .startArray("guests").withValues(joe, angela, nikole, mike).end()
                .build();

        assertThat(composed).isEqualTo(expected);


        List<JobResource> outputResources = schemaService.extractOutput(metadata, defaultOptions(), composed);
        assertThat(outputResources).containsExactlyInAnyOrderElementsOf(resources);
    }

    @Test
    public void composeDifferentInjects() {
        TaskletMetadata metadata = metadataFor(MortgageTasklet.getDescriptor(), Mortgage.getDescriptor());

        JsonObject owner = person("John Wick", 35);
        JsonObject house = new JsonObject();
        house.addProperty("address", "90 Bedford St, NY");
        house.addProperty("bedrooms", 3);

        JobResource ownerResource = JobResource.any("ci.test.Person", owner);
        JobResource houseResource = JobResource.any("ci.test.House", house);


        var input = schemaService.composeInput(metadata, defaultOptions(), List.of(
                houseResource,
                ownerResource
        ));


        JsonObject expected = new JsonObject();
        expected.add("owner", owner);
        expected.add("house", house);
        assertThat(input).isEqualTo(expected);

        List<JobResource> outputResources = schemaService.extractOutput(metadata, defaultOptions(), input);
        assertThat(outputResources).containsExactlyInAnyOrder(houseResource, ownerResource);
    }

    @Test
    public void recognizeSingleInputSingleResource() {
        TaskletMetadata metadata = metadataFor(SinglePersonTasklet.getDescriptor(), Person.getDescriptor());

        JsonObject inputValue = person("Nickole", 42);
        JobResource singleInput = JobResource.any("ci.test.Person", inputValue);


        List<JobResource> inputResources = List.of(singleInput);

        var composed = schemaService.composeInput(metadata, singleInput(), inputResources);

        assertThat(composed).isSameAs(inputValue);


        List<JobResource> outputResources = schemaService.extractOutput(metadata, singleOutput(), composed);

        assertThat(outputResources).isEqualTo(inputResources);
    }

    @Test
    void extractResourceNotFound() {
        TaskletMetadata metadata = metadataFor(MortgageTasklet.getDescriptor(), Mortgage.getDescriptor());

        JsonObject house = new JsonObject();
        house.addProperty("address", "90 Bedford St, NY");
        house.addProperty("bedrooms", 3);


        JsonObject output = new JsonObject();

        assertThatThrownBy(() -> schemaService.extractOutput(metadata, defaultOptions(), output))
                .hasMessageContaining("tasklet didn't produce resource ci.test.Person on field 'owner'")
                .hasMessageContaining("tasklet didn't produce resource ci.test.House on field 'house'");

    }

    private static FileDescriptorSet descriptorSetFrom(Descriptor descriptor) {

        Set<FileDescriptor> files = new LinkedHashSet<>();
        descriptorSetFrom(descriptor.getFile(), files);

        FileDescriptorSet.Builder result = FileDescriptorSet.newBuilder();
        files.stream()
                .map(FileDescriptor::toProto)
                .forEach(result::addFile);

        return result.build();
    }

    private static JsonObject person(String name, int age) {
        JsonObject benAffleck = new JsonObject();
        benAffleck.addProperty("age", age);
        // используется json naming policy, отличается от того, как поле задекларировано в proto
        // https://developers.google.com/protocol-buffers/docs/proto3#json
        benAffleck.addProperty("firstName", name);
        return benAffleck;
    }

    private static TaskletMetadata metadataFor(
            Descriptor taskletDescriptor, Descriptor inputOutputDescriptor
    ) {
        return metadataFor(taskletDescriptor, inputOutputDescriptor, inputOutputDescriptor);
    }

    private static TaskletMetadata metadataFor(
            Descriptor taskletDescriptor, Descriptor inputDescriptor, Descriptor outputDescriptor
    ) {
        FileDescriptorSet descriptors = descriptorSetFrom(taskletDescriptor);
        return new TaskletMetadata.Builder()
                .descriptors(descriptors.toByteArray())
                .inputType(JobResourceType.ofDescriptor(inputDescriptor))
                .outputType(JobResourceType.ofDescriptor(outputDescriptor))
                .build();
    }

    private static void descriptorSetFrom(FileDescriptor current,
                                          Set<FileDescriptor> result) {
        if (!result.contains(current)) {
            for (FileDescriptor dependency : current.getDependencies()) {
                descriptorSetFrom(dependency, result);
            }
            result.add(current);
        }
    }
}
