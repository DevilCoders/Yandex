package ru.yandex.ci.core.resolver;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonPrimitive;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.ValueSource;

import ru.yandex.ci.engine.test.schema.PrimitiveInput;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.gson.JsonObjectBuilder;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.core.resolver.PropertiesSubstitutor.asJsonObject;

@Slf4j
class PropertiesSubstitutorTest {

    @Test
    void returnSameInstanceIfNoPropertiesFound() {
        var source = person("Alice Bukowski").build();

        testSubstitute(source, new JsonObject(),
                source.deepCopy());
    }

    @Test
    void injectSimple() {
        var source = person("Alice ${lastName}").build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .build();

        testSubstitute(source, document,
                person("Alice Somebody").build());
    }

    @Test
    void booleanNegate() {
        var source = new JsonPrimitive("${!`false` && !`true`}");
        testSubstitute(source, new JsonObject(), new JsonPrimitive(false));
    }

    @Test
    void injectSimpleKey() {
        var source = person("Alice ${lastName}")
                .withProperty("${string}", "is string")
                .withProperty("${int}", "is int")
                .withProperty("${boolean}", "is boolean")
                .build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .withProperty("string", "string value")
                .withProperty("int", 42)
                .withProperty("boolean", true)
                .build();

        testSubstitute(source, document,
                person("Alice Somebody")
                        .withProperty("string value", "is string")
                        .withProperty("42", "is int")
                        .withProperty("true", "is boolean")
                        .build());
    }

    @Test
    void injectSimpleKeyInvalidNull() {
        var source = person("Alice ${lastName}")
                .withProperty("${null}", "is null")
                .build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .withProperty("null", new JsonNull())
                .build();

        testSubstituteThrow(source, document, """
                Unable to substitute field "${null}" with expression "${null}": \
                No value resolved for expression: null""");
    }

    @Test
    void injectSimpleKeyInvalidArray() {
        var source = person("Alice ${lastName}")
                .withProperty("${list}", "is list")
                .build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .withProperty("list", new JsonArray())
                .build();

        testSubstituteThrow(source, document, """
                Unable to calculate ${list} expression: ${list}, must be string, got []""");
    }

    @Test
    void injectSimpleKeyInvalidObject() {
        var source = person("Alice ${lastName}")
                .withProperty("${object}", "is list")
                .build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .withProperty("object", new JsonObject())
                .build();

        testSubstituteThrow(source, document, """
                Unable to calculate ${object} expression: ${object}, must be string, got {}""");
    }

    @Test
    void injectNotClosed() {
        var source = person("Alice ${lastName").build();
        testSubstitute(source, new JsonObject(),
                person("Alice ${lastName").build());
    }

    @Test
    void injectNotClosedEscape() {
        var source = person("Alice $${lastName").build();
        testSubstitute(source, new JsonObject(),
                person("Alice ${lastName").build());
    }

    @Test
    void injectSimpleWithSkip() {
        var source = person("Alice ${lastName} from ${context.username}").build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .build();
        var documentWithSkip = DocumentSource.skipContext(DocumentSource.of(document));

        testSubstitute(source, documentWithSkip,
                person("Alice Somebody from ${context.username}").build());
    }

    @Test
    void injectFullWithSkip() {
        var source = person("${context.username}").build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .build();

        var documentWithSkip = DocumentSource.skipContext(DocumentSource.of(document));

        testSubstitute(source, documentWithSkip,
                person("${context.username}").build());
    }

    @Test
    void injectSimpleSorted() {
        var source = person("Alice ${items[].lastName | sort(@) | @[0]}").build();

        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .startObject()
                .withProperty("lastName", "Somebody")
                .end()
                .startObject()
                .withProperty("lastName", "Anybody")
                .end()
                .end()
                .build();

        testSubstitute(source, document,
                person("Alice Anybody").build());
    }


    @Test
    void injectSimpleMultiLookup() {
        var source = person("${firstName} ${lastName}")
                .build();
        var document = JsonObjectBuilder.builder()
                .withProperty("firstName", "John")
                .withProperty("lastName", "Dow")
                .build();

        testSubstitute(source, document,
                person("John Dow").build());
    }


    @Test
    void injectSimpleSingleString() {
        var source = person("${title}")
                .build();
        var document = JsonObjectBuilder.builder()
                .withProperty("title", "Jane Dow")
                .build();

        testSubstitute(source, document,
                person("Jane Dow").build());
    }

