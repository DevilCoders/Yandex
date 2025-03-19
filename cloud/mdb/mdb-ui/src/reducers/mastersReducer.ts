import {MasterResp, Error} from '../models/deployapi'
import {MastersAction, MastersActionsTypes} from "../actions/mastersAction";

import produce from 'immer'

export interface IMastersState {
    masters: MasterResp[] | undefined;
    error: Error|null;
}

export const initialMastersState : IMastersState = {
    masters: [],
    error: null,
};

export default function mastersReducer(state: IMastersState = initialMastersState,
                                       action: MastersAction) {
    return produce(state, draft => {
        switch (action.type) {
            case MastersActionsTypes.MASTERS_LISTED:
                draft.masters = action.payload;
                break;
            case MastersActionsTypes.MASTERS_LISTING_FAILED:
                draft.error = action.error;
                draft.masters = undefined;
                break;
        }
    });
}
