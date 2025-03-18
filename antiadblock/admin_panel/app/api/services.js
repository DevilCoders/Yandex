import {antiadbRequest} from 'app/lib/api';

export default {

    fetchServices: function() {
        return antiadbRequest({
            path: '/services',
            options: {
                method: 'GET'
            }
        });
    },

    searchConfigs: function(pattern, offset, limit, active) {
        return antiadbRequest({
            path: '/search',
            options: {
                method: 'GET'
            },
            queryParams: {
                pattern,
                offset,
                limit,
                active
            }
        });
    },

    fetchAlerts: function() {
        return antiadbRequest({
            path: '/get_all_checks',
            options: {
                method: 'GET'
            }
        });
    }
};
