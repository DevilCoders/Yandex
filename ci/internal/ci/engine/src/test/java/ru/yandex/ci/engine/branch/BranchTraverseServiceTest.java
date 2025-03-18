package ru.yandex.ci.engine.branch;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import org.apache.commons.lang3.tuple.Pair;
import org.assertj.core.api.Assertions;
import org.assertj.core.api.ObjectAssert;
import org.assertj.core.presentation.StandardRepresentation;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchTraverseService.RangeConsumer;

class BranchTraverseServiceTest extends EngineTestBase {
    @Autowired
    private BranchTraverseService branchTraverseService;

    private RangeCollector consumer;

    @BeforeEach
    public void setUp() {
        consumer = new RangeCollector();
    }

    @Test
    void trunk() {
        traverse(TestData.TRUNK_R10, null, consumer);
        assertThat(consumer).wasCalledWithRanges(TestData.TRUNK_R10, null);
    }

    @Test
    void trunkLimited() {
        traverse(TestData.TRUNK_R10, TestData.TRUNK_R4, consumer);
        assertThat(consumer).wasCalledWithRanges(TestData.TRUNK_R10, TestData.TRUNK_R4);
    }

    @Test
    void branchLimited() {
        ArcBranch branch = addBranchAt(TestData.TRUNK_R6);
        OrderedArcRevision branchRev9 = rev(branch, 9);
        OrderedArcRevision branchRev6 = rev(branch, 6);

        traverse(branchRev9, branchRev6, consumer);

        assertThat(consumer).wasCalledWithRanges(branchRev9, branchRev6);
    }

    @Test
    void branch() {
        ArcBranch branch = addBranchAt(TestData.TRUNK_R6);
        OrderedArcRevision branchRev9 = rev(branch, 9);

        traverse(branchRev9, null, consumer);

        assertThat(consumer).wasCalledWithRanges(branchRev9, null, TestData.TRUNK_R6, null);
    }

    @Test
    void branchToTrunk() {
        ArcBranch branch = addBranchAt(TestData.TRUNK_R6);
        OrderedArcRevision branchRev9 = rev(branch, 9);

        traverse(branchRev9, TestData.TRUNK_R3, consumer);

        assertThat(consumer).wasCalledWithRanges(branchRev9, null, TestData.TRUNK_R6, TestData.TRUNK_R3);
    }

    @Test
    void branchToTrunkEmptyRange() {
        ArcBranch branch = addBranchAt(TestData.TRUNK_R6);
        OrderedArcRevision branchRev9 = rev(branch, 9);

        traverse(branchRev9, TestData.TRUNK_R6, consumer);

        assertThat(consumer).wasCalledWithRanges(branchRev9, null); // в транке нечего ловить
    }

    @Test
    void branchToTrunkBreak() {
        ArcBranch branch = addBranchAt(TestData.TRUNK_R6);
        OrderedArcRevision branchRev9 = rev(branch, 9);

        consumer = new RangeCollector() {
            @Override
            public boolean consume(OrderedArcRevision fromRevision, @Nullable OrderedArcRevision toRevision) {
                super.consume(fromRevision, toRevision);
                return true;
            }
        };
        traverse(branchRev9, TestData.TRUNK_R3, consumer);

        assertThat(consumer).wasCalledWithRanges(branchRev9, null);
    }

    private static OrderedArcRevision rev(ArcBranch branch, int number) {
        return OrderedArcRevision.fromHash(Integer.toHexString(number), branch, number, 0);
    }

    private void traverse(OrderedArcRevision from, @Nullable OrderedArcRevision to, RangeConsumer consumer) {
        db.currentOrReadOnly(() ->
                branchTraverseService.traverse(from, to, consumer)
        );
    }

    private ArcBranch addBranchAt(OrderedArcRevision revision) {
        String name = "users/a/" + revision.getCommitId();
        db.currentOrTx(() -> {
            db.branches().save(BranchInfo.builder()
                    .id(BranchInfo.Id.of(name))
                    .baseRevision(revision)
                    .build()
            );
        });
        return ArcBranch.ofBranchName(name);
    }

    private static RangesAssert assertThat(RangeCollector rangeCollector) {
        return new RangesAssert(rangeCollector);
    }

    private static class RangesAssert extends ObjectAssert<RangeCollector> {
        private RangesAssert(RangeCollector rangeCollector) {
            super(rangeCollector);
        }

        public RangesAssert wasCalledWithRanges(OrderedArcRevision... revisions) {
            Preconditions.checkArgument(revisions.length % 2 == 0);

            int size = revisions.length / 2;
            List<Pair<OrderedArcRevision, OrderedArcRevision>> pairs = new ArrayList<>(size);
            for (int i = 0; i < size; i++) {
                pairs.add(Pair.of(revisions[i * 2], revisions[i * 2 + 1]));
            }

            //noinspection unchecked
            Assertions.assertThat(actual.ranges)
                    .withRepresentation(new StandardRepresentation() {
                        @Override
                        public String toStringOf(Object object) {
                            if (object instanceof Pair) {
                                @SuppressWarnings("unchecked")
                                var pair = (Pair<OrderedArcRevision, OrderedArcRevision>) object;
                                return reprRevision(pair.getLeft()) + " --> " + reprRevision(pair.getRight());
                            }
                            return super.toStringOf(object);
                        }

                        @Nullable
                        private String reprRevision(@Nullable OrderedArcRevision rev) {
                            if (rev == null) {
                                return null;
                            }
                            return rev.getCommitId() + " [" + rev.getBranch() + "]";
                        }
                    })
                    .containsExactly(pairs.toArray(Pair[]::new));

            return this;
        }
    }

    private static class RangeCollector implements RangeConsumer {
        private final List<Pair<OrderedArcRevision, OrderedArcRevision>> ranges = new ArrayList<>();

        @Override
        public boolean consume(OrderedArcRevision fromRevision, @Nullable OrderedArcRevision toRevision) {
            ranges.add(Pair.of(fromRevision, toRevision));
            return false;
        }
    }
}