    @Test
    void injectSimpleSingleNumber() {
        var source = person("Alice Bukowski")
                .withProperty("age", "${age}")
                .build();
        var document = JsonObjectBuilder.builder().withProperty("age", 16).build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .withProperty("age", 16)
                        .build());
    }

    @Test
    void injectSimpleSingleBoolean() {
        var source = person("Alice Bukowski")
                .withProperty("smart", "${smart}")
                .build();
        var document = JsonObjectBuilder.builder().withProperty("smart", false).build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .withProperty("smart", false)
                        .build());
    }


    @Test
    void injectSimpleProto() {
        var source = person("Alice ${string_field}")
                .withProperty("age", "${int_field}")
                .withProperty("rate", "${double_field}")
                .withProperty("smart", "${boolean_field}")
                .build();

        var document = PrimitiveInput.newBuilder()
                .setIntField(14)
                .setStringField("Bukowski")
                .setDoubleField(8.4)
                .setBooleanField(true)
                .build();

        testSubstitute(source, asJsonObject(document),
                person("Alice Bukowski")
                        .withProperty("rate", 8.4)
                        .build());
    }

    @Test
    void injectProtoWithJavascriptNotation() {
        var source = person("Alice ${stringField}").build();

        var document = PrimitiveInput.newBuilder()
                .setStringField("Bukowski")
                .build();

        testSubstituteThrow(source, asJsonObject(document), """
                Unable to substitute field "name" with expression \
                "Alice ${stringField}": No value resolved for expression: stringField""");
    }

    @Test
    void injectFullMessage() {
        var source = JsonObjectBuilder.builder()
                .withProperty("persons", "${fullPerson}")
                .build();

        var document = JsonObjectBuilder.builder()
                .withProperty("fullPerson", person("Alice Bukowski").build())
                .build();

        testSubstitute(source, document,
                JsonObjectBuilder.builder()
                        .withProperty("persons", person("Alice Bukowski").build())
                        .build());
    }

    @Test
    void complexMessage() {
        var source = person("Alice ${alice.lastName}")
                .startArray("friends")
                .withValue(person("Mike ${mike.lastName}").build())
                .withValue(
                        person("Ann ${ann.lastName}")
                                .startArray("flowers")
                                .withValue("${ann.flower[0]} yellow")
                                .withValue("rose red")
                                .withValue("${ann.flower[1]} white")
                                .end()
                                .build()
                )
                .end()
                .build();

        var document = JsonObjectBuilder.builder()
                .withProperty("alice", documentPerson("Bukowski").build())
                .withProperty("mike", documentPerson("Tyson").build())
                .withProperty("ann", documentPerson("Cooper")
                        .startArray("flower").withValues("tulip", "chrysanthemum").end()
                        .build())
                .build();

        testSubstitute(source, resolve -> {
                    assertThat(resolve).isFalse();
                    return document;
                },
                person("Alice Bukowski")
                        .startArray("friends")
                        .withValue(person("Mike Tyson").build())
                        .withValue(
                                person("Ann Cooper")
                                        .startArray("flowers")
                                        .withValue("tulip yellow")
                                        .withValue("rose red")
                                        .withValue("chrysanthemum white")
                                        .end()
                                        .build()
                        )
                        .end()
                        .build());
    }

    @ParameterizedTest
    @ValueSource(strings = {
            "${tasks.full-person.items[0].id}",
            "${tasks.full-person.items[ 0 ].id}",
            "${tasks.full-person. items [0]. id}",
            "${  tasks.full-person.items[0].id  }",
            "${ tasks . full-person . items[0].id}"
    })
    void tasksAutoQuotes(String expression) {
        var source = JsonObjectBuilder.builder()
                .withProperty("persons", expression)
                .withProperty("lookups", "${length(tasks.*.items[])}")
                .build();

        var document = JsonObjectBuilder.builder()
                .startMap("tasks")
                .startMap("full-person")
                .startArray("items")
                .startObject()
                .withProperty("id", person("Alice Bukowski").build())
                .end()
                .startObject()
                .withProperty("id", person("John Dow").build())
                .end()
                .end()
                .end()
                .startMap("copy-person")
                .startArray("items")
                .startObject()
                .withProperty("id", person("Jane Dow").build())
                .end()
                .end()
                .end()
                .end()
                .build();

        testSubstitute(source, resolve -> {
                    assertThat(resolve).isTrue();
                    return document;
                },
                JsonObjectBuilder.builder()
                        .withProperty("persons", person("Alice Bukowski").build())
                        .withProperty("lookups", 3)
                        .build());

    }

    @Test
    void failIfPropertyNotFoundFull() {
        testSubstituteThrow(new JsonPrimitive("${context.unexpected.property}"), new JsonObject(), """
                Unable to substitute with expression "${context.unexpected.property}": \
                No value resolved for expression: context.unexpected.property""");
    }

    @Test
    void failIfPropertyNotFoundPartil() {
        testSubstituteThrow(new JsonPrimitive("Value = ${context.unexpected.property}"), new JsonObject(), """
                Unable to substitute with expression "Value = ${context.unexpected.property}": \
                No value resolved for expression: context.unexpected.property""");
    }

    @Test
    void failIfPropertyNotFoundRecursive() {
        var source = JsonParser.parseString("""
                {
                    "input": {
                        "reference": "${tasks.upstream1.output.key}"
                    }
                }
                """).getAsJsonObject();

        testSubstituteThrow(source, new JsonObject(), """
                Unable to substitute field "input.reference" with expression \
                "${tasks.upstream1.output.key}": No value resolved for expression: tasks.upstream1.output.key""");
    }

    @Test
    void dontExposeEnvironment() {
        testSubstituteThrow(new JsonPrimitive("${env:USER}"), new JsonObject(), """
                Unable to substitute with expression "${env:USER}": \
                Unable to compile expression "env:USER": syntax error mismatched input ':' \
                expecting {<EOF>, '.', '&&', '||', '|', '[', '[?', COMPARATOR} at position 3""");
    }

    @Test
    void dontExposeSystem() {
        System.setProperty("system.property", "should not be exposed");

        testSubstituteThrow(new JsonPrimitive("${sys:system.property}"), new JsonObject(), """
                Unable to substitute with expression "${sys:system.property}": Unable to compile expression \
                "sys:system.property": syntax error mismatched input ':' expecting {<EOF>, '.', '&&', '||', '|', \
                '[', '[?', COMPARATOR} at position 3""");
    }

    @Test
    void dontExposeJavascript() {
        testSubstituteThrow(new JsonPrimitive("${javascript:3 + 4}}"), new JsonObject(), """
                Unable to substitute with expression "${javascript:3 + 4}}": Unable to compile expression \
                "javascript:3 + 4": syntax error mismatched input ':' expecting {<EOF>, '.', '&&', '||', '|', '[', \
                '[?', COMPARATOR} at position 10, syntax error token recognition error at: '+' at position 13""");
    }

    @Test
    void dontExposeConstants() {
        testSubstituteThrow(new JsonPrimitive("${const:java.awt.event.KeyEvent.VK_ESCAPE}"), new JsonObject(), """
                Unable to substitute with expression "${const:java.awt.event.KeyEvent.VK_ESCAPE}": Unable to \
                compile expression "const:java.awt.event.KeyEvent.VK_ESCAPE": syntax error mismatched input ':' \
                expecting {<EOF>, '.', '&&', '||', '|', '[', '[?', COMPARATOR} at position 5""");
    }

    @Test
    void dontExposeFile() {
        testSubstituteThrow(new JsonPrimitive("${file:UTF-8:/etc/hosts}"), new JsonObject(), """
                Unable to substitute with expression "${file:UTF-8:/etc/hosts}": \
                Unable to compile expression "file:UTF-8:/etc/hosts": syntax error mismatched input ':' \
                expecting {<EOF>, '.', '&&', '||', '|', '[', '[?', COMPARATOR} \
                at position 4, syntax error token recognition error at: '/' at position 11, \
                syntax error token recognition error at: '/' at position 15""");
    }

    @Test
    void functionJoinAny() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[].id | join_any(', ', @)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .startObject()
                .withProperty("id", 1)
                .end()
                .startObject()
                .withProperty("id", 2)
                .end()
                .startObject()
                .withProperty("id", 3)
                .end()
                .end()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .withProperty("id", "1, 2, 3")
                        .build());
    }

    @Test
    void functionJoinAnyMixedTypes() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[].id | join_any(', ', @)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .startObject()
                .withProperty("id", 1)
                .end()
                .startObject()
                .withProperty("id", true)
                .end()
                .startObject()
                .withProperty("id", "Test")
                .end()
                .end()
                .build();

        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression \
                "${items[].id | join_any(', ', @)}": Invalid argument type calling "join_any": \
                expected array of any value but was array containing number, boolean and string""");
    }

    @Test
    void functionJoinAnyNotAList() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items | join_any(', ', @) | @.id}")
                .build();

        var document = JsonObjectBuilder.builder()
                .withProperty("items", 1)
                .build();
        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression \
                "${items | join_any(', ', @) | @.id}": Invalid argument type calling "join_any": \
                expected array of any value but was number""");
    }

    @Test
    void functionJoinAnyInvalidArg() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items | join_any(`1`, @) | @.id}")
                .build();

        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .end()
                .build();
        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression \
                "${items | join_any(`1`, @) | @.id}": Invalid argument type calling "join_any": \
                expected string but was number""");
    }

    @Test
    void functionJoinRecurse() {
        var source = person("Alice Bukowski")
                .startMap("id1")
                .startMap("id2")
                .withProperty("actual_id", "${items | join_any(', ', @) | @.id}")
                .end()
                .end()
                .build();

        var document = JsonObjectBuilder.builder()
                .withProperty("items", 1)
                .build();
        testSubstituteThrow(source, document, """
                Unable to substitute field "id1.id2.actual_id" with expression \
                "${items | join_any(', ', @) | @.id}": Invalid argument type calling "join_any": \
                expected array of any value but was number""");
    }

    @Test
    void functionSingleObject() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[?id == `3`] | single(@)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .startObject()
                .withProperty("id", 1)
                .end()
                .startObject()
                .withProperty("id", 2)
                .end()
                .startObject()
                .withProperty("id", 3)
                .end()
                .end()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .withProperty("id", JsonObjectBuilder.builder()
                                .withProperty("id", 3)
                                .build())
                        .build());
    }

    @Test
    void functionSingleValue() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[?@ == `3`] | single(@)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .withValue(2)
                .withValue(3)
                .end()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .withProperty("id", 3)
                        .build());
    }

    @Test
    void functionSingleFoundNone() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[?@ >= `4`] | single(@)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .withValue(2)
                .withValue(3)
                .end()
                .build();

        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression "${items[?@ >= `4`] | single(@)}": \
                Expect single value, found none""");
    }

    @Test
    void functionSingleFoundMoreThanOne() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[?@ >= `2`] | single(@)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .withValue(2)
                .withValue(3)
                .end()
                .build();

        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression "${items[?@ >= `2`] | single(@)}": \
                Expect single value, found 2""");
    }

    @Test
    void functionNonEmptyValue() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[?@ == `3`] | non_empty(@)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .withValue(2)
                .withValue(3)
                .end()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .startArray("id")
                        .withValue(3)
                        .end()
                        .build());
    }

    @Test
    void functionNonEmptyNone() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${items[?@ >= `4`] | non_empty(@)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .withValue(2)
                .withValue(3)
                .end()
                .build();

        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression \
                "${items[?@ >= `4`] | non_empty(@)}": Expect one or more values, found none""");
    }

    @Test
    void functionNonEmptyFoundMoreThanOne() {
        JsonObject source = person("Alice Bukowski")
                .withProperty("id", "${items[?@ >= `2`] | non_empty(@)}")
                .build();

        var document = JsonObjectBuilder.builder()
                .startArray("items")
                .withValue(1)
                .withValue(2)
                .withValue(3)
                .end()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .startArray("id")
                        .withValue(2)
                        .withValue(3)
                        .end()
                        .build());
    }

    @Test
    void functionAdd() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${add(`1`, `2`)}")
                .build();

        testSubstitute(source, new JsonObject(),
                person("Alice Bukowski")
                        .withProperty("id", 3)
                        .build());
    }

    @Test
    void functionReplace() {
        var source = JsonObjectBuilder.builder()
                .withProperty("case1", "${replace('Hello, world!', 'Hello', 'Goodbuy')}")
                .withProperty("case2", "${replace('Hello, world!', 'l', '')}")
                .withProperty("case3", "${replace('Hello, world!', '^(.*), (\\S+)\\W{1}$', '$2: \"$1!\"')}")
                .withProperty("case4", "${replace('Hello, world!', 'Foo', 'Bar')}")
                .build();

        var document = JsonObjectBuilder.builder().build();

        testSubstitute(source, document,
                JsonObjectBuilder.builder()
                        .withProperty("case1", "Goodbuy, world!")
                        .withProperty("case2", "Heo, word!")
                        .withProperty("case3", "world: \"Hello!\"")
                        .withProperty("case4", "Hello, world!")
                        .build()
        );
    }

    @Test
    void functionToJsonArray() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${to_json('[1, true,\"three\"]')}")
                .build();
        var document = JsonObjectBuilder.builder()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .startArray("id")
                        .withValue(1)
                        .withValue(true)
                        .withValue("three")
                        .end()
                        .build());
    }

    @Test
    void functionToJsonMap() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${to_json('{\"v1\": 1, \"v2\": true, \"v3\": \"three\"}')}")
                .build();
        var document = JsonObjectBuilder.builder()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .startMap("id")
                        .withProperty("v1", 1)
                        .withProperty("v2", true)
                        .withProperty("v3", "three")
                        .end()
                        .build());
    }

    @Test
    void functionToJsonSimpleString() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${to_json('username')}")
                .build();
        var document = JsonObjectBuilder.builder()
                .build();

        testSubstitute(source, document,
                person("Alice Bukowski")
                        .withProperty("id", "username")
                        .build());
    }

    @Test
    void functionCase() {
        var source = JsonObjectBuilder.builder()
                .withProperty("lowercase", "${lowercase('hello world! YEEHA!')}")
                .withProperty("uppercase", "${uppercase('hello world! YEEHA!')}")
                .withProperty("capitalize", "${capitalize('hello world! YEEHA!')}")
                .build();

        var document = JsonObjectBuilder.builder().build();

        testSubstitute(source, document,
                JsonObjectBuilder.builder()
                        .withProperty("lowercase", "hello world! yeeha!")
                        .withProperty("uppercase", "HELLO WORLD! YEEHA!")
                        .withProperty("capitalize", "Hello world! YEEHA!")
                        .build()
        );
    }

    @Test
    void functionToJsonInvalid() {
        var source = person("Alice Bukowski")
                .withProperty("id", "${to_json(gibberish)}")
                .build();
        var document = JsonObjectBuilder.builder()
                .build();

        testSubstituteThrow(source, document, """
                Unable to substitute field "id" with expression "${to_json(gibberish)}": \
                Invalid argument type calling "to_json": expected string but was null""");
    }

    @Test
    void functionRange() {

        var source = JsonObjectBuilder.builder()
                .withProperty("range", "${range(`1`, `3`)}")
                .build();

        var expectedArray = new JsonArray();
        expectedArray.add(1);
        expectedArray.add(2);

        var expected = JsonObjectBuilder.builder()
                .withProperty("range", expectedArray)
                .build();

        testSubstitute(source, new JsonObject(), expected);
    }

    @Test
    void functionRangeTooManyNumbers() {
        var source = JsonObjectBuilder.builder()
                .withProperty("range", "${range(`42`, `100500`)}")
                .build();

        testSubstituteThrow(
                source, new JsonObject(),
                "Unable to substitute field \"range\" with expression \"${range" +
                        "(`42`, `100500`)}\": Max numbers allowed numbers is 100. Provided: 100458 (100500 - 42)"
        );
    }

    @Test
    void functionRangeToMoreThanFrom() {
        var source = JsonObjectBuilder.builder()
                .withProperty("range", "${range(`42`, `21`)}")
                .build();

        testSubstituteThrow(
                source, new JsonObject(),
                "Unable to substitute field \"range\" with expression \"${range" +
                        "(`42`, `21`)}\": 'from' (first agr) must be smaller than 'to' (second arg). Provided: 42, 21"
        );
    }


    @Test
    void escapeFullExpression() {
        assertThat(substitute(new JsonPrimitive("$${USER_TIME:0:10}"),
                new JsonObject())).isEqualTo(new JsonPrimitive("${USER_TIME:0:10}"));
    }

    @Test
    void escapePartialExpression() {
        assertThat(substitute(new JsonPrimitive("test $${USER_TIME:0:10}"),
                new JsonObject())).isEqualTo(new JsonPrimitive("test ${USER_TIME:0:10}"));
    }

    @Test
    void testDocumentationSubst() {
        var task = JsonParser.parseString("""
                {"resources": [
                        {
                          "id": 1953762381,
                          "type": "TASK_LOGS",
                          "task_id": 879685986
                        },
                        {
                          "id": 1953762449,
                          "type": "TASK_LOGS",
                          "task_id": 879685991
                        }],
                     "output_params": {
                            "service_endpoints": [
                                {
                                    "ya_deploy_endpoint": {
                                        "cluster_name": "man",
                                        "endpoint_set": {
                                            "endpoint_set_id": "ydeploy.api",
                                            "endpoints": [
                                                {
                                                    "port": 4221
                                                }
                                            ]
                                        },
                                        "deploy_unit": "api"
                                    }
                                }
                            ]
                        }
                    }
                """);

        var source = JsonObjectBuilder.builder()
                .withProperty("title", """
                        Деплой api на порту ${tasks.deploy.output_params.service_endpoints[]\
                        .ya_deploy_endpoint | @[?deploy_unit == 'api'].endpoint_set.endpoints[].port | single(@)}, \
                        логи см. ${tasks.deploy.resources[?type == 'TASK_LOGS'].id | join_any(',', @)}\
                        """)
                .build();

        log.info("Sample source: {}", source);
        var document = JsonObjectBuilder.builder()
                .startMap("tasks")
                .withProperty("deploy", task)
                .end()
                .build();

        testSubstitute(source, document,
                JsonObjectBuilder.builder()
                        .withProperty("title", "Деплой api на порту 4221, логи см. 1953762381,1953762449")
                        .build());
    }

    @Test
    void testCleanupAllPossibleValues() {
        var source = JsonObjectBuilder.builder()
                .withProperty("r1", "${replace.it}")
                .withProperty("r2", " ${replace.it}")
                .withProperty("r3", "${replace.it} ")
                .withProperty("r4", "part ${replace.it}")
                .withProperty("r5", " ${replace.it} part ")
                .withProperty("r6", " test ")
                .build();
        var copy = source.deepCopy();

        // Все полные замены удалены
        // r2 и r3 являются полной заменой (т.е. мы делаем trim)
        assertThat(cleanup(source))
                .isEqualTo(JsonObjectBuilder.builder()
                        .withProperty("r4", "part ${replace.it}")
                        .withProperty("r5", "${replace.it} part")
                        .withProperty("r6", " test ")
                        .build());
        assertThat(source).isEqualTo(copy);
    }

    @Test
    void testCleanupComplex() {
        var source = JsonObjectBuilder.builder()
                .withProperty("key", "start ${a.key} ${b.value} $${escape} ${c} end")
                .build();
        var copy = source.deepCopy();

        assertThat(cleanup(source))
                .isEqualTo(JsonObjectBuilder.builder()
                        .withProperty("key", "start ${a.key} ${b.value} ${escape} ${c} end")
                        .build());
        assertThat(source).isEqualTo(copy);
    }

    @Test
    void testInjectComplexPartial() {
        var source = person("""
                ${`[{"name": "Alice"}, {"name": "Not Alice"}]`[0].name} ${lastName}
                """).build();
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .build();

        testSubstitute(source, document,
                person("Alice Somebody").build());
    }

    @Test
    void testInjectComplexFull() {
        var source = person("""
                ${`[{"name": "Alice"}, {"name": "Not Alice"}]`[0].name}""").build();
        testSubstitute(source, new JsonObject(),
                person("Alice").build());
    }

    @Test
    void testInjectComplexMultilineFull() {
        var source = person("""
                ${
                    `[
                        {"name": "Alice"},
                        {"name": "Not Alice"}
                    ]`[0].name
                }
                    """).build();
        testSubstitute(source, new JsonObject(),
                person("Alice").build());
    }

    @Test
    void testInjectComplexExpression() {
        var source = person("""
                ${(tasks.generate.resources[?type == 'LOG'] | @[?attributes.title == 'Alice'].{"source":
                    {"name": @.attributes.title}, "producer": 'Тест'})[0].source.name}
                    """).build();
        var document = JsonParser.parseString("""
                {
                    "tasks": {
                        "generate": {
                            "resources": [{
                                "type": "LOG",
                                "attributes": {
                                    "title": "Alice"
                                }
                            }]
                        }
                    }
                }
                """).getAsJsonObject();
        testSubstitute(source, document,
                person("Alice").build());
    }

    @Test
    void testInjectComplexObjectExpression() {
        var source = person("""
                ${tasks.generate.resources[?type == 'LOG'] | {"keys": @[].attributes.title} }
                """).build();
        var document = JsonParser.parseString("""
                {
                    "tasks": {
                        "generate": {
                            "resources": [{
                                "type": "LOG",
                                "attributes": {
                                    "title": "Alice"
                                }
                            }]
                        }
                    }
                }
                """).getAsJsonObject();

        var list = JsonParser.parseString("""
                { "keys": ["Alice"] }""");
        testSubstitute(source, document,
                person("")
                        .withProperty("name", list)
                        .build());
    }

    @Test
    void testWithFlowVars() {
        var source = person("""
                ${flow-vars.test_item}""").build();
        var document = JsonParser.parseString("""
                {
                    "flow-vars": {
                        "test_item": "1"
                    }
                }
                """).getAsJsonObject();
        testSubstitute(source, document,
                person("1").build());
    }

    @Test
    void testWithFlowVarsInFilter() {
        var source = person("""
                ${tasks.generate.resources[?type == root().flow-vars.kind] | {"keys": @[].attributes.title} }
                """).build();
        var document = JsonParser.parseString("""
                {
                    "tasks": {
                        "generate": {
                            "resources": [{
                                "type": "LOG",
                                "attributes": {
                                    "title": "Alice"
                                }
                            }]
                        }
                    },
                    "flow-vars": {
                        "kind": "LOG"
                    }
                }
                """).getAsJsonObject();

        var list = JsonParser.parseString("""
                { "keys": ["Alice"] }""");
        testSubstitute(source, document,
                person("")
                        .withProperty("name", list)
                        .build());
    }

    @Test
    void testWithMultiplyByCompliant() {
        var source = person("Alice")
                .withProperty("compliant",
                        "${tasks.*.output_params | @[?has_diff]}")
                .withProperty("non-compliant-but-should-be",
                        "${tasks.*.output_params[?has_diff]}").build();

        var document = JsonParser.parseString("""
                {
                  "tasks": {
                    "run-tag-check-19": {
                      "resources": [
                        {
                          "id": 2405038403,
                          "type": "TASK_LOGS",
                          "task_id": 1066477622
                        }
                      ],
                      "output_params": {
                        "has_diff": false,
                        "hosts_filter_out": "ironsource"
                      }
                    },
                    "run-tag-check-17": {
                      "resources": [
                        {
                          "id": 2405039342,
                          "type": "TASK_LOGS",
                          "task_id": 1066477746
                        }
                      ],
                      "output_params": {
                        "has_diff": true,
                        "hosts_filter_out": "tiktok2-rs2"
                      }
                    },
                    "run-tag-check-18": {
                      "resources": [
                        {
                          "id": 2405039510,
                          "type": "TASK_LOGS",
                          "task_id": 1066477679
                        }
                      ],
                      "output_params": {
                        "has_diff": true,
                        "hosts_filter_out": "tiktok2-rs3"
                      }
                    }
                  }
                }
                """).getAsJsonObject();

        var list = JsonParser.parseString("""
                [
                    {"has_diff":true,"hosts_filter_out":"tiktok2-rs2"},
                    {"has_diff":true,"hosts_filter_out":"tiktok2-rs3"}
                ]""");

        testSubstitute(source, document,
                person("Alice")
                        .withProperty("compliant", list)
                        .withProperty("non-compliant-but-should-be", new JsonArray())
                        .build());

    }

    @Test
    void testResolveNonNullExpression() {
        var source = person("Alice")
                .withProperty("compliant", """
                        ${\
                            not_null(tasks.build-sandbox.resources[?type=='YA_PACKAGE'].\
                            { name: attributes.resource_name, version: attributes.resource_version }, \
                                tasks.build-aw.release_info ) | [0]\
                        }""")
                .build();

        var document = JsonParser.parseString(TestUtils.textResource("jmes/unmatched.json")).getAsJsonObject();

        var compliant = JsonParser.parseString("""
                 {"version":"97.2","name":"s3-goose"}
                """).getAsJsonObject();

        testSubstitute(source, document,
                person("Alice")
                        .withProperty("compliant", compliant)
                        .build());
    }

    @Test
    void testResolveNonNullExpressionWithEmptyList() {
        var source = person("Alice")
                .withProperty("failed", """
                        ${\
                            not_null(tasks.build-sandbox.resources[?type=='YA_PACKAGE'].\
                            { name: attributes.resource_name, version: attributes.resource_version }, [])\
                        }""")
                .build();

        var document = JsonParser.parseString(TestUtils.textResource("jmes/unmatched.json")).getAsJsonObject();

        testSubstituteThrow(source, document, """
                Unable to substitute field "failed" with expression \
                "${    not_null(tasks.build-sandbox.resources[?type=='YA_PACKAGE'].    \
                { name: attributes.resource_name, version: attributes.resource_version }, [])}": \
                No value resolved for expression: not_null(tasks.build-sandbox.resources[?type=='YA_PACKAGE'].    \
                { name: attributes.resource_name, version: attributes.resource_version }, [])""");
    }

    @Test
    void substituteString() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .build();

        assertThat(substituteToString("${lastName}", document, "f"))
                .isEqualTo("Somebody");
    }

    @Test
    void substituteStringInvalid() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", 1)
                .build();

        assertThatThrownBy(() -> substituteToString("${lastName}", document, "f"))
                .hasMessage("Unable to calculate f expression: ${lastName}, must be string, got 1");
    }

    @Test
    void substituteStringOrNumberString() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "Somebody")
                .build();

        assertThat(substituteToStringOrNumber("${lastName}", document, "f"))
                .isEqualTo("Somebody");
    }

    @Test
    void substituteStringOrNumberNumber() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", 1)
                .build();

        assertThat(substituteToStringOrNumber("${lastName}", document, "f"))
                .isEqualTo("1");
    }

    @Test
    void substituteStringOrNumberInvalid() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", true)
                .build();

        assertThatThrownBy(() -> substituteToStringOrNumber("${lastName}", document, "f"))
                .hasMessage("Unable to calculate f expression: ${lastName}, must be string or number, got true");
    }

    @Test
    void substituteNumber() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", 1)
                .build();

        assertThat(substituteToNumber("${lastName}", document, "f"))
                .isEqualTo(1);
    }

    @Test
    void substituteNumberAsString() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "1")
                .build();

        assertThat(substituteToNumber("${lastName}", document, "f"))
                .isEqualTo(1);
    }

    @Test
    void substituteNumberInvalid() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", true)
                .build();

        assertThatThrownBy(() -> substituteToNumber("${lastName}", document, "f"))
                .hasMessage("Unable to calculate f expression: ${lastName}, must be number, got true");
    }

    @Test
    void substituteNumberInvalidString() {
        var document = JsonObjectBuilder.builder()
                .withProperty("lastName", "true")
                .build();

        assertThatThrownBy(() -> substituteToNumber("${lastName}", document, "f"))
                .hasMessage("Unable to calculate f expression: ${lastName}, must be number, got \"true\"");
    }

    @ParameterizedTest
    @CsvSource(textBlock = """
            ${date_format('2022-03-31T10:46:09.196444Z', 'd LLLL yyyy, HH:mm:ss')} ; 31 March 2022, 10:46:09
            ${date_format('2022-03-31T10:46:09.196444Z', 'd LLLL yyyy, HH:mm:ssz')} ; 31 March 2022, 10:46:09Z
            ${date_format('2022-03-31T10:46:09.196444Z', 'd LLLL yyyy, HH:mm:ssz', 'MSK')} ; 31 March 2022, 13:46:09MSK
            ${date_format('2022-03-31T01:00:00Z', 'd LLLL yyyy, HH:mm:ssz', 'GMT-4:30')} ; 30 March 2022, 20:30:00-04:30
            ${date_format('2022-03-31T01:00:00+04:00', 'd LLLL yyyy, HH:mm:ssz')} ; 31 March 2022, 01:00:00+04:00
            ${date_format('2022-03-31T01:00:00+04:00', 'd LLLL yyyy, HH:mm:ssz', 'UTC')} ; 30 March 2022, 21:00:00UTC
            ${date_format(`1648743542`, 'd LLLL yyyy, HH:mm:ssz', 'MSK')} ; 31 March 2022, 19:19:02MSK
            ${date_format('2022-03-31T22:00:15+04:00', 'yyyy_MM_dd z')} ; 2022_03_31 +04:00
            ${date_format('2022-03-31T23:00:15+04:00', 'yyyy_MM_dd z', 'Asia/Yekaterinburg')} ; 2022_04_01 YEKT
            """,
            delimiter = ';'
    )
    void formatDates(String expression, String result) {
        testSubstituteSimple(expression, result);
    }

    private static void testSubstituteThrow(JsonElement source, JsonObject document, String message) {
        assertThatThrownBy(() -> substitute(source, document))
                .hasMessage(message);
    }

    private static void testSubstitute(JsonElement source, JsonObject document, JsonElement expect) {
        testSubstitute(source, DocumentSource.of(document), expect);
    }

    private static void testSubstituteSimple(String source, String expect) {
        testSubstitute(new JsonPrimitive(source), DocumentSource.of(new JsonObject()), new JsonPrimitive(expect));
    }

    private static void testSubstitute(JsonElement source, DocumentSource document, JsonElement expect) {
        var copy = source.deepCopy();

        // Check if can cleanup this expression (i.e. it could be used in our jobs)
        PropertiesSubstitutor.cleanup(source, document);

        assertThat(PropertiesSubstitutor.substitute(source, document)).isEqualTo(expect);
        assertThat(source).isEqualTo(copy);
    }

    private static JsonElement substitute(JsonElement source, JsonObject document) {
        return PropertiesSubstitutor.substitute(source, DocumentSource.of(document));
    }

    private static String substituteToString(String expression, JsonObject document, String field) {
        return PropertiesSubstitutor.substituteToString(expression, DocumentSource.of(document), field);
    }

    private static String substituteToStringOrNumber(String expression, JsonObject document, String field) {
        return PropertiesSubstitutor.substituteToStringOrNumber(expression, DocumentSource.of(document), field);
    }

    private static long substituteToNumber(String expression, JsonObject document, String field) {
        return PropertiesSubstitutor.substituteToNumber(expression, DocumentSource.of(document), field);
    }

    private static JsonElement cleanup(JsonElement source) {
        return PropertiesSubstitutor.cleanup(source);
    }

    private static JsonObjectBuilder<?> person(String name) {
        return JsonObjectBuilder.builder()
                .withProperty("name", name)
                .withProperty("age", 14)
                .withProperty("smart", true);
    }

    private static JsonObjectBuilder<?> documentPerson(String name) {
        return JsonObjectBuilder.builder()
                .withProperty("lastName", name);
    }
}
