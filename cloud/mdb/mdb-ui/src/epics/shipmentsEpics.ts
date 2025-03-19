import {combineEpics, Epic} from "redux-observable";
import {
    ShipmentsAction,
    ShipmentsActionsTypes,
    shipmentsListed,
    shipmentsListing,
    shipmentsListingFailed
} from "../actions/shipmentsAction";
import {IState} from "../reducers";
import {isOfType} from "typesafe-actions"
import {from, of} from "rxjs";
import {catchError, filter, map, startWith, switchMap} from "rxjs/operators";
import {Api, Error} from "../models/deployapi";

const shipmentsListEpic: Epic<ShipmentsAction, ShipmentsAction, IState, {deployAPI: Api}> = (action$, _, {deployAPI}) =>
    action$.pipe(
        filter(isOfType(ShipmentsActionsTypes.SHIPMENTS_LIST)),
        switchMap(action =>
            from(deployAPI.v1.getShipmentsList({
                pageSize: 30,
                shipmentStatus: action.showErrors ? 'error' : undefined,
                sortOrder: "desc"
            })).pipe(
                map(response => shipmentsListed(response.shipments)),
                startWith(shipmentsListing()),
                catchError((e: Error) => of(shipmentsListingFailed(e)))
            )
        )
    );

export default combineEpics(shipmentsListEpic)
