import {ShipmentAction, ShipmentActionsTypes} from "../actions/shipmentAction";

import produce from 'immer'
import {IShipmentDeep} from "../models";

export interface IShipmentState {
    shipmentId?: string;
    shipment?: IShipmentDeep;
}

export const initialShipmentState : IShipmentState = {};

export default function shipmentsReducer(state: IShipmentState = initialShipmentState,
                                         action: ShipmentAction) {
    return produce(state, draft => {
       switch (action.type) {
           case ShipmentActionsTypes.SHIPMENT_LOAD:
               draft.shipmentId = action.shipmentId;
               break;
           case ShipmentActionsTypes.SHIPMENT_LOADED:
               draft.shipment = action.shipment;
               break;
           case ShipmentActionsTypes.SHIPMENT_LOADING_FAILED:
               draft = {}
               break;
       }
    });
}
