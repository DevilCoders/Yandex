package ru.yandex.ci.storage.core.doc;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.storage.core.db.CiStorageEntities;
import ru.yandex.ci.storage.core.db.annotations.IndirectReference;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

/**
 * brew install graphviz
 * dot -Tpng < test.txt > simple.png
 */
@Slf4j
public class GenerateERDiagram {

    @Test
    public void generate() throws IOException {
        var entities = new ArrayList<>(CiStorageEntities.getStorageEntities());
        entities.add(TestEntity.class);

        var entityIdClassToEntity = new HashMap<Class<? extends Entity.Id>, Class<? extends Entity>>();
        for (var entity : entities) {
            Arrays.stream(entity.getClasses())
                    .filter(Entity.Id.class::isAssignableFrom)
                    .findFirst()
                    .ifPresent(idClass -> entityIdClassToEntity.put((Class<? extends Entity.Id>) idClass, entity));
        }

        var links = new HashSet<String>();

        var result = new StringBuilder();
        result.append("digraph G {\n")
                .append("node [shape=none, margin=0]\n")
                .append("edge [arrowhead=open, arrowtail=none, dir=both]\n");

        for (var entry : entityIdClassToEntity.entrySet()) {
            var entity = entry.getValue();
            result.append("\n")
                    .append(entity.getSimpleName())
                    .append(" [label=<\n")
                    .append("<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\">\n");

            var table = entity.getAnnotation(Table.class);
            result.append("<tr><td bgcolor=\"lightblue\">")
                    .append(table != null ? table.name() : entity.getSimpleName())
                    .append("</td></tr>\n");

            var idFields = Arrays.asList(entry.getKey().getDeclaredFields());
            processFields("id_", entityIdClassToEntity, links, result, entity, idFields);

            var fields = Arrays.stream(entity.getDeclaredFields())
                    .filter(x -> x.getType() != entry.getKey())
                    .collect(Collectors.toList());

            processFields("", entityIdClassToEntity, links, result, entity, fields);

            result.append("</table>");
            result.append(">];\n\n");
        }

        result.append(String.join(";\n", links));
        result.append("}\n");
        var file = File.createTempFile("erd", ".dot");
        Files.writeString(file.toPath(), result.toString());
        log.info("Execute: dot -Tpng < {} > storage_er.png", file.getAbsolutePath());
    }

    private void processFields(
            String prefix,
            HashMap<Class<? extends Entity.Id>, Class<? extends Entity>> entityIdClassToEntity,
            Set<String> links,
            StringBuilder result,
            Class<? extends Entity> entity,
            List<Field> fields
    ) {
        for (var field : fields) {
            if (Modifier.isStatic(field.getModifiers())) {
                continue;
            }

            var indirectReference = field.getAnnotation(IndirectReference.class);
            var reference = entityIdClassToEntity.get(
                    indirectReference == null ? field.getType() : indirectReference.id()
            );
            if (reference != null) {
                links.add(entity.getSimpleName() + "->" + reference.getSimpleName());
            }

            var typeName = indirectReference == null && reference != null ?
                    reference.getSimpleName() + "." + field.getType().getSimpleName() :
                    field.getType().getSimpleName();

            result.append("<tr><td align=\"left\">")
                    .append(prefix)
                    .append(field.getName())
                    .append(": ")
                    .append(typeName)
                    .append("</td></tr>");
        }
    }
}
