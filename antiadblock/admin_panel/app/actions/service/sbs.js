import serviceApi from 'app/api/service';
import {setGlobalErrors} from 'app/actions/errors';

export const START_SERVICE_SBS_PROFILE_LOADING = 'START_SERVICE_SBS_PROFILE_LOADING';
export const END_SERVICE_SBS_PROFILE_LOADING = 'END_SERVICE_SBS_PROFILE_LOADING';
export const START_SERVICE_SBS_PROFILE_SAVING = 'START_SERVICE_SBS_PROFILE_SAVING';
export const END_SERVICE_SBS_PROFILE_SAVING = 'END_SERVICE_SBS_PROFILE_SAVING';
export const OPEN_SERVICE_SBS_PROFILE = 'OPEN_SERVICE_SBS_PROFILE';
export const CLOSE_SERVICE_SBS_PROFILE = 'CLOSE_SERVICE_SBS_PROFILE';
export const SET_ERROR_VALIDATION_SERVICE_SBS_PROFILE = 'SET_ERROR_VALIDATION_SERVICE_SBS_PROFILE';
export const START_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING = 'START_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING';
export const END_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING = 'END_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING';
export const START_SERVICE_SBS_RESULT_CHECKS = 'START_SERVICE_SBS_RESULT_CHECKS';
export const END_SERVICE_SBS_RESULT_CHECKS = 'END_SERVICE_SBS_RESULT_CHECKS';
export const START_SERVICE_SBS_RUN_CHECKS = 'START_SERVICE_SBS_RUN_CHECKS';
export const END_SERVICE_SBS_RUN_CHECKS = 'END_SERVICE_SBS_RUN_CHECKS';
export const CHANGE_LOG_VISIBILITY = 'CHANGE_LOG_VISIBILITY';
export const START_SBS_LOGS_LOADING = 'START_SBS_LOGS_LOADING';
export const END_SBS_LOGS_LOADING = 'END_SBS_LOGS_LOADING';
export const START_SERVICE_SBS_PROFILE_TAGS_LOADING = 'START_SERVICE_SBS_PROFILE_TAGS_LOADING';
export const END_SERVICE_SBS_PROFILE_TAGS_LOADING = 'END_SERVICE_SBS_PROFILE_TAGS_LOADING';
export const START_SERVICE_SBS_PROFILE_TAG_DELETING = 'START_SERVICE_SBS_PROFILE_TAG_DELETING';
export const END_SERVICE_SBS_PROFILE_TAG_DELETING = 'END_SERVICE_SBS_PROFILE_TAG_DELETING';

