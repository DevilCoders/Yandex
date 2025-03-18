def adjust_weights_with_snippet_load(orig_weights, snippet_power_ratio):
    """
        Adjust replicas weights, knowing that snippet requests go only to snippets machines, thus creating disbalance.

        :type orig_weights: lis[(float, bool)]
        :type snippet_power_ratio: float

        :param orig_weights: list os pairs (weight, is snippet source)
        :param snippet_power_ratio: ratio of cpu usage required to process snippet requests (in range of [0., 1.))

        :return (list of float): list of new weights for instances
    """

    if snippet_power_ratio == 0.0:
        return map(lambda (w, used_as_snip): w, orig_weights)

    total_weight = 0.0
    for w, used_as_snip in orig_weights:
        total_weight += w

    snippet_instances = len(filter(lambda (w, used_as_snip): used_as_snip is True, orig_weights))
    snippet_weight = total_weight * snippet_power_ratio
    snippet_weight_per_instane = snippet_weight / snippet_instances

    result = []
    for w, used_as_snip in orig_weights:
        if used_as_snip:
            result.append(max(0.0, w - snippet_weight_per_instane))
        else:
            result.append(w)

    return result
