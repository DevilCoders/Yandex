import {antiadbRequest} from 'app/lib/api';
import {METRICS} from 'app/enums/graphs';

const enumSbsRunCheck = {
    onlyMyRuns: 'only_my_runs',
    offset: 'offset',
    limit: 'limit',
    fromDate: 'from',
    toDate: 'to',
    sortedBy: 'sortedby'
};

export default {
    fetchService: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchTrend: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/trend_ticket`,
            options: {
                method: 'GET'
            },
            queryParams: {
                type: 'negative_trend'
            }
        });
    },

    createTrend: function (serviceId, datetime) {
        return antiadbRequest({
            path: `/service/${serviceId}/create_ticket`,
            options: {
                method: 'POST',
                body: {
                    datetime: Math.floor((new Date(datetime)).getTime() / 1000),
                    type: 'negative_trend'
                }
            }
        });
    },

    fetchLabels: function (serviceId) {
        return antiadbRequest({
            path: '/labels',
            options: {
                method: 'GET'
            },
            queryParams: {
                service_id: serviceId
            }
        });
    },

    changeLabelParent: function (labelId, parentId) {
        return antiadbRequest({
            path: `/label/${labelId}/change_parent_label`,
            options: {
                method: 'POST',
                body: {
                    parent_label_id: parentId
                }
            }
        });
    },

    createLabel: function (labelId, parentId) {
        return antiadbRequest({
            path: '/label',
            options: {
                method: 'POST',
                body: {
                    label_id: labelId,
                    parent_label_id: parentId
                }
            }
        });
    },

    createService: function (serviceId, name, domain) {
        return antiadbRequest({
            path: '/service',
            options: {
                method: 'POST',
                body: {
                    service_id: serviceId,
                    name,
                    domain
                }
            }
        });
    },

    changePriority: function (serviceId, priority) {
        return antiadbRequest({
            path: `/service/${serviceId}/support_priority`,
            options: {
                method: 'PATCH',
                body: {
                    support_priority: priority
                }
            }
        });
    },

    fetchSchema: function (labelId) {
        return antiadbRequest({
            path: `/label/${labelId}/config/schema`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceConfigs: function (serviceId, offset, limit, filters) {
        return antiadbRequest({
            path: `/label/${serviceId}/configs`,
            options: {
                method: 'GET'
            },
            queryParams: {
                offset,
                limit,
                ...filters
            }
        });
    },

    fetchServiceMetric: function (serviceId, name, range) {
        const queryParams = {
            date_range: range
        };

        // Для метрики по таймингам дополнительный параметр - массив ожидаемых перцентилей
        if (name === METRICS.TIMINGS) {
            queryParams.percentile = [95, 98];
        }

        return antiadbRequest({
            path: `/metrics/${serviceId}/${name}`,
            options: {
                method: 'GET'
            },
            queryParams
        });
    },

    fetchServiceConfig: function (serviceId, configId) {
        return antiadbRequest({
            path: `/service/${serviceId}/config/${configId}`,
            options: {
                method: 'GET'
            }
        });
    },

    applyServiceConfig: function (serviceId, configId, options) {
        return antiadbRequest({
            path: `/service/${serviceId}/config/${configId}/${options.target}`,
            options: {
                method: 'PUT',
                body: {
                    old_id: options.oldConfigId ? options.oldConfigId : null
                }
            }
        });
    },

    applyExperiment: function (configId, options) {
        return antiadbRequest({
            path: `/config/${configId}/experiment`,
            options: {
                method: 'PATCH',
                body: {
                    exp_id: options.comment
                }
            }
        });
    },

    removeExperiment: function (configId) {
        return antiadbRequest({
            path: `/config/${configId}/experiment/remove`,
            options: {
                method: 'PATCH'
            }
        });
    },

    createServiceConfig: function (serviceId, parentId, comment, data) {
        return antiadbRequest({
            path: `/service/${serviceId}/config`,
            options: {
                method: 'POST',
                body: {
                    parent_id: parentId,
                    comment,
                    data
                }
            }
        });
    },

    fetchServiceSetup: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/setup`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceHealth(serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/get_service_checks`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceAudit: function (serviceId, offset, limit, labelId) {
        return antiadbRequest({
            path: `/audit/service/${serviceId}`,
            options: {
                method: 'GET'
            },
            queryParams: {
                offset,
                limit,
                label_id: labelId
            }
        });
    },

    setServiceHealtInProgress(serviceId, checkId, hours) {
        return antiadbRequest({
            path: `/service/${serviceId}/check/${checkId}/in_progress`,
            options: {
                method: 'PATCH',
                body: {
                    hours
                }
            }
        });
    },

    decryptLink: function (serviceId, encrypted) {
        return antiadbRequest({
            path: `/service/${serviceId}/utils/decrypt_url`,
            options: {
                method: 'POST',
                body: {
                    url: encrypted
                }
            }
        });
    },

    disableService: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/disable`,
            options: {
                method: 'POST'
            }
        });
    },

    enableService: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/enable`,
            options: {
                method: 'POST'
            }
        });
    },

    generateToken: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/gen_token`,
            options: {
                method: 'GET'
            }
        });
    },

    addServiceComment: function (serviceId, comment) {
        return antiadbRequest({
            path: `/service/${serviceId}/comment`,
            options: {
                method: 'POST',
                body: {
                    comment
                }
            }
        });
    },

    getServiceComment: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/comment`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceRules: function (serviceId, tags = []) {
        return antiadbRequest({
            path: `/service/${serviceId}/sonar/rules`,
            options: {
                method: 'GET'
            },
            queryParams: {...(tags.length ? {tags: tags.join(',')} : {})}
        });
    },

    fetchServiceSbsProfile: function (serviceId, reqParams) {
        const params = reqParams || {};
        // тут profileId опциональный параметр, если он есть, то бек возвращает профиль с указанным id
        // если его нет, то возвращает текущий рабочий (последний)
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/profile${params.profileId ? `/${params.profileId}` : ''}`,
            options: {
                method: 'GET'
            },
            queryParams: {
                tag: params.tag
            }
        });
    },

    deleteServiceSbsProfileByTag: function (serviceId, tag) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/tag/${tag}`,
            options: {
                method: 'PATCH'
            }
        });
    },

    fetchServiceSbsProfileByTag: function (serviceId, tag) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/tag/${tag}`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceSbsProfileTags: function (serviceId) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/tags`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceSbsProfileTag: function (serviceId, tagName) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/tag/${tagName}`,
            options: {
                method: 'GET'
            }
        });
    },

    changeServiceSbsProfileTag: function (serviceId, tagName, data) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/tag/${tagName}`,
            options: {
                method: 'PATCH',
                body: {
                    data
                }
            }
        });
    },

    saveServiceSbsProfile: function (serviceId, data, tag) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/profile`,
            options: {
                method: 'POST',
                body: {
                    data,
                    tag
                }
             }
        });
    },

    fetchServiceSbsCheckScreenshots: function (serviceId, resultId) {
        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/results/${resultId}`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchSbsLogs: function (resId, reqId, type) {
        return antiadbRequest({
            path: `/sbs_check/results/${resId}/logs/${type}/id/${reqId}`,
            options: {
                method: 'GET'
            }
        });
    },

    fetchServiceSbsResultChecks: function (serviceId, options = {}) {
        const queryParams = Object.keys(enumSbsRunCheck).reduce((res, key) => {
            if (options[key] || Number.isFinite(options[key])) {
                res[enumSbsRunCheck[key]] = options[key];
            }

            return res;
        }, {});

        if (options.sortedBy) {
            queryParams.order = options.isReverseSorted ? 'desc' : 'acs';
        }

        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/results`,
            options: {
                method: 'GET'
            },
            queryParams
        }, 2);
    },

    runServiceSbsChecks: function (serviceId, testing, expId, tag) {
        const body = {};

        if (testing) {
            body.testing = testing;
        }

        if (expId) {
            body.id = expId;
        }

        if (tag) {
            body.tag = tag;
        }

        return antiadbRequest({
            path: `/service/${serviceId}/sbs_check/run`,
            options: {
                method: 'POST',
                body
            }
        });
    },

    setServiceEnableMonitoring: function (serviceId, monitoringName, monitoringIsEnable) {
        const key = `${monitoringName}_enabled`;

        return antiadbRequest({
            path: `/service/${serviceId}/set`,
            options: {
                method: 'POST',
                body: {
                    [key]: monitoringIsEnable
                }
            }
        });
    }
};