export function fetchServiceSbsProfile(serviceId, params) {
    return dispatch => {
        dispatch(startServiceSbsProfileLoading(serviceId));

        return serviceApi.fetchServiceSbsProfile(serviceId, params).then(data => {
            return dispatch(endServiceSbsProfileLoading(serviceId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function deleteServiceSbsProfileByTag(serviceId, tag) {
    return dispatch => {
        dispatch(startServiceSbsProfileTagDeleting(serviceId));

        return serviceApi.deleteServiceSbsProfileByTag(serviceId, tag).then(() => {
            return dispatch(endServiceSbsProfileDeleting(serviceId));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function fetchServiceSbsProfileByTag(serviceId, tag) {
    return dispatch => {
        dispatch(startServiceSbsProfileLoading(serviceId));

        return serviceApi.fetchServiceSbsProfileByTag(serviceId, tag).then(data => {
            return dispatch(endServiceSbsProfileLoading(serviceId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function endServiceSbsProfileTagsLoading(serviceId, data) {
    return {
        type: END_SERVICE_SBS_PROFILE_TAGS_LOADING,
        serviceId,
        data: data.data
    };
}

export function startServiceSbsProfileTagDeleting(serviceId) {
    return {
        type: START_SERVICE_SBS_PROFILE_TAG_DELETING,
        serviceId
    };
}

export function endServiceSbsProfileDeleting(serviceId) {
    return {
        type: END_SERVICE_SBS_PROFILE_TAG_DELETING,
        serviceId
    };
}

export function startServiceSbsProfileTagsLoading(serviceId) {
    return {
        type: START_SERVICE_SBS_PROFILE_TAGS_LOADING,
        serviceId
    };
}

export function fetchServiceSbsProfileTags(serviceId) {
    return dispatch => {
        dispatch(startServiceSbsProfileTagsLoading(serviceId));

        return serviceApi.fetchServiceSbsProfileTags(serviceId).then(data => {
            return dispatch(endServiceSbsProfileTagsLoading(serviceId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function fetchServiceSbsCheckScreenshots(serviceId, runId) {
    return dispatch => {
        dispatch(startServiceSbsCheckScreenshotsLoading(serviceId, runId));

        return serviceApi.fetchServiceSbsCheckScreenshots(serviceId, runId).then(data => {
            return dispatch(endServiceSbsCheckScreenshotsLoading(
                serviceId,
                runId,
                {
                    ...data,
                    cases: prepareScreenshotsCasesLog(data)
                }
            ));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function fetchServiceSbsResultChecks(serviceId, options) {
    return dispatch => {
        dispatch(startServiceSbsResultChecks(serviceId));

        return serviceApi.fetchServiceSbsResultChecks(serviceId, options).then(data => {
            return dispatch(endServiceSbsResultChecks(serviceId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceSbsProfileLoading(serviceId) {
    return {
        type: START_SERVICE_SBS_PROFILE_LOADING,
        serviceId
    };
}

export function startServiceSbsCheckScreenshotsLoading(serviceId, runId) {
    return {
        type: START_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING,
        serviceId,
        runId
    };
}

function createUrl(runId, reqId, type) {
    return runId && reqId && type && `/logs/view/res_id/${runId}/type/${type}/req_id/${reqId}`;
}

function createDefaultLogTitle(keys) {
    let title = {};

    Object.keys(keys).forEach(key => {
        title[key] = key;
    });
    return title;
}

function createDefaultLog(log, url) {
    const body = Object.keys(log.meta).reduce((tables, key) => {
        if (log.meta[key] && typeof log.meta[key] === 'object') {
            tables[key] = {
                title: createDefaultLogTitle(log.meta[key]),
                body: log.meta[key]
            };
        } else {
            tables.all = {
                body: {
                    ...(tables.all.body || {}),
                    [key]: log.meta[key]
                },
                title: {
                    ...(tables.all.title || {}),
                    [key]: key
                }
            };
        }
        return tables;
    }, {
        all: {}
    });

    return {
        body,
        status: log.status,
        url
    };
}

function prepareScreenshotsCaseLog(data, item) {
    return Object.keys(item.logs).reduce((prepLogs, key) => {
        const url = createUrl(data.id, item.headers['x-aab-requestid'], key);

        switch (key) {
            case 'elastic-count':
            case 'elastic-auction':
                if (!prepLogs.logs) {
                    prepLogs.logs = {
                        title: {
                            [key]: {
                                url,
                                status: item.logs[key].status
                            }
                        },
                        body: {
                            [key]: item.logs[key].meta.items
                        }
                    };
                } else {
                    prepLogs.logs.title[key] = {
                        url,
                        status: item.logs[key].status
                    };
                    prepLogs.logs.body[key] = item.logs[key].meta.items;
                }
                break;
            default:
                prepLogs[key] = createDefaultLog(item.logs[key], url);
                break;
        }

        return prepLogs;
    }, {
        cookies: {
            title: {
                cookies: 'cookies',
                value: 'value'
            },
            body: Object.keys(item.cookies).map(key => {
                return {
                    cookies: key,
                    value: item.cookies[key]
                };
            })
        },
        blocked: prepareRulesLogs(item.adblocker, item.rules),
        console: prepareConsoleLogs(item.console_logs)
    });
}

function prepareConsoleLogs(consoleLogs) {
    return {
        body: consoleLogs ? consoleLogs.reduce((data, log) => {
            const newLog = {
                level: log.level,
                message: log.message,
                source: log.source
            };
            if (data[log.level] &&
                data[log.level].body.find(item => (
                    item.message === newLog.message)
                ) === undefined
            ) {
                data[log.level].body.push(newLog);
            } else {
                data[log.level] = {
                    title: {
                        level: 'level',
                        message: 'message',
                        source: 'source'
                    },
                    body: [newLog]
                };
            }
            return data;
        }, {}) : {}
    };
}

function getRuleFromBlocker(adblockerType, rule) {
    switch (adblockerType) {
        case 'Adblock Plus':
        case 'AdBlock':
            return {
                rule: rule.text,
                type: rule.type,
                url: rule.url
            };
        case 'AdGuard':
            return {
                rule: rule.requestRule && rule.requestRule.ruleText,
                type: rule.requestType,
                url: rule.requestUrl
            };
        case 'uBlock Origin':
            return {
                rule: rule.filter && rule.filter.raw,
                type: rule.type,
                url: rule.url
            };
        default:
            return {};
    }
}

function prepareRulesLogs(adblockerType, rules) {
    return rules ? {
        title: {
            rule: 'rule',
            type: 'type',
            url: 'url'
        },
        body: Object
            .values(rules)
            .flat()
            .reduce((prepared, item) => {
                const rule = getRuleFromBlocker(adblockerType, item);
                const isUniqRule = prepared.find((preparedRule => (
                    JSON.stringify(preparedRule) === JSON.stringify(rule)
                ))) === undefined;
                if (isUniqRule) {
                    prepared.push(rule);
                }
                return prepared;
            }, [])
    } : {};
}

function prepareScreenshotsCasesLog(data) {
    return data.cases.map(item => ({
        ...item,
        logs: prepareScreenshotsCaseLog(data, item)
    }));
}

export function endServiceSbsCheckScreenshotsLoading(serviceId, runId, data) {
    return {
        type: END_SERVICE_SBS_CHECK_SCREENSHOTS_LOADING,
        serviceId,
        runId,
        data
    };
}

export function startServiceSbsResultChecks(serviceId) {
    return {
        type: START_SERVICE_SBS_RESULT_CHECKS,
        serviceId
    };
}

export function endServiceSbsProfileLoading(serviceId, data) {
    return {
        type: END_SERVICE_SBS_PROFILE_LOADING,
        serviceId,
        data: data.data,
        id: data.id
    };
}

export function endServiceSbsResultChecks(serviceId, data) {
    return {
        type: END_SERVICE_SBS_RESULT_CHECKS,
        serviceId,
        data
    };
}

export function saveServiceSbsProfile(serviceId, data, tag) {
    return dispatch => {
        dispatch(startServiceSbsProfileSaving(serviceId));

        return serviceApi.saveServiceSbsProfile(serviceId, data, tag).then(() => {
            return dispatch(endServiceSbsProfileSaving(serviceId));
        }, error => {
            if (error.status === 400) {
                return dispatch(setErrorValidationServiceSbsProfile(serviceId, error.response.properties));
            }
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function setErrorValidationServiceSbsProfile(serviceId, validation) {
    return {
        type: SET_ERROR_VALIDATION_SERVICE_SBS_PROFILE,
        serviceId,
        validation
    };
}

export function startServiceSbsProfileSaving(serviceId) {
    return {
        type: START_SERVICE_SBS_PROFILE_SAVING,
        serviceId
    };
}

export function runServiceSbsChecks(serviceId, testing, expid, tag) {
    return dispatch => {
        dispatch(startServiceSbsRunChecks(serviceId));

        return serviceApi.runServiceSbsChecks(serviceId, testing, expid, tag).then(() => {
            return dispatch(endServiceSbsRunChecks(serviceId));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServiceSbsRunChecks(serviceId) {
    return {
        type: START_SERVICE_SBS_RUN_CHECKS,
        serviceId
    };
}

export function endServiceSbsProfileSaving(serviceId) {
    return {
        type: END_SERVICE_SBS_PROFILE_SAVING,
        serviceId
    };
}

export function openServiceSbsProfile(profileId, readOnly) {
    return {
        type: OPEN_SERVICE_SBS_PROFILE,
        profileId,
        readOnly
    };
}

export function closeServiceSbsProfile() {
    return {
        type: CLOSE_SERVICE_SBS_PROFILE
    };
}

export function endServiceSbsRunChecks(serviceId) {
    return {
        type: END_SERVICE_SBS_RUN_CHECKS,
        serviceId
    };
}

export function fetchSbsLogs(resId, reqId, type) {
    return dispatch => {
        dispatch(startSbsLogsLoading());

        return serviceApi.fetchSbsLogs(resId, reqId, type).then(data => {
            return dispatch(endSbsLogsLoading(data));
        }, error => {
            if (error.status === 408) {
                return dispatch(endSbsLogsLoading({
                    data: {
                        items: [],
                        total: 0
                    },
                    schema: {}
                }));
            }
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startSbsLogsLoading() {
    return {
        type: START_SBS_LOGS_LOADING
    };
}

export function endSbsLogsLoading(data) {
    return {
        type: END_SBS_LOGS_LOADING,
        data
    };
}

export function changeLogVisibility(key, value) {
    return dispatch => {
        dispatch({
            type: CHANGE_LOG_VISIBILITY,
            key,
            value
        });

        const ls = {
            ...JSON.parse(localStorage.getItem('sbs-logs-visible') || '{}'),
            [key]: value
        };
        localStorage.setItem('sbs-logs-visible', JSON.stringify(ls));
    };
}
