export const OPEN_EXPERIMENT_APPLYING = 'OPEN_EXPERIMENT_APPLYING';
export const CLOSE_EXPERIMENT_APPLYING = 'CLOSE_EXPERIMENT_APPLYING';

export function openExperimentApplying(serviceId, configId) {
    return {
        serviceId,
        configId,
        type: OPEN_EXPERIMENT_APPLYING
    };
}

export function closeExperimentApplying() {
    return {
        type: CLOSE_EXPERIMENT_APPLYING
    };
}
