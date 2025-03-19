import { combineReducers } from 'redux';
import shipmentsReducer, {initialShipmentsState, IShipmentsState} from "./shipmentsReducer";
import mastersReducer, {IMastersState, initialMastersState} from "./mastersReducer";
import headerReducer, {IHeaderState, initialHeaderState} from "./headerReducer";
import shipmentReducer, {initialShipmentState, IShipmentState} from "./shipmentReducer";
import minionsReducer, {IMinionsState, initialMinionsState} from "./minionsReducer";

export interface IState {
    shipments: IShipmentsState;
    masters: IMastersState;
    header: IHeaderState;
    shipment: IShipmentState;
    minions: IMinionsState;
}

export const initialState: IState = {
    shipments: initialShipmentsState,
    masters: initialMastersState,
    header: initialHeaderState,
    shipment: initialShipmentState,
    minions: initialMinionsState,
};

export default combineReducers({
    shipments: shipmentsReducer,
    masters: mastersReducer,
    header: headerReducer,
    shipment: shipmentReducer,
    minions: minionsReducer,
});
