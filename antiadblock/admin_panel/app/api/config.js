import {antiadbRequest} from '../lib/api';

export default {
    fetchConfig: function (configId, status, expId) {
        return antiadbRequest({
            path: `/config/${configId}`,
            options: {
                method: 'GET'
            },
            queryParams: {
                status,
                exp_id: expId
            }
        });
    },

    fetchExpConfigsId: function (labelId) {
        return antiadbRequest({
            path: `/label/${labelId}/experiment`,
            options: {
                method: 'GET'
            }
        });
    },

    saveConfig: function (labelId, parentId, data, dataSettings, comment) {
        return antiadbRequest({
            path: `/label/${labelId}/config`,
            options: {
                method: 'POST',
                body: {
                    data: data,
                    data_settings: dataSettings,
                    parent_id: parentId,
                    comment
                }
            }
        });
    },

    applyConfig: function (labelId, configId, options) {
        return antiadbRequest({
            path: `/label/${labelId}/config/${configId}/${options.target}`,
            options: {
                method: 'PUT',
                body: {
                    old_id: options.oldConfigId ? options.oldConfigId : null
                }
            }
        });
    },

    fetchCurrentConfig: function (labelId, target) {
        return antiadbRequest({
            path: `/label/${labelId}/config/${target}`,
            options: {
                method: 'GET'
            }
        });
    },

    setArchivedConfig: function (configId, archived) {
        return antiadbRequest({
            path: `/config/${configId}`,
            options: {
                method: 'PATCH',
                body: {
                    archived
                }
            }
        });
    },

    setModerateConfig: function (labelId, configId, approved, comment) {
        return antiadbRequest({
            path: `/label/${labelId}/config/${configId}/moderate`,
            options: {
                method: 'PATCH',
                body: {
                    approved,
                    comment
                }
            }
        });
    }
};
