package yandex.cloud.team.integration.abc;

import java.util.Locale;
import java.util.regex.Pattern;

import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;

@Log4j2
public class NamePolicyServiceImpl implements NamePolicyService {

    private static final Pattern NAME_NOT_ALLOWED = Pattern.compile("[^a-z0-9]+");
    private static final Pattern LABEL_NOT_ALLOWED = Pattern.compile("[^_a-z0-9]+");

    private static final int MAX_NAME_LENGTH = 63;
    private static final int MAX_LABEL_LENGTH = 63;

    private static final String[] DIGITS = {
            "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
    };

    /**
     * Removes any violations to make the given string matches the requirements for the cloud/folder name.
     * The pattern is {@code [a-z]([-a-z0-9]{0,61}[a-z0-9])}. The first character must be a letter,
     * then a few letters, numbers, or dashes, and the last one should be a letter or number.
     * For example, "_5-event__promo__2019" becomes "five-event-promo-2019", "__wiki__" becomes "wiki".
     *
     * @param name string to normalize
     * @return normalized string
     */
    @Override
    public String normalizeName(
            @NotNull String name
    ) {
        var normalized = name.toLowerCase(Locale.ENGLISH);

        // Replace all illegal symbols with dashes
        normalized = NAME_NOT_ALLOWED.matcher(normalized).replaceAll("-");

        // Remove the leading dash
        if (normalized.charAt(0) == '-') {
            normalized = normalized.substring(1);
        }

        if (normalized.length() > 0) {
            // Replace a leading digit with its name
            var first = normalized.charAt(0);
            if (first >= '0' && first <= '9') {
                normalized = DIGITS[first - '0'] + normalized.substring(1);
            }

            // Ensure that the name will fit
            if (normalized.length() > MAX_NAME_LENGTH) {
                normalized = normalized.substring(0, MAX_NAME_LENGTH);
            }

            // Remove the trailing dash if present
            var length = normalized.length();
            if (normalized.charAt(length - 1) == '-') {
                normalized = normalized.substring(0, length - 1);
            }
        }

        log.trace("Normalized name for '{}' is '{}'", name, normalized);
        return normalized;
    }

    /**
     * Removes any violations to make the given string matches the requirements for the label name.
     * The pattern is {@code [-_a-z0-9]{0,63}}. Only letters, numbers, underscores or dashes are allowed.
     * For example, "5-event__promo__2019." becomes "5-event__promo__2019-".
     *
     * @param label string to normalize
     * @return normalized string
     */
    @Override
    public String normalizeLabel(
            @NotNull String label
    ) {
        var normalized = label.toLowerCase(Locale.ENGLISH);

        // Replace all illegal symbols with dashes
        normalized = LABEL_NOT_ALLOWED.matcher(normalized).replaceAll("-");

        // Ensure that the label will fit
        if (normalized.length() > MAX_LABEL_LENGTH) {
            normalized = normalized.substring(0, MAX_LABEL_LENGTH);
        }

        log.trace("Normalized label for '{}' is '{}'", label, normalized);
        return normalized;
    }

}
