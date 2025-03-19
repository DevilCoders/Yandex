import numpy as np

## TODO: Refactor code according CLOUD-86335

def apply_attribution(events, target_event_ = 'billing_account_created'.encode('UTF-8'), attribution_type_ = 'last'.encode('UTF-8')):
    #     events = [[event_id, event_type, event_time, event_channel]]
    if(len(events) == 0 or events is None):
        return []
    target_event = target_event_.decode('UTF-8')
    attribution_type = attribution_type_.decode('UTF-8')

    try:
        target_event_time = np.min([e[2] for e in events if e[1].decode('UTF-8') == target_event])
    except:
        return []
    events_before_target = [e for e in events if (e[1].decode('UTF-8') in { 'site_visit', 'event_application', 'event_visit'}
                            ) and (e[2] <= target_event_time)]


    time_id_by_events = dict()
    new_events_before_target = []
    for e in events_before_target:
        e_help = (e[1], e[3:9])
        if e_help not in time_id_by_events:
            time_id_by_events[e_help] = set()
        for (event_id, event_time) in time_id_by_events[e_help]:
            if abs(event_time - e[2]) <= 5:
                break
        else:
            time_id_by_events[e_help].add((e[0], e[2]))
            new_events_before_target.append(e)
    events_before_target = new_events_before_target

    events_before_target_channels = set([e[3] for e in events_before_target])
    try:
        events_before_target_channels.remove('Direct')
        if len(events_before_target) > 0:
            i = len(events_before_target) - 1
            while(i>=0):
                if(events_before_target[i][3] == 'Direct'):
                    del events_before_target[i]
                i -= 1
    except:
        pass


    if(len(events_before_target) == 0):
        return []
    if (len(events_before_target) == 1):
        return [tuple(e) + (1,) for e in events_before_target]



    if attribution_type == 'first':
        res = [tuple(e)+(0,) for e in events_before_target]
        res[0] = tuple(res[0][:-1]) + (1,)
        return res
    elif attribution_type == 'last':
        res = [tuple(e)+(0,) for e in events_before_target]
        res[-1] = tuple(res[-1][:-1]) + (1,)
        return res
    elif attribution_type == 'uniform':
        try:
            return [tuple(e) + (1/len(events_before_target),) for e in events_before_target]
        except:
            return []
    elif attribution_type == 'u_shape':
        if (len(events_before_target) == 2):
            return [tuple(e) + (0.5,) for e in events_before_target]
        try:
            res = [tuple(e) + (0.2/(len(events_before_target)-2),) for e in events_before_target]
        except:
            return []
        res[0] = tuple(res[0][:-1]) + (0.4,)
        res[-1] = tuple(res[-1][:-1]) + (0.4,)
        return res
    elif attribution_type == 'exp_7d_half_life_time_decay':
        time_first = events_before_target[0][2]
        time_last = events_before_target[-1][2]
        try:
            res = [tuple(e) +  (np.power(2,(e[2] - time_last)/(7*24*3600)),) for e in events_before_target]
        except:
            return []
        return [tuple(r[:-1]) + (r[-1] / np.sum([r[-1] for r in res]),) for r in res]
    else:
        return []

def apply_attribution_all(events, target_event_ = 'billing_account_created'):
    target_event = target_event_.decode('UTF-8')
    attr_weights = []
    for attr_type in ['first','last','uniform','u_shape','exp_7d_half_life_time_decay']:
        attr_weights.append(apply_attribution(events, target_event_, attr_type.encode('UTF-8')))

    res = []
    for j in range(len(attr_weights[0])):
        res.append(
            tuple(
                attr_weights[0][j][:-1]
            )
            +
            tuple([[attr_weights[i][j][-1] for i in range(len(attr_weights))]],)
        )
    return res
