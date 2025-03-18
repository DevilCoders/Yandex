package ru.yandex.ci.core.arc;

import java.util.HashSet;
import java.util.Optional;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;

public class ArcCommitUtils {
    private static final Pattern REVIEW_PATTERN = Pattern.compile("(REVIEW: )([0-9]+)");
    private static final Pattern REPLACE_REVIEW_PATTERN = Pattern.compile("([.|\\s]?)(REVIEW: [0-9]+)[\\s]?");
    private static final Pattern TICKET_PATTERN = Pattern.compile("([A-Z]{2,})-[0-9]+");
    private static final Pattern DEVEXP_PATTERN = Pattern.compile(
            "<!-- DEVEXP BEGIN -->.*?<!-- DEVEXP END -->",
            Pattern.MULTILINE | Pattern.DOTALL
    );

    private ArcCommitUtils() {
    }

    public static Optional<Integer> parsePullRequestId(String message) {
        try {
            Matcher m = REVIEW_PATTERN.matcher(message);
            return m.find() ? Optional.of(Integer.valueOf(m.group(2))) : Optional.empty();
        } catch (NumberFormatException ex) {
            return Optional.empty();
        }
    }

    public static String cleanupMessage(String message) {
        return messageWithoutPullRequestId(removeDevexpMessage(message)).strip();
    }

    @VisibleForTesting
    static String messageWithoutPullRequestId(String message) {
        return REPLACE_REVIEW_PATTERN.matcher(message).replaceAll("$1");
    }

    private static String removeDevexpMessage(String message) {
        return DEVEXP_PATTERN.matcher(message).replaceAll("");
    }

    public static Set<Ticket> parseTicketsFully(String message) {
        var matches = new HashSet<Ticket>();
        Matcher m = TICKET_PATTERN.matcher(message);
        while (m.find()) {
            matches.add(new Ticket(m.group(1), m.group()));
        }
        return matches;
    }

    public static Set<String> parseTickets(String message) {
        return parseTicketsFully(message).stream()
                .map(Ticket::getKey)
                .collect(Collectors.toSet());
    }

    public static Set<String> parseStQueues(String message) {
        return parseTicketsFully(message).stream()
                .map(Ticket::getQueue)
                .collect(Collectors.toSet());
    }

    public static Optional<String> firstParentCommitHash(ArcCommit commit) {
        String previousHash = commit.getParents().isEmpty() ? null : commit.getParents().get(0);
        return Optional.ofNullable(previousHash);
    }

    public static Optional<ArcRevision> firstParentArcRevision(ArcCommit commit) {
        return firstParentCommitHash(commit).map(ArcRevision::of);
    }
}
