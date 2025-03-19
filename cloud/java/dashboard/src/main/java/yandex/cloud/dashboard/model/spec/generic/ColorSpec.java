package yandex.cloud.dashboard.model.spec.generic;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.collect.ImmutableMap;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;

import java.beans.ConstructorProperties;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.stream.Collectors;

import static com.google.common.base.Preconditions.checkArgument;
import static yandex.cloud.dashboard.model.spec.generic.ColorParser.parseColor;
import static yandex.cloud.dashboard.util.ObjectUtils.takeOrDefault;

/**
 * @author ssytnik
 */
@With
@Value
public class ColorSpec implements Spec {
    private static final Map<String, ColorSpec> library = ImmutableMap.<String, String>builder()
            .put("indianred", "#cd5c5c")
            .put("lightcoral", "#f08080")
            .put("salmon", "#fa8072")
            .put("darksalmon", "#e9967a")
            .put("crimson", "#dc143c")
            .put("red", "#ff0000")
            .put("firebrick", "#b22222")
            .put("darkred", "#8b0000")
            .put("pink", "#ffc0cb")
            .put("lightpink", "#ffb6c1")
            .put("hotpink", "#ff69b4")
            .put("deeppink", "#ff1493")
            .put("mediumvioletred", "#c71585")
            .put("palevioletred", "#db7093")
            .put("lightsalmon", "#ffa07a")
            .put("coral", "#ff7f50")
            .put("tomato", "#ff6347")
            .put("orangered", "#ff4500")
            .put("darkorange", "#ff8c00")
            .put("orange", "#ffa500")
            .put("gold", "#ffd700")
            .put("yellow", "#ffff00")
            .put("lightyellow", "#ffffe0")
            .put("lemonchiffon", "#fffacd")
            .put("lightgoldenrodyellow", "#fafad2")
            .put("papayawhip", "#ffefd5")
            .put("moccasin", "#ffe4b5")
            .put("peachpuff", "#ffdab9")
            .put("palegoldenrod", "#eee8aa")
            .put("khaki", "#f0e68c")
            .put("darkkhaki", "#bdb76b")
            .put("lavender", "#e6e6fa")
            .put("thistle", "#d8bfd8")
            .put("plum", "#dda0dd")
            .put("violet", "#ee82ee")
            .put("orchid", "#da70d6")
            .put("fuchsia", "#ff00ff")
            .put("magenta", "#ff00ff")
            .put("mediumorchid", "#ba55d3")
            .put("mediumpurple", "#9370db")
            .put("blueviolet", "#8a2be2")
            .put("darkviolet", "#9400d3")
            .put("darkorchid", "#9932cc")
            .put("darkmagenta", "#8b008b")
            .put("purple", "#800080")
            .put("indigo", "#4b0082")
            .put("slateblue", "#6a5acd")
            .put("darkslateblue", "#483d8b")
            .put("cornsilk", "#fff8dc")
            .put("blanchedalmond", "#ffebcd")
            .put("bisque", "#ffe4c4")
            .put("navajowhite", "#ffdead")
            .put("wheat", "#f5deb3")
            .put("burlywood", "#deb887")
            .put("tan", "#d2b48c")
            .put("rosybrown", "#bc8f8f")
            .put("sandybrown", "#f4a460")
            .put("goldenrod", "#daa520")
            .put("darkgoldenrod", "#b8860b")
            .put("peru", "#cd853f")
            .put("chocolate", "#d2691e")
            .put("saddlebrown", "#8b4513")
            .put("sienna", "#a0522d")
            .put("brown", "#a52a2a")
            .put("maroon", "#800000")
            .put("greenyellow", "#adff2f")
            .put("chartreuse", "#7fff00")
            .put("lawngreen", "#7cfc00")
            .put("lime", "#00ff00")
            .put("limegreen", "#32cd32")
            .put("palegreen", "#98fb98")
            .put("lightgreen", "#90ee90")
            .put("mediumspringgreen", "#00fa9a")
            .put("springgreen", "#00ff7f")
            .put("mediumseagreen", "#3cb371")
            .put("seagreen", "#2e8b57")
            .put("forestgreen", "#228b22")
            .put("green", "#008000")
            .put("darkgreen", "#006400")
            .put("yellowgreen", "#9acd32")
            .put("olivedrab", "#6b8e23")
            .put("olive", "#808000")
            .put("darkolivegreen", "#556b2f")
            .put("mediumaquamarine", "#66cdaa")
            .put("darkseagreen", "#8fbc8f")
            .put("lightseagreen", "#20b2aa")
            .put("darkcyan", "#008b8b")
            .put("teal", "#008080")
            .put("aqua", "#00ffff")
            .put("cyan", "#00ffff")
            .put("lightcyan", "#e0ffff")
            .put("paleturquoise", "#afeeee")
            .put("aquamarine", "#7fffd4")
            .put("turquoise", "#40e0d0")
            .put("mediumturquoise", "#48d1cc")
            .put("darkturquoise", "#00ced1")
            .put("cadetblue", "#5f9ea0")
            .put("steelblue", "#4682b4")
            .put("lightsteelblue", "#b0c4de")
            .put("powderblue", "#b0e0e6")
            .put("lightblue", "#add8e6")
            .put("skyblue", "#87ceeb")
            .put("lightskyblue", "#87cefa")
            .put("deepskyblue", "#00bfff")
            .put("dodgerblue", "#1e90ff")
            .put("cornflowerblue", "#6495ed")
            .put("mediumslateblue", "#7b68ee")
            .put("royalblue", "#4169e1")
            .put("blue", "#0000ff")
            .put("mediumblue", "#0000cd")
            .put("darkblue", "#00008b")
            .put("navy", "#000080")
            .put("midnightblue", "#191970")
            .put("white", "#ffffff")
            .put("snow", "#fffafa")
            .put("honeydew", "#f0fff0")
            .put("mintcream", "#f5fffa")
            .put("azure", "#f0ffff")
            .put("aliceblue", "#f0f8ff")
            .put("ghostwhite", "#f8f8ff")
            .put("whitesmoke", "#f5f5f5")
            .put("seashell", "#fff5ee")
            .put("beige", "#f5f5dc")
            .put("oldlace", "#fdf5e6")
            .put("floralwhite", "#fffaf0")
            .put("ivory", "#fffff0")
            .put("antiquewhite", "#faebd7")
            .put("linen", "#faf0e6")
            .put("lavenderblush", "#fff0f5")
            .put("mistyrose", "#ffe4e1")
            .put("gainsboro", "#dcdcdc")
            .put("lightgrey", "#d3d3d3")
            .put("lightgray", "#d3d3d3")
            .put("silver", "#c0c0c0")
            .put("darkgray", "#a9a9a9")
            .put("darkgrey", "#a9a9a9")
            .put("gray", "#808080")
            .put("grey", "#808080")
            .put("dimgray", "#696969")
            .put("dimgrey", "#696969")
            .put("lightslategray", "#778899")
            .put("lightslategrey", "#778899")
            .put("slategray", "#708090")
            .put("slategrey", "#708090")
            .put("darkslategray", "#2f4f4f")
            .put("darkslategrey", "#2f4f4f")
            .put("black", "#000000")
            .build().entrySet().stream().collect(Collectors.toMap(Entry::getKey, e -> parseColor(e.getValue())));

