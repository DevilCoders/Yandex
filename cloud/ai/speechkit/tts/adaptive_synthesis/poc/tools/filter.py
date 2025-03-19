class DialogueResult:
    NOT_HEARED = 'NotHeared'
    PASSED = 'Passed'
    FILTERED = 'Filtered'
    STOP = 'Stop'


def dialogue():
    print('1 - filtered, 2 - passed, 3 - stop, 0 - not heared')
    answer = int(input())
    return {
        1: DialogueResult.FILTERED,
        2: DialogueResult.PASSED,
        0: DialogueResult.NOT_HEARED,
        3: DialogueResult.STOP
    }[answer]


def filter_samples(samples, num_total_samples=1000, min_sample_duration=5, filter_rule=None):
    filtered_samples = []
    num_filtered = 0

    for counter, sample in enumerate(samples):
        if sample.duration < min_sample_duration:
            continue

        if filter_rule is not None and not filter_rule(sample):
            continue

        print(sample.speaker_id)

        print(f'Num collected samples: {len(filtered_samples)}')
        sample.show_text()
        sample.play()

        dialogue_result = dialogue()
        print(dialogue_result)

        if dialogue_result == DialogueResult.PASSED:
            filtered_samples.append(sample)
        elif dialogue_result == DialogueResult.FILTERED:
            num_filtered += 1
        elif len(filtered_samples) > num_total_samples or dialogue_result == DialogueResult.STOP:
            break

    print(f'total,% filtered : {num_filtered}, {num_filtered / counter}')
    return filtered_samples
