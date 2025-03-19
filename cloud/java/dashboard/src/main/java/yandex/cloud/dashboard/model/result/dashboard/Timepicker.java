package yandex.cloud.dashboard.model.result.dashboard;

import com.fasterxml.jackson.databind.PropertyNamingStrategy;
import com.fasterxml.jackson.databind.annotation.JsonNaming;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
@JsonNaming(PropertyNamingStrategy.SnakeCaseStrategy.class)
public class Timepicker {
    List<String> refreshIntervals;
    List<String> timeOptions;
}
