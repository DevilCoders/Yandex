import serviceApi from 'app/api/service';
import configApi from 'app/api/config';

import {setGlobalErrors} from 'app/actions/errors';

export const START_SERVICE_LOADING = 'START_SERVICE_LOADING';
export const END_SERVICE_LOADING = 'END_SERVICE_LOADING';
export const START_SERVICE_TREND_LOADING = 'START_SERVICE_TREND_LOADING';
export const END_SERVICE_TREND_LOADING = 'END_SERVICE_TREND_LOADING';
export const START_SERVICE_TREND_CREATION = 'START_SERVICE_TREND_CREATION';
export const END_SERVICE_TREND_CREATION = 'END_SERVICE_TREND_CREATION';
export const START_LABELS_LOADING = 'START_LABELS_LOADING';
export const END_LABELS_LOADING = 'END_LABELS_LOADING';
export const CHANGE_LABEL_PARENT_START = 'CHANGE_LABEL_PARENT_START';
export const CHANGE_LABEL_PARENT_END = 'CHANGE_LABEL_PARENT_END';
export const START_SERVICE_CONFIG_LOADING = 'START_SERVICE_CONFIG_LOADING';
export const END_SERVICE_CONFIG_LOADING = 'END_SERVICE_CONFIG_LOADING';
export const START_SERVICE_CONFIGS_LOADING = 'START_SERVICE_CONFIGS_LOADING';
export const END_SERVICE_CONFIGS_LOADING = 'END_SERVICE_CONFIGS_LOADING';
export const START_SERVICE_CHANGE_PRIORITY = 'START_SERVICE_CHANGE_PRIORITY';
export const END_SERVICE_CHANGE_PRIORITY = 'END_SERVICE_CHANGE_PRIORITY';
export const START_SERVICE_CONFIG_APPLYING = 'START_SERVICE_CONFIG_APPLYING';
export const END_SERVICE_CONFIG_APPLYING = 'END_SERVICE_CONFIG_APPLYING';
export const START_SERVICE_EXPERIMENT_APPLYING = 'START_SERVICE_EXPERIMENT_APPLYING';
export const END_SERVICE_EXPERIMENT_APPLYING = 'END_SERVICE_EXPERIMENT_APPLYING';
export const START_CONFIG_REMOVE_EXPERIMENT = 'START_CONFIG_REMOVE_EXPERIMENT';
export const END_CONFIG_REMOVE_EXPERIMENT = 'END_CONFIG_REMOVE_EXPERIMENT';
export const START_SERVICE_CONFIG_SAVING = 'START_SERVICE_CONFIG_SAVING';
export const END_SERVICE_CONFIG_SAVING = 'END_SERVICE_CONFIG_SAVING';
export const START_SERVICE_SETUP_LOADING = 'START_SERVICE_SETUP_LOADING';
export const END_SERVICE_SETUP_LOADING = 'END_SERVICE_SETUP_LOADING';
export const RESET_SERVICE_AUDIT = 'RESET_SERVICE_AUDIT';
export const START_SERVICE_AUDIT_LOADING = 'START_SERVICE_AUDIT_LOADING';
export const END_SERVICE_AUDIT_LOADING = 'END_SERVICE_AUDIT_LOADING';
export const START_SERVICE_CONFIG_ARCHIVED_SETTING = 'START_SERVICE_CONFIG_ARCHIVED_SETTING';
export const END_SERVICE_CONFIG_ARCHIVED_SETTING = 'END_SERVICE_CONFIG_ARCHIVED_SETTING';
export const START_SERVICE_DISABLING = 'START_SERVICE_DISABLING';
export const END_SERVICE_DISABLING = 'END_SERVICE_DISABLING';
export const START_SERVICE_ENABLING = 'START_SERVICE_ENABLING';
export const END_SERVICE_ENABLING = 'END_SERVICE_ENABLING';
export const START_SERVICE_CONFIG_MODERATING = 'START_SERVICE_CONFIG_MODERATING';
export const END_SERVICE_CONFIG_MODERATING = 'END_SERVICE_CONFIG_MODERATING';
export const START_SERVICE_COMMENT = 'START_SERVICE_COMMENT';
export const END_SERVICE_COMMENT = 'END_SERVICE_COMMENT';
export const START_SET_SERVICE_ENABLE_MONITORING = 'START_SET_SERVICE_ENABLE_MONITORING';
export const END_SET_SERVICE_ENABLE_MONITORING = 'END_SET_SERVICE_ENABLE_MONITORING';
export const START_PARENT_EXP_CONFIGS_LOADING = 'START_PARENT_EXP_CONFIGS_LOADING';
export const END_PARENT_EXP_CONFIGS_LOADING = 'END_PARENT_EXP_CONFIGS_LOADING';

