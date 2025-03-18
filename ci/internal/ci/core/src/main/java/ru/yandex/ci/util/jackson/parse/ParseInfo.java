package ru.yandex.ci.util.jackson.parse;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonIgnoreType;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@JsonIgnoreType
@Value
public class ParseInfo {
    @Nonnull
    String path;

    /**
     * Возвращает путь к узлу
     *
     * @return путь к узлу в документе json/yaml, из которого получен текущий объект.
     */
    public String getParsedPath() {
        return path;
    }

    /**
     * Возвращает путь до узла
     *
     * @param property значение, которое нужно добавить к узлу
     * @return возвращает путь до узла в документе json/yaml, из которого получено поле текущего объекта
     */
    public String getParsedPath(String property) {
        return this.path + "/" + property;
    }
}
