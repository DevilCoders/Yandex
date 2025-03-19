package yandex.cloud.dashboard.model.result.generic;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class RGBA {
    private static final DecimalFormat ALPHA_FORMAT = new DecimalFormat("#.##", DecimalFormatSymbols.getInstance(Locale.US));

    int r;
    int g;
    int b;
    double a;

    @JsonValue
    public String serialize() {
        return String.format("rgba(%d, %d, %d, %s)", r, g, b, ALPHA_FORMAT.format(a));
    }
}
