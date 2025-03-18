import heatmapApi from 'app/api/heatmap';
import {setErrors, clearErrors} from 'app/actions/errors';
import {getErrors, getHeatmap} from 'app/reducers/index';
import {getById} from 'app/reducers/errors';
import {timeIsOver} from 'app/lib/errors-logic';

export const START_HEATMAP_LOADING = 'START_HEATMAP_LOADING';
export const END_HEATMAP_LOADING = 'END_HEATMAP_LOADING';

export function fetchHeatmap() {
    return (dispatch, getState) => {
        dispatch(startHeatmapLoading());

        return heatmapApi.fetchHeatmap().then(heatmap => {
            const errors = getById(getErrors(getState()), 'dashboard');
            if (errors && errors.length) {
                dispatch(clearErrors('dashboard'));
            }

            return dispatch(endHeatmapLoading(heatmap));
        }, error => {
            const lastUpdateTime = getHeatmap(getState()).lastUpdateTime;

            if (timeIsOver(lastUpdateTime, 5)) {
                return dispatch(setErrors('dashboard', [error.message]));
            }
        });
    };
}

export function startHeatmapLoading() {
    return {
        type: START_HEATMAP_LOADING
    };
}

export function endHeatmapLoading(heatmap) {
    return {
        heatmapData: heatmap,
        type: END_HEATMAP_LOADING
    };
}
