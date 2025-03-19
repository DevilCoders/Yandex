import {HeaderAction, HeaderActionsTypes} from "../actions/headerActions";
import {Error} from '../models/deployapi';

import produce from 'immer'

export interface IHeaderState {
    loadingCount: number;
    lastError: Error | undefined;
}

export const initialHeaderState : IHeaderState = {
    loadingCount: 0,
    lastError: undefined,
};

export default function headerReducer(state: IHeaderState = initialHeaderState,
                                      action: HeaderAction) {
    return produce(state, draft => {
        switch (action.type) {
            case HeaderActionsTypes.HEADER_BEGIN_LOADING:
                draft.loadingCount++;
                break;
            case HeaderActionsTypes.HEADER_END_LOADING:
                draft.loadingCount--;
                break;
            case HeaderActionsTypes.HEADER_ADD_ERROR:
                draft.lastError = action.error;
                break;
            case HeaderActionsTypes.HEADER_REMOVE_ERROR:
                draft.lastError = undefined;
        }
    });
}
