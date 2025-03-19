import {combineEpics, Epic} from "redux-observable";
import {
    IMastersListingFailed,
    MastersAction,
    MastersActionsTypes,
} from "../actions/mastersAction";
import {filter, map, delay, mapTo} from "rxjs/operators";
import {isOfType} from 'typesafe-actions';
import {
    IHeaderAddError,
    HeaderAction,
    headerBeginLoading,
    headerEndLoading,
    headerAddError,
    headerRemoveError, IHeaderRemoveError, HeaderActionsTypes,
} from "../actions/headerActions";
import {IShipmentsListingFailed, ShipmentsAction, ShipmentsActionsTypes} from "../actions/shipmentsAction";
import {ShipmentActionsTypes} from "../actions/shipmentAction";
import {MinionsActionsTypes} from "../actions/minionsAction";

type LoadingAction = MastersAction | ShipmentsAction;

const headerBeginLoadingEpic: Epic<LoadingAction|HeaderAction, HeaderAction> = (action$) =>
    action$.pipe(
        filter(action => [
            MastersActionsTypes.MASTERS_LISTING,
            ShipmentsActionsTypes.SHIPMENTS_LISTING,
            ShipmentActionsTypes.SHIPMENT_LOADING,
            MinionsActionsTypes.MINIONS_LISTING,
        ].some(type => action.type === type)),
        map(action => headerBeginLoading())
    );

const headerEndLoadingEpic: Epic<LoadingAction|HeaderAction, HeaderAction> = (action$) =>
    action$.pipe(
        filter(action => [
            MastersActionsTypes.MASTERS_LISTED,
            MastersActionsTypes.MASTERS_LISTING_FAILED,
            ShipmentsActionsTypes.SHIPMENTS_LISTED,
            ShipmentsActionsTypes.SHIPMENTS_LISTING_FAILED,
            ShipmentActionsTypes.SHIPMENT_LOADED,
            ShipmentActionsTypes.SHIPMENT_LOADING_FAILED,
            MinionsActionsTypes.MINIONS_LISTED,
            MinionsActionsTypes.MINIONS_LISTING_FAILED,
        ].some(type => action.type === type)),
        map(action => headerEndLoading())
    );

type FailAction = IMastersListingFailed | IShipmentsListingFailed;

const headerAddErrorEpic: Epic<FailAction|IHeaderAddError, IHeaderAddError> = (action$) =>
    action$.pipe(
        filter(action => [
            MastersActionsTypes.MASTERS_LISTING_FAILED,
            ShipmentsActionsTypes.SHIPMENTS_LISTING_FAILED,
            MinionsActionsTypes.MINIONS_LISTING_FAILED,
        ].some(type => action.type === type)),
        map(action => headerAddError(action.error)),
    );

const headerRemoveErrorEpic: Epic<IHeaderAddError | IHeaderRemoveError, IHeaderRemoveError> = (action$) =>
    action$.pipe(
        filter(isOfType(HeaderActionsTypes.HEADER_ADD_ERROR)),
        delay(10000),
        mapTo(headerRemoveError())
    );

export default combineEpics(
    headerBeginLoadingEpic,
    headerEndLoadingEpic,
    headerAddErrorEpic,
    headerRemoveErrorEpic,
)