export function fetchService(serviceId) {
    return dispatch => {
        dispatch(startServiceLoading(serviceId));

        return Promise.all(
            [
                serviceApi.fetchService(serviceId),
                serviceApi.fetchLabels(serviceId)
            ])
            .then(([service, labels]) => {
                dispatch(endServiceLoading(serviceId, service));
                dispatch(endLabelsLoading(serviceId, labels));
            }, error => {
                return dispatch(setGlobalErrors([error.message]));
            });
    };
}

export function startServiceLoading(serviceId) {
    return {
        type: START_SERVICE_LOADING,
        serviceId
    };
}

export function endServiceLoading(serviceId, service) {
    return {
        type: END_SERVICE_LOADING,
        serviceId,
        service
    };
}

export function fetchTrend(serviceId) {
    return dispatch => {
        dispatch(startTrendLoading(serviceId));

        return serviceApi.fetchTrend(serviceId)
            .then(trend => {
                dispatch(endTrendLoading(serviceId, trend, true));
            }, error => {
                // Ошибка Not Found, значит тикета нет
                if (error.status === 404) {
                    return dispatch(endTrendLoading(serviceId, {
                        ticket_id: null
                    }, true));
                }

                // Ошибка Wrong Component, значит сервис не заведен в startrek и создать тикет невозможно
                if (error.status === 400) {
                    return dispatch(endTrendLoading(serviceId, {
                        ticket_id: null
                    }, false));
                }

                // Другие ошибки
                return dispatch(setGlobalErrors([error.message]));
            });
    };
}

export function startTrendLoading(serviceId) {
    return {
        type: START_SERVICE_TREND_LOADING,
        serviceId
    };
}

export function endTrendLoading(serviceId, trend, loaded) {
    return {
        type: END_SERVICE_TREND_LOADING,
        serviceId,
        trend,
        loaded
    };
}

export function createTrend(serviceId, datetime) {
    return dispatch => {
        dispatch(startTrendCreation(serviceId, datetime));

        return serviceApi.createTrend(serviceId, datetime)
            .then(trend => {
                dispatch(endTrendCreation(serviceId, trend));
            }, error => {
                return dispatch(setGlobalErrors([error.message]));
            });
    };
}

export function startTrendCreation(serviceId, datetime) {
    return {
        type: START_SERVICE_TREND_CREATION,
        serviceId,
        datetime
    };
}

export function endTrendCreation(serviceId, trend) {
    return {
        type: END_SERVICE_TREND_CREATION,
        serviceId,
        trend
    };
}

export function fetchLabels(serviceId) {
    return dispatch => {
        dispatch(startLabelsLoading(serviceId));

        return serviceApi.fetchLabels(serviceId)
            .then(labels => {
                dispatch(endLabelsLoading(serviceId, labels));
            }, error => {
                return dispatch(setGlobalErrors([error.message]));
            });
    };
}

export function startLabelsLoading(serviceId) {
    return {
        type: START_LABELS_LOADING,
        serviceId
    };
}

export function endLabelsLoading(serviceId, labels) {
    return {
        type: END_LABELS_LOADING,
        serviceId,
        labels
    };
}

export function changeLabelParent(labelId, parentId, serviceId) {
    return dispatch => {
        dispatch(changeLabelParentStart(labelId, parentId, serviceId));

        return serviceApi.changeLabelParent(labelId, parentId)
            .then(() => dispatch(fetchLabels(serviceId)))
            .catch(error => dispatch(setGlobalErrors([error.message])));
    };
}

export function changeLabelParentStart(labelId, parentId, serviceId) {
    return {
        type: CHANGE_LABEL_PARENT_START,
        labelId,
        parentId,
        serviceId
    };
}

export function changeLabelParentEnd(labelId, parentId, serviceId) {
    return {
        type: CHANGE_LABEL_PARENT_END,
        labelId,
        parentId,
        serviceId
    };
}

