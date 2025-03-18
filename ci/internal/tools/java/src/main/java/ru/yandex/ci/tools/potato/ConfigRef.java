package ru.yandex.ci.tools.potato;

import lombok.Value;

@Value
class ConfigRef {
    String source;
    Type type;
    String path;
    String content;

    public enum Type {
        YAML(".potato.yml", ".yaml"), JSON(".potato.json", ".json");
        private final String filename;
        private final String ext;

        Type(String filename, String ext) {
            this.filename = filename;
            this.ext = ext;
        }

        public String getFilename() {
            return filename;
        }

        public String getExt() {
            return ext;
        }
    }
}
