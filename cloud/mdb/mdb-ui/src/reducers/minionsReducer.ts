import {MinionResp, Error} from '../models/deployapi'
import {MinionsAction, MinionsActionsTypes} from "../actions/minionsAction";

import produce from 'immer'

export interface IMinionsState {
    minions: MinionResp[] | undefined;
    error: Error|null;
}

export const initialMinionsState : IMinionsState = {
    minions: [],
    error: null,
};

export default function mastersReducer(state: IMinionsState = initialMinionsState,
                                       action: MinionsAction) {
    return produce(state, draft => {
        switch (action.type) {
            case MinionsActionsTypes.MINIONS_LISTED:
                draft.minions = action.payload;
                break;
            case MinionsActionsTypes.MINIONS_LISTING_FAILED:
                draft.error = action.error;
                draft.minions = undefined;
                break;
        }
    });
}
