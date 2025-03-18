package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.activity.Activity;
import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

@ActivityInterface
public interface FailingActivity {

    @ActivityMethod
    void run(int passAttemptNumber);

    class Impl implements FailingActivity {
        @Override
        public void run(int passAttemptNumber) {
            int attempt = Activity.getExecutionContext().getInfo().getAttempt();
            if (attempt < passAttemptNumber) {
                throw new RuntimeException(
                        "Fail on attempt #" + attempt + ", will pass on attempt #" + passAttemptNumber
                );
            }
        }
    }
}
