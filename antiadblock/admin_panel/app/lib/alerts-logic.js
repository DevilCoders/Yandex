import PriorityQueue from 'js-priority-queue';

const COOEFF_STATE_PRIO = 10000000000;
const WEIGHTS = {
        red: 1,
        yellow: 2,
        green: 3
    };

function statePriority(state) {
    return WEIGHTS[state] || Number.MAX_SAFE_INTEGER;
}

function alertPriorityComparator(a, b) {
    return (((statePriority(a.state) - statePriority(b.state)) * COOEFF_STATE_PRIO) + (b.transition_time - a.transition_time));
}

function alertFilter(item) {
    return (statePriority(item.state) < WEIGHTS.green);
}

export function prepareServiceAlerts(alerts, limit = 5) {
    const queue = new PriorityQueue({comparator: alertPriorityComparator});

    let result = {
        topAlerts: [],
        tailAlerts: []
    };

    for (const alertItem of alerts) {
        if (alertFilter(alertItem)) {
            queue.queue(alertItem);
        }
    }

    const len = queue.length;
    for (let i = 0; i < len; i++) {
        if (i < limit) {
            result.topAlerts.push(queue.dequeue());
        } else {
            result.tailAlerts.push(queue.dequeue());
        }
    }

    return result;
}

export function prepareAlerts(alertItems, limit = 5) {
    // do prepare for income alerts
    let topAlerts = {},
        tailAlerts = {};

    for (const [serviceId, serviceAlertItems] of Object.entries(alertItems)) {
        const serviceAlerts = prepareServiceAlerts(serviceAlertItems, limit);

        topAlerts[serviceId] = serviceAlerts.topAlerts;
        tailAlerts[serviceId] = serviceAlerts.tailAlerts;
    }
    return {topAlerts, tailAlerts};
}
// todo: add tests for this
