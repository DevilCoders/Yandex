import {antiadbRequest} from 'app/lib/api';

export default {

    fetchHeatmap: function () {
        return antiadbRequest({
            path: '/get_heatmap',
            options: {
                method: 'GET'
            }
        });
    }
};
