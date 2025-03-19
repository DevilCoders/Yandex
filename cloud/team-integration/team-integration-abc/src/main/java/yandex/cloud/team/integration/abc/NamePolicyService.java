package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.NotNull;

/**
 * Service to adjust cloud/folder attributes to satisfy the Yandex.Cloud requirements.
 */
public interface NamePolicyService {

    /**
     * Removes any violations to make the given string matches the requirements for the cloud/folder name.
     * The pattern is {@code [a-z]([-a-z0-9]{0,61}[a-z0-9])}. The first character must be a letter,
     * then a few letters, numbers, or dashes, and the last one should be a letter or number.
     * For example, "_5-event__promo__2019" becomes "five-event-promo-2019", "__wiki__" becomes "wiki".
     *
     * @param name string to normalize
     * @return normalized string
     */
    String normalizeName(
            @NotNull String name
    );

    /**
     * Removes any violations to make the given string matches the requirements for the label name.
     * The pattern is {@code [-_a-z0-9]{0,63}}. Only letters, numbers, underscores or dashes are allowed.
     * For example, "5-event__promo__2019." becomes "5-event__promo__2019-".
     *
     * @param label string to normalize
     * @return normalized string
     */
    String normalizeLabel(
            @NotNull String label
    );

}
