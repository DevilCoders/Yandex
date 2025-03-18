#!/usr/bin/env python2

import textwrap


class LabelsTemplate(object):
    def __init__(self, count):
        self._count = count
        self._class_name = "Labels%d" % count
        self._w = open("%s.java" % self._class_name, "w")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, tb):
        self._w.close()

    def imports(self):
        self._write_dedent("""
            package ru.yandex.monlib.metrics.labels.impl;

            import java.util.Arrays;
            import java.util.Collections;
            import java.util.HashMap;
            import java.util.List;
            import java.util.Map;
            import java.util.function.Consumer;
            import java.util.stream.Stream;

            import javax.annotation.Nullable;
            import javax.annotation.ParametersAreNonnullByDefault;

            import ru.yandex.monlib.metrics.labels.Label;
            import ru.yandex.monlib.metrics.labels.Labels;
            import ru.yandex.monlib.metrics.labels.LabelsBuilder;
            import ru.yandex.monlib.metrics.labels.LabelsBuilder.SortState;
            import ru.yandex.monlib.metrics.labels.validate.LabelsValidator;
        """)

    def class_begin(self):
        self._write_dedent("""
            /**
             * This class is automatically generated. Do not edit it!
             */
            @ParametersAreNonnullByDefault
            public final class %s extends Labels {
        """ % self._class_name)
        if self._count == 0:
            self._write("\n    public static final %s SELF = new %s();\n\n" % (
                self._class_name, self._class_name))

    def class_end(self):
        self._write("}")

    def fields(self):
        if self._count:
            self._write("\n")
            for n in xrange(self._count):
                self._write("    private final Label label%d;\n" % n)
            self._write("\n")

    def constructors(self):
        if self._count:
            params = ", ".join(["Label label%d" % n for n in xrange(self._count)])
            self._write("    public %s(%s) {\n" % (self._class_name, params))
            for n in xrange(self._count):
                self._write("        this.label%d = label%d;\n" % (n, n))
            self._write("    }\n\n")

            self._write("    public %s(LabelsBuilder builder) {\n" % self._class_name)
            for n in xrange(self._count):
                self._write("        this.label%d = builder.at(%d);\n" % (n, n))
            self._write("    }\n\n")


    def size(self):
        self._write(
            "    @Override\n"
            "    public int size() {\n"
            "        return %d;\n"
            "    }\n\n" % self._count)

    def is_empty(self):
        self._write(
            "    @Override\n"
            "    public boolean isEmpty() {\n"
            "        return %s;\n"
            "    }\n\n" % ("false" if self._count else "true"))

    def at(self):
        self._write(
            "    @Override\n"
            "    public Label at(int index) {\n")
        if self._count:
            self._write("        checkIndex(index);\n")
            self._write("        switch (index) {\n")
            for n in xrange(self._count):
                self._write("            case %d: return label%d;\n" % (n, n))
            self._write("        }\n")
        else:
            self._write("        checkIndex(index);\n")
        self._write("        return null; // never get here\n")
        self._write("    }\n\n")

    def for_each(self):
        self._write(
            "    @Override\n"
            "    public void forEach(Consumer<Label> c) {\n")
        for n in xrange(self._count):
            self._write("        c.accept(label%d);\n" % n)
        self._write("    }\n\n")

    def stream(self):
        self._write(
            "    @Override\n"
            "    public Stream<Label> stream() {\n")
        if self._count:
            params = ", ".join(["label%d" % n for n in xrange(self._count)])
            self._write("        return Stream.of(%s);\n" % params)
        else:
            self._write("        return Stream.empty();\n")
        self._write("    }\n\n")

    def index_by_key(self):
        self._write(
            "    @Override\n"
            "    public int indexByKey(String key) {\n"
            "        LabelsValidator.checkKeyValid(key);\n")
        for n in xrange(self._count):
            self._write("        if (label%d.hasKey(key)) return %d;\n" % (n, n))
        self._write(
            "        return -1;\n"
            "    }\n\n")

    def index_by_same_key(self):
        self._write(
            "    @Override\n"
            "    public int indexBySameKey(Label label) {\n")
        for n in xrange(self._count):
            self._write("        if (label%d.hasSameKey(label)) return %d;\n" % (n, n))
        self._write(
            "        return -1;\n"
            "    }\n\n")

    def find_by_key(self):
        self._write(
            "    @Override\n"
            "    @Nullable\n"
            "    public Label findByKey(String key) {\n"
            "        LabelsValidator.checkKeyValid(key);\n")
        if self._count > 0:
            for n in xrange(self._count):
                self._write("        if (label%d.hasKey(key)) return label%d;\n" % (n, n))
        self._write(
            "        return null;\n"
            "    }\n\n")

    def find_by_same_key(self):
        self._write(
            "    @Override\n"
            "    @Nullable\n"
            "    public Label findBySameKey(Label label) {\n")
        if self._count > 0:
            for n in xrange(self._count):
                self._write("        if (label%d.hasSameKey(label)) return label%d;\n" % (n, n))
        self._write(
            "        return null;\n"
            "    }\n\n")

    def to_builder(self):
        self._write(
            "    @Override\n"
            "    public LabelsBuilder toBuilder() {\n")
        if self._count > 0:
            labels = ", ".join(["label%d" % n for n in xrange(self._count)])
            self._write("        return new LabelsBuilder(SortState.SORTED, %s);\n" % labels)
        else:
            self._write("        return new LabelsBuilder(SortState.SORTED);\n")
        self._write("    }\n\n")

    def to_map(self):
        self._write(
            "    @Override\n"
            "    public Map<String, String> toMap() {\n")
        if self._count == 0:
            self._write("        return new HashMap<>(0);\n")
        else:
            self._write("        Map<String, String> map = new HashMap<>(size());\n")
            for n in xrange(self._count):
                self._write("        map.put(label%d.getKey(), label%d.getValue());\n" % (n, n))
            self._write("        return map;\n")
        self._write("    }\n\n")

    def to_list(self):
        self._write(
            "    @Override\n"
            "    public List<Label> toList() {\n")
        if self._count == 0:
            self._write("        return Collections.emptyList();\n")
        elif self._count == 1:
            self._write("        return Collections.singletonList(label0);\n")
        else:
            labels = ", ".join(["label%d" % n for n in xrange(self._count)])
            self._write("        return Arrays.asList(%s);\n" % labels)
        self._write("    }\n\n")

    def copy_into(self):
        self._write(
            "    @Override\n"
            "    public void copyInto(Label[] array) {\n")
        for n in xrange(self._count):
            self._write("        array[%d] = label%d;\n" % (n, n))
        self._write("    }\n\n")


    def equals(self):
        self._write(
            "    @Override\n"
            "    public boolean equals(Object o) {\n"
            "        if (this == o) return true;\n")
        if self._count == 0:
            self._write("        return o != null && getClass() == o.getClass();\n")
        else:
            self._write("        if (o == null || getClass() != o.getClass()) return false;\n")
            self._write("        Labels%d rhs = (Labels%d) o;\n" % (self._count, self._count))
            for n in xrange(self._count - 1):
                self._write("        if (!label%d.equals(rhs.label%d)) return false;\n" % (n, n))
            last_n = self._count - 1
            self._write("        return label%d.equals(rhs.label%d);\n" % (last_n, last_n))
        self._write("    }\n\n")

    def hash_code(self):
        self._write(
            "    @Override\n"
            "    public int hashCode() {\n")
        if self._count == 0:
            self._write("        return 31;\n")
        elif self._count == 1:
            self._write("        return label0.hashCode();\n")
        else:
            self._write("        int result = label0.hashCode();\n")
            for n in xrange(1, self._count):
                self._write("        result = 31 * result + label%d.hashCode();\n" % n)
            self._write("        return result;\n")
        self._write("    }\n\n")

    def _write_dedent(self, text):
        self._w.write(textwrap.dedent(text))

    def _write(self, text):
        self._w.write(text)


def main():
    for n in xrange(17):
        with LabelsTemplate(n) as t:
            t.imports()
            t.class_begin()
            t.fields()
            t.constructors()
            t.size()
            t.is_empty()
            t.at()
            t.for_each()
            t.stream()
            t.index_by_key()
            t.index_by_same_key()
            t.find_by_key()
            t.find_by_same_key()
            t.to_builder()
            t.to_map()
            t.to_list()
            t.copy_into()
            t.equals()
            t.hash_code()
            t.class_end()


if __name__ == "__main__":
    main()
