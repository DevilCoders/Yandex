package ru.yandex.ci.util;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class OverriderTest {
    private Person base;

    private Person override;

    @BeforeEach
    public void setUp() {
        base = person();
        override = person();
    }

    private static OverriderTest.Person person() {
        return new Person();
    }

    @Test
    void inBaseNotOverride() {
        base.setName("Monica");
        Person result = base.override(override);
        assertThat(result.getName()).isEqualTo("Monica");
    }

    @Test
    void notBaseInOverride() {
        override.setName("Monica");
        Person result = base.override(override);
        assertThat(result.getName()).isEqualTo("Monica");
    }

    @Test
    void inBaseInOverride() {
        base.setName("Nata");
        override.setName("Monica");
        Person result = base.override(override);
        assertThat(result.getName()).isEqualTo("Monica");
    }

    @Test
    void inBaseNotOverrideNested() {
        base.setFather(person().setName("Chandler").setAge(41));
        override.setFather(person());

        Person result = base.override(override);
        assertThat(result.getFather().getName()).isEqualTo("Chandler");
        assertThat(result.getFather().getAge()).isEqualTo(41);
    }

    @Test
    void inBaseNullOverrideNested() {
        base.setFather(person().setName("Chandler").setAge(41));

        Person result = base.override(override);
        assertThat(result.getFather().getName()).isEqualTo("Chandler");
        assertThat(result.getFather().getAge()).isEqualTo(41);
    }

    @Test
    void notBaseInOverrideNested() {
        base.setFather(person().setAge(41));
        override.setFather(person().setName("Ross"));

        Person result = base.override(override);
        assertThat(result.getFather().getName()).isEqualTo("Ross");
        assertThat(result.getFather().getAge()).isEqualTo(41);
    }

    @Test
    void nullBaseInOverrideNested() {
        override.setFather(person().setName("Ross"));

        Person result = base.override(override);
        assertThat(result.getFather().getName()).isEqualTo("Ross");
        assertThat(result.getFather().getAge()).isNull();
    }

    @Test
    void inBaseInOverrideNested() {
        base.setFather(person().setName("Joey").setAge(41));
        override.setFather(person().setName("Ross"));

        Person result = base.override(override);
        assertThat(result.getFather().getName()).isEqualTo("Ross");
        assertThat(result.getFather().getAge()).isEqualTo(41);
        assertThat(result.getFriends()).isNull();
    }

    @Test
    void inBaseNotOverrideCollection() {
        base.setFriends(List.of(
                person().setName("Monica"),
                person().setName("Rachel"),
                person().setName("Phoebe")
        ));

        Person result = base.override(override);

        assertThat(result.getFriends())
                .extracting(Person::getName)
                .containsExactly("Monica", "Rachel", "Phoebe");
    }

    @Test
    void notBaseInOverrideCollection() {
        override.setFriends(List.of(
                person().setName("Monica"),
                person().setName("Rachel"),
                person().setName("Phoebe")
        ));

        Person result = base.override(override);

        assertThat(result.getFriends())
                .extracting(Person::getName)
                .containsExactly("Monica", "Rachel", "Phoebe");
    }

    @Test
    void overrideDeclaresEmptyCollection() {
        base.setFriends(List.of(
                person().setName("Monica"),
                person().setName("Rachel"),
                person().setName("Phoebe")
        ));
        override.setFriends(List.of());

        Person result = base.override(override);

        assertThat(result.getFriends()).isNotNull().isEmpty();
    }

    @Test
    void overrideCollectionFill() {
        base.setFriends(List.of(
                person().setName("Monica"),
                person().setName("Rachel"),
                person().setName("Phoebe")
        ));
        override.setFriends(List.of(
                person().setName("Joey"),
                person().setName("Ross"),
                person().setName("Chandler")
        ));

        Person result = base.override(override);

        assertThat(result.getFriends())
                .extracting(Person::getName)
                .containsExactly("Joey", "Ross", "Chandler");
    }

    private static class Person implements Overridable<Person> {
        private String name;
        private Integer age;
        private Person father;
        private Person mother;
        private List<Person> friends;

        public String getName() {
            return name;
        }

        public Person setName(String name) {
            this.name = name;
            return this;
        }

        public Person getFather() {
            return father;
        }

        public Integer getAge() {
            return age;
        }

        public Person setAge(Integer age) {
            this.age = age;
            return this;
        }

        public Person setFather(Person father) {
            this.father = father;
            return this;
        }

        public Person getMother() {
            return mother;
        }

        public Person setMother(Person mother) {
            this.mother = mother;
            return this;
        }

        public List<Person> getFriends() {
            return friends;
        }

        public Person setFriends(List<Person> friends) {
            this.friends = friends;
            return this;
        }

        @Override
        public Person override(Person override) {
            Person copy = new Person();
            Overrider<Person> overrider = new Overrider<>(this, override);
            overrider.field(copy::setName, Person::getName);
            overrider.field(copy::setAge, Person::getAge);
            overrider.fieldDeep(copy::setFather, Person::getFather);
            overrider.fieldDeep(copy::setMother, Person::getMother);
            overrider.collection(copy::setFriends, Person::getFriends);
            return copy;
        }
    }
}

