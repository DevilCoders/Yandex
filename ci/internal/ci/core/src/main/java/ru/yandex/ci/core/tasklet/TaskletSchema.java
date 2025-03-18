package ru.yandex.ci.core.tasklet;

import java.util.List;

import lombok.ToString;

import ru.yandex.ci.core.job.JobResourceType;

@ToString
public class TaskletSchema {
    private final List<Field> input;
    private final List<Field> output;

    public TaskletSchema(List<Field> input, List<Field> output) {
        this.input = List.copyOf(input);
        this.output = List.copyOf(output);
    }

    public List<Field> getInput() {
        return input;
    }

    public List<Field> getOutput() {
        return output;
    }

    @ToString
    public static class Field {
        private final JobResourceType resourceType;
        private final boolean repeated;

        private Field(JobResourceType resourceType, boolean repeated) {
            this.resourceType = resourceType;
            this.repeated = repeated;
        }

        public static Field regular(JobResourceType resourceType) {
            return new Field(resourceType, false);
        }

        public static Field repeated(JobResourceType resourceType) {
            return new Field(resourceType, true);
        }

        public JobResourceType getResourceType() {
            return resourceType;
        }

        public boolean isRepeated() {
            return repeated;
        }
    }
}