export function serviceChangePriority(serviceId, priority) {
    return dispatch => {
        dispatch(startServiceChangePriority(serviceId, priority));

        return serviceApi.changePriority(serviceId, priority).then(() => {
            return dispatch(endServiceChangePriority(serviceId, priority));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceChangePriority(serviceId, priority) {
    return {
        type: START_SERVICE_CHANGE_PRIORITY,
        serviceId,
        priority
    };
}

export function endServiceChangePriority(serviceId, priority) {
    return {
        type: END_SERVICE_CHANGE_PRIORITY,
        serviceId,
        priority
    };
}

export function fetchConfig(serviceId, configId) {
    return dispatch => {
        dispatch(startServiceConfigLoading(serviceId, configId));

        return configApi.fetchConfig(configId).then(config => {
            return dispatch(endServiceConfigLoading(serviceId, configId, config));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceConfigLoading(serviceId, configId) {
    return {
        type: START_SERVICE_CONFIG_LOADING,
        serviceId,
        configId
    };
}

export function endServiceConfigLoading(serviceId, configId, config) {
    return {
        type: END_SERVICE_CONFIG_LOADING,
        serviceId,
        configId,
        config
    };
}

export function fetchServiceConfigs(labelId, offset, limit, filters) {
    return dispatch => {
        dispatch(startServiceConfigsLoading(labelId, offset, limit));

        return serviceApi.fetchServiceConfigs(labelId, offset, limit, filters).then(configs => {
            return dispatch(endServiceConfigsLoading(labelId, configs));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceConfigsLoading(labelId, offset, limit) {
    return {
        type: START_SERVICE_CONFIGS_LOADING,
        labelId,
        offset,
        limit
    };
}

export function endServiceConfigsLoading(labelId, configs) {
    return {
        type: END_SERVICE_CONFIGS_LOADING,
        labelId,
        configs
    };
}

export function applyConfig(labelId, configId, options) {
    return dispatch => {
        dispatch(startServiceConfigApplying(labelId, configId, options));

        return configApi.applyConfig(labelId, configId, options).then(() => {
            return dispatch(endServiceConfigApplying(labelId, configId, options));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceConfigApplying(labelId, configId, options) {
    return {
        type: START_SERVICE_CONFIG_APPLYING,
        labelId,
        configId,
        options
    };
}

export function endServiceConfigApplying(labelId, configId, options) {
    return {
        type: END_SERVICE_CONFIG_APPLYING,
        labelId,
        configId,
        options
    };
}

export function applyExperiment(serviceId, configId, options) {
    return dispatch => {
        dispatch(startServiceExperimentApplying(serviceId, configId, options));

        return serviceApi.applyExperiment(configId, options)
            .then(() => {
                return dispatch(endServiceExperimentApplying(serviceId, configId, options, true));
            }, error => {
                dispatch(endServiceExperimentApplying(serviceId, configId, options, false));
                throw error;
            });
    };
}

export function startServiceExperimentApplying(serviceId, configId, options) {
    return {
        type: START_SERVICE_EXPERIMENT_APPLYING,
        serviceId,
        configId,
        options
    };
}

export function endServiceExperimentApplying(serviceId, configId, options, success) {
    return {
        type: END_SERVICE_EXPERIMENT_APPLYING,
        serviceId,
        configId,
        options,
        success
    };
}

export function removeExperiment(serviceId, configId) {
    return dispatch => {
        dispatch(startConfigRemoveExperiment(serviceId, configId));

        return serviceApi.removeExperiment(configId)
            .then(() => {
                return dispatch(endConfigRemoveExperiment(serviceId, configId));
            }, error => {
                return dispatch(setGlobalErrors([error.message]));
            });
    };
}

export function startConfigRemoveExperiment(serviceId, configId) {
    return {
        type: START_CONFIG_REMOVE_EXPERIMENT,
        serviceId,
        configId
    };
}

export function endConfigRemoveExperiment(serviceId, configId) {
    return {
        type: END_CONFIG_REMOVE_EXPERIMENT,
        serviceId,
        configId
    };
}

export function fetchServiceSetup(serviceId) {
    return dispatch => {
        dispatch(startServiceSetupLoading(serviceId));

        /* TODO: вернуть этот код после отрывания мока
        return serviceApi.fetchServiceSetup(serviceId).then(service => {
            return dispatch(endServiceSetupLoading(serviceId, service));
        }, error => {
            redirect(antiadbUrl(`/error/${error.status}`));
        }); */

        return Promise.resolve(dispatch(endServiceSetupLoading(serviceId, {
            token: 'AABBCCDDEEFF....',
            js_inline: '(func () { /* here goes awesome JS code */})("<PARTNER-ID>")',
            nginx_config: 'here \n goes \n config'
        })));
    };
}

export function startServiceSetupLoading(serviceId) {
    return {
        type: START_SERVICE_SETUP_LOADING,
        serviceId
    };
}

export function endServiceSetupLoading(serviceId, setup) {
    return {
        type: END_SERVICE_SETUP_LOADING,
        serviceId,
        setup
    };
}

export function resetServiceAudit(serviceId) {
    return {
        type: RESET_SERVICE_AUDIT,
        serviceId
    };
}

export function fetchServiceAudit(serviceId, offset, limit, labelId) {
    return dispatch => {
        dispatch(startServiceAuditLoading(serviceId, offset, limit));

        // если есть labelId, то присылаются события только для него.
        return serviceApi.fetchServiceAudit(serviceId, offset, limit, labelId).then(audit => {
            return dispatch(endServiceAuditLoading(serviceId, audit));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceAuditLoading(serviceId, offset, limit) {
    return {
        type: START_SERVICE_AUDIT_LOADING,
        serviceId,
        offset,
        limit
    };
}

export function endServiceAuditLoading(serviceId, audit) {
    return {
        type: END_SERVICE_AUDIT_LOADING,
        serviceId,
        audit
    };
}

export function setArchivedConfig(serviceId, configId, archived) {
    return dispatch => {
        dispatch(startServiceConfigArchivedSetting(serviceId, configId, archived));

        return configApi.setArchivedConfig(configId, archived).then(() => {
            return dispatch(endServiceConfigArchivedSetting(serviceId, configId, archived));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceConfigArchivedSetting(serviceId, configId, archived) {
    return {
        type: START_SERVICE_CONFIG_ARCHIVED_SETTING,
        serviceId,
        configId,
        archived
    };
}

export function endServiceConfigArchivedSetting(serviceId, configId, archived) {
    return {
        type: END_SERVICE_CONFIG_ARCHIVED_SETTING,
        serviceId,
        configId,
        archived
    };
}

export function disableService(serviceId) {
    return dispatch => {
        dispatch(startServiceDisabling(serviceId));

        return serviceApi.disableService(serviceId).then(() => {
            return dispatch(endServiceDisabling(serviceId));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function enableService(serviceId) {
    return dispatch => {
        dispatch(startServiceEnabling(serviceId));

        return serviceApi.enableService(serviceId).then(() => {
            return dispatch(endServiceEnabling(serviceId));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceDisabling(serviceId) {
    return {
        type: START_SERVICE_DISABLING,
        serviceId
    };
}

export function endServiceDisabling(serviceId) {
    return {
        type: END_SERVICE_DISABLING,
        serviceId
    };
}

export function startServiceEnabling(serviceId) {
    return {
        type: START_SERVICE_ENABLING,
        serviceId
    };
}

export function endServiceEnabling(serviceId) {
    return {
        type: END_SERVICE_ENABLING,
        serviceId
    };
}

export function setModerateConfig(serviceId, labelId, configId, approved, comment) {
    return dispatch => {
        dispatch(startModerateServiceConfig(serviceId, configId, approved, comment));

        return configApi.setModerateConfig(labelId, configId, approved, comment).then(config => {
            return dispatch(endModerateServiceConfig(serviceId, configId, approved, comment, config));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startModerateServiceConfig(serviceId, configId, approved, comment) {
    return {
        type: START_SERVICE_CONFIG_MODERATING,
        serviceId,
        configId,
        approved,
        comment
    };
}

export function endModerateServiceConfig(serviceId, configId, approved, comment, config) {
    return {
        type: END_SERVICE_CONFIG_MODERATING,
        serviceId,
        configId,
        approved,
        comment,
        config
    };
}

export function addServiceComment(serviceId, comment) {
    return dispatch => {
        dispatch(startServiceComment(serviceId));

        return serviceApi.addServiceComment(serviceId, comment).then(() => {
            return dispatch(endServiceComment(serviceId, comment));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceComment(serviceId) {
    return {
        type: START_SERVICE_COMMENT,
        serviceId
    };
}

export function endServiceComment(serviceId, comment) {
    return {
        type: END_SERVICE_COMMENT,
        serviceId,
        comment
    };
}

export function setServiceEnableMonitoring(serviceId, monitoringName, monitoringIsEnable) {
    return dispatch => {
        dispatch(startSetServiceEnableMonitoring(serviceId));

        return serviceApi.setServiceEnableMonitoring(serviceId, monitoringName, monitoringIsEnable).then(() => {
            return dispatch(endSetServiceEnableMonitoring(serviceId, monitoringName, monitoringIsEnable));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startSetServiceEnableMonitoring(serviceId) {
    return {
        type: START_SET_SERVICE_ENABLE_MONITORING,
        serviceId
    };
}

export function endSetServiceEnableMonitoring(serviceId, monitoringName, monitoringIsEnable) {
    return {
        type: END_SET_SERVICE_ENABLE_MONITORING,
        serviceId,
        monitoringName,
        monitoringIsEnable
    };
}

export function fetchParentExpConfigs(labelId) {
    return dispatch => {
        dispatch(startParentExpConfigsLoading(labelId));

        return configApi.fetchExpConfigsId(labelId).then(data => {
            return dispatch(endParentExpConfigsLoading(labelId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startParentExpConfigsLoading(labelId) {
    return {
        type: START_PARENT_EXP_CONFIGS_LOADING,
        labelId
    };
}

export function endParentExpConfigsLoading(labelId, data) {
    return {
        type: END_PARENT_EXP_CONFIGS_LOADING,
        labelId,
        data
    };
}