    public static final ColorSpec BLACK = of("black");
    public static final ColorSpec GRAY = of("gray");
    public static final ColorSpec SILVER = of("silver");
    public static final ColorSpec WHITE = of("white");
    public static final ColorSpec FUCHSIA = of("fuchsia");
    public static final ColorSpec PURPLE = of("purple");
    public static final ColorSpec RED = of("red");
    public static final ColorSpec MAROON = of("maroon");
    public static final ColorSpec YELLOW = of("yellow");
    public static final ColorSpec OLIVE = of("olive");
    public static final ColorSpec LIME = of("lime");
    public static final ColorSpec GREEN = of("green");
    public static final ColorSpec AQUA = of("aqua");
    public static final ColorSpec TEAL = of("teal");
    public static final ColorSpec BLUE = of("blue");
    public static final ColorSpec NAVY = of("navy");

    int r;
    int g;
    int b;

    @JsonCreator
    @ConstructorProperties({"r", "g", "b"})
    public ColorSpec(int r, int g, int b) {
        List.of(r, g, b).forEach(v -> checkArgument(v >= 0 && v <= 0xff, "Component is out of range: '%s'", v));
        this.r = r;
        this.g = g;
        this.b = b;
    }

    @JsonCreator
    public static ColorSpec of(String color) {
        return takeOrDefault(library.get(color), () -> parseColor(color));
    }
}
