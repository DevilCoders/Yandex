package yandex.cloud.dashboard.model.spec.panel;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.panel.DisplaySpec.LineModesSpec.LineMode;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;
import yandex.cloud.dashboard.util.Mergeable;

import java.util.EnumSet;
import java.util.List;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author ssytnik
 */
@With
@Value
public class DisplaySpec implements Spec, Mergeable<DisplaySpec> {
    public static final DisplaySpec DEFAULT = new DisplaySpec(
            LegendSpec.DEFAULT, null, null, SortSpec.DEFAULT, false, null, null, null, null);

    LegendSpec legend;
    Integer decimals; // affects legend and tooltip
    Boolean empty;
    SortSpec sort;
    Boolean stack;
    LineModesSpec lineModes;
    Integer lineWidth;
    Integer fill;
    NullPointMode nulls;

    @Override
    public DisplaySpec merge(DisplaySpec lowerPrecedence) {
        return new DisplaySpec(
                firstNonNullOrNull(legend, lowerPrecedence.legend),
                firstNonNullOrNull(decimals, lowerPrecedence.decimals),
                firstNonNullOrNull(empty, lowerPrecedence.empty),
                firstNonNullOrNull(sort, lowerPrecedence.sort),
                firstNonNullOrNull(stack, lowerPrecedence.stack),
                firstNonNullOrNull(lineModes, lowerPrecedence.lineModes),
                firstNonNullOrNull(lineWidth, lowerPrecedence.lineWidth),
                firstNonNullOrNull(fill, lowerPrecedence.fill),
                firstNonNullOrNull(nulls, lowerPrecedence.nulls)
        );
    }


    public boolean containsLineMode(LineMode mode) {
        return firstNonNullOrNull(lineModes, LineModesSpec.DEFAULT).contains(mode);
    }

    @With
    @Value
    public static class LineModesSpec implements Spec {
        public static final LineModesSpec DEFAULT = new LineModesSpec(LineMode.lines);
        List<LineMode> modes;

        @JsonCreator
        public LineModesSpec(LineMode mode) {
            this(List.of(mode));
        }

        @JsonCreator
        public LineModesSpec(List<LineMode> modes) {
            this.modes = modes;
        }

        @Override
        public void validate(SpecValidationContext context) {
            Preconditions.checkState(EnumSet.copyOf(modes).size() == modes.size(), "Duplicate modes found: %s", modes);
        }

        public boolean contains(LineMode mode) {
            return modes.contains(mode);
        }

        public enum LineMode {
            bars, lines, points
        }
    }

    @Value
    public static class SortSpec {
        public static final SortSpec DEFAULT = new SortSpec(SortMode.none);
        SortMode mode;

        @JsonCreator
        public SortSpec(boolean sort) {
            this.mode = sort ? SortMode.increasing : SortMode.none;
        }

        @JsonCreator
        public SortSpec(SortMode mode) {
            this.mode = mode;
        }

        @AllArgsConstructor
        public enum SortMode {
            none(0),
            increasing(1),
            decreasing(2);

            @Getter
            int value;
        }
    }

    @AllArgsConstructor
    public enum NullPointMode {
        keep("null"),
        zero("null as zero"),
        connected("connected");

        String value;

        //        @JsonValue
        public String serialize() {
            return value;
        }
    }

    @With
    @Value
    public static class LegendSpec implements Spec {
        public static final LegendSpec DEFAULT = new LegendSpec(true);

        Side side;

        @JsonCreator
        public LegendSpec(boolean show) {
            this(show ? Side.bottom : null);
        }

        @JsonCreator
        public LegendSpec(Side side) {
            this.side = side;
        }

        public boolean isShow() {
            return side != null;
        }

        public Boolean isRightSide() {
            return Side.right == side ? true : null;
        }

        public enum Side {
            bottom, right
        }
    }
}
