import * as heatmap from 'app/actions/heatmap';
import update from 'app/lib/update';

export default function (state, action) {
    const {type} = action;

    state = state || {
        heatmapData: {
            result: {
                heatmap: [],
                ts: 0
            }
        },
        lastUpdateTime: 0,
        dataLoaded: false
    };

    switch (type) {
        case heatmap.START_HEATMAP_LOADING: {
            if (state.dataLoaded) {
                return state;
            }
            return update(state, {
                dataLoaded: {
                    $set: false
                }
            });
        }
        case heatmap.END_HEATMAP_LOADING: {
            return update(state, {
                $merge: {
                    heatmapData: action.heatmapData,
                    dataLoaded: true,
                    lastUpdateTime: new Date()
                }
            });
        }
        default: {
            return state;
        }
    }
}
