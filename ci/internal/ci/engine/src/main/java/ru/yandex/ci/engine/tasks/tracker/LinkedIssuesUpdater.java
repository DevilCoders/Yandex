package ru.yandex.ci.engine.tasks.tracker;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import ci.tracker.create_issue.CreateIssueOuterClass;
import com.google.common.base.Suppliers;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.startrek.client.Session;
import ru.yandex.startrek.client.error.EntityNotFoundException;
import ru.yandex.startrek.client.model.CommentCreate;
import ru.yandex.startrek.client.model.Issue;
import ru.yandex.startrek.client.model.IssueRef;
import ru.yandex.startrek.client.model.IssueUpdate;
import ru.yandex.startrek.client.model.Ref;
import ru.yandex.startrek.client.model.Resolution;
import ru.yandex.startrek.client.model.ResolutionRef;
import ru.yandex.startrek.client.model.Transition;

@Slf4j
@RequiredArgsConstructor
public class LinkedIssuesUpdater {

    @Nonnull
    private final TrackerIssuesLinker.IssuesLinker linker;

    @Nonnull
    private final Session session;

    public void updateIssue(CreateIssueOuterClass.UpdateTemplate template, IssueRef issueRef) {
        if (template.getComment().isEmpty()) {
            log.info("Comment is empty, skip updating");
        } else {
            addComment(renderComment(template.getComment()), issueRef);
        }
    }

    public void updateAndTransitLinkedIssues(CreateIssueOuterClass.UpdateTemplate template) {
        new LinkedIssuesProcessor().updateAndTransitLinkedIssues(template.getLinked(), template.getLinkedQueuesMap());
    }

    public void updateAndTransitLinkedIssues(CreateIssueOuterClass.Template template) {
        new LinkedIssuesProcessor().updateAndTransitLinkedIssues(template.getLinked(), template.getLinkedQueuesMap());
    }

    public void transitIssue(CreateIssueOuterClass.Transition transition, Issue issueRef) {
        new LinkedIssuesProcessor().transitIssue(transition, issueRef);
    }

    //

    private String renderComment(String template) {
        return linker.getDescriptionLookup().getSubstitutor().replace(template);
    }

    private void addComment(String commentText, IssueRef issueRef) {
        var comment = CommentCreate.builder()
                .comment(commentText)
                .build();
        log.info("Comment to add: {}", comment);
        var newComment = session.comments().create(issueRef, comment);

        log.info("Created comment: {}", newComment);
    }

    @RequiredArgsConstructor
    private class LinkedIssuesProcessor {
        private final Supplier<List<Resolution>> getResolutions = Suppliers.memoize(this::getResolutions);

        void transitIssue(CreateIssueOuterClass.Transition transition, Issue trackerIssue) {
            new SingleIssueProcessor(transition, trackerIssue).makeTransition();
        }

        void updateAndTransitLinkedIssues(CreateIssueOuterClass.LinkedIssueUpdate defaultRules,
                                          Map<String, CreateIssueOuterClass.LinkedIssueUpdate> queueRules) {
            if (isEmpty(defaultRules) && queueRules.isEmpty()) {
                log.info("No linked issues updates rules found");
                return; // ---
            }

            var issues = linker.getDescriptionLookup().getAllIssues();
            if (issues.isEmpty()) {
                log.info("No linked issues found");
                return; // ---
            }

            var renderCache = new HashMap<String, String>();

            for (var linkedIssue : issues) {
                log.info("Trying to update issue: {}", linkedIssue.getKey());

                var rule = queueRules.getOrDefault(linkedIssue.getQueue(), defaultRules);
                log.info("Matched rule: {}", rule);
                if (isEmpty(rule)) {
                    continue;
                }

                var hasTransition = rule.hasTransition();
                var hasComment = !rule.getComment().isEmpty();

                if (!hasTransition && !hasComment) {
                    continue;
                }

                Issue trackerLinkedIssue;
                try {
                    trackerLinkedIssue = session.issues().get(linkedIssue.getKey());
                } catch (EntityNotFoundException e) {
                    log.error("Unable to load linked issue {}, skipping", linkedIssue.getKey(), e);
                    continue;
                }

                if (hasTransition) {
                    new SingleIssueProcessor(rule.getTransition(), trackerLinkedIssue).makeTransition();
                }

                if (hasComment) {
                    var renderedComment = renderCache.computeIfAbsent(rule.getComment(),
                            LinkedIssuesUpdater.this::renderComment);
                    addComment(renderedComment, trackerLinkedIssue);
                }
            }
        }

