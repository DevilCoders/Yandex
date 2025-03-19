import {ShipmentResp, Error} from '../models/deployapi'
import {ShipmentsAction, ShipmentsActionsTypes} from "../actions/shipmentsAction";

import produce from 'immer'

export interface IShipmentsState {
    shipments: ShipmentResp[] | undefined;
    showErrorsOnly: boolean;
    error: Error | undefined;
}

export const initialShipmentsState : IShipmentsState = {
    shipments: [],
    showErrorsOnly: false,
    error: undefined,
};

export default function shipmentsReducer(state: IShipmentsState = initialShipmentsState,
                                         action: ShipmentsAction) {
    return produce(state, draft => {
       switch (action.type) {
           case ShipmentsActionsTypes.SHIPMENTS_LIST:
               draft.showErrorsOnly = action.showErrors;
               break;
           case ShipmentsActionsTypes.SHIPMENTS_LISTED:
               draft.shipments = action.payload;
               break;
           case ShipmentsActionsTypes.SHIPMENTS_LISTING_FAILED:
               draft.error = action.error;
               draft.shipments = undefined;
               break;
       }
    });
}
