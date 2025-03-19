package yandex.cloud.dashboard.model.result.dashboard;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class TemplateVariable {
    @JsonInclude
    String allValue;
    Item current;
    int hide;
    boolean includeAll;
    @JsonInclude
    String label;
    boolean multi;
    String name;
    List<Item> options;
    String query;
    boolean skipUrlSync;
    String type;
    // query-specific
    String datasource;
    Integer refresh;
    String regex;
    Integer sort;

    @AllArgsConstructor
    @Data
    public static class Item {
        boolean selected;
        List<Void> tags;
        String text;
        String value;

        public Item(String text, String value) {
            this(false, null, text, value);
        }
    }
}
