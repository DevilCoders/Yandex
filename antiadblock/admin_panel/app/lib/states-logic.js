export function prepareServiceStates(serviceStateItems) {
    let isAlert = false,
        isWarning = false,
        countOutdated = 0,
        countAlerts = 0;

    serviceStateItems.forEach(item => {
        if (!item.in_progress) {
            if (item.state === 'red') {
                isAlert = true;
                countAlerts++;
            } else if (item.state === 'yellow') {
                isWarning = true;
            }

            if (item.outdated) {
                countOutdated++;
            }
        }
    });

    return {
        state: isAlert ? 'red' : isWarning ? 'yellow' : 'green',
        countOutdated,
        countAlerts,
        countAllChecks: serviceStateItems.length
    };
}

export function prepareStates(stateItems) {
    let stateObj = {};

    for (const [serviceId, serviceStateItems] of Object.entries(stateItems)) {
        stateObj[serviceId] = prepareServiceStates(serviceStateItems);
    }

    return stateObj;
}