        private boolean isEmpty(CreateIssueOuterClass.LinkedIssueUpdate rules) {
            return rules.getComment().isEmpty() && !rules.hasTransition();
        }

        private List<Resolution> getResolutions() {
            var resolutions = session.resolutions().getAll().toList();
            resolutions.forEach(value -> log.info("{}", value));
            return resolutions;
        }

        @RequiredArgsConstructor
        private class SingleIssueProcessor {
            private final CreateIssueOuterClass.Transition transition;
            private final Issue trackerIssue;

            void makeTransition() {
                var foundTransitionOptional = lookupTransition();
                if (foundTransitionOptional.isPresent()) {
                    var foundResolutionOptional = lookupResolution(transition.getResolution());
                    try {
                        var foundTransition = foundTransitionOptional.get();
                        if (foundResolutionOptional.isEmpty()) {
                            log.info("Moving issue {} to {}",
                                    trackerIssue.getKey(), foundTransition);
                            session.transitions().execute(trackerIssue, foundTransition);
                        } else {
                            var foundResolution = foundResolutionOptional.get();
                            log.info("Moving issue {} to {} with resolution {}",
                                    trackerIssue.getKey(), foundTransition, foundResolution);
                            var update = IssueUpdate.resolution(foundResolution.getId()).build();
                            session.transitions().execute(trackerIssue, foundTransition, update);
                        }
                    } catch (Exception e) {
                        log.error("Unable to execute transition", e);
                        if (!transition.getIgnoreError()) {
                            throw e;
                        }
                    }
                }
            }

            private Optional<Transition> lookupTransition() {
                var currentStatus = trackerIssue.getStatus();
                var targetStatus = transition.getStatus();
                log.info("Try to move issue {} from [{}] to [{}]", trackerIssue.getKey(), currentStatus, targetStatus);

                if (Objects.equals(currentStatus.getKey(), targetStatus) ||
                        Objects.equals(currentStatus.getDisplay(), targetStatus)) {
                    log.info("Already on requested status: {}", currentStatus);
                    return Optional.empty();
                }

                var transitions = trackerIssue.getTransitions();
                transitions.forEach(value -> log.info("{}", value));

                var byId = TrackerIssuesLinker.toMap(transitions, Transition::getId);
                var matched = byId.get(targetStatus);
                if (matched == null) {
                    var byToKey = TrackerIssuesLinker.toMap(transitions, trans -> trans.getTo().getKey());
                    matched = byToKey.get(targetStatus);
                    if (matched == null) {
                        var byToDisplay = TrackerIssuesLinker.toMap(transitions, trans -> trans.getTo().getDisplay());
                        matched = byToDisplay.get(targetStatus);
                        if (matched == null) {
                            var msg = String.format("Unable to find available transition from [%s] to [%s]." +
                                            "Available transitions are %s, %s or %s",
                                    currentStatus, targetStatus, byId, byToKey, byToDisplay);
                            if (transition.getIgnoreNoTransition()) {
                                log.error(msg);
                                return Optional.empty();
                            } else {
                                throw new RuntimeException(msg);
                            }
                        }
                    }
                }
                return Optional.of(matched);
            }

            private Optional<Resolution> lookupResolution(String resolution) {
                if (resolution.isEmpty()) {
                    return Optional.empty();
                }
                var resolutions = getResolutions.get();

                var byId = TrackerIssuesLinker.toMap(resolutions, ResolutionRef::getKey);
                var matched = byId.get(resolution);
                if (matched == null) {
                    var byDisplay = TrackerIssuesLinker.toMap(resolutions, Ref::getDisplay);
                    matched = byDisplay.get(resolution);
                    if (matched == null) {
                        var msg = String.format("Unable to find resolution [%s]. " +
                                        "Available resolutions: %s or %s",
                                resolution, byId.keySet(), byDisplay.keySet());
                        throw new RuntimeException(msg);
                    }
                }
                return Optional.of(matched);
            }


        }
    }
}
