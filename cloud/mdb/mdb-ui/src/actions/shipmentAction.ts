import {Error} from "../models/deployapi"
import {IShipmentDeep} from "../models";

export enum ShipmentActionsTypes {
    SHIPMENT_LOAD = 'SHIPMENTS/LOAD',
    SHIPMENT_LOADING = 'SHIPMENTS/LOADING',
    SHIPMENT_LOADED = 'SHIPMENTS/LOADED',
    SHIPMENT_LOADING_FAILED = 'SHIPMENTS/LOADING_FAILED',
}

export interface IShipmentLoad {
    type: ShipmentActionsTypes.SHIPMENT_LOAD;
    shipmentId: string
}

export interface IShipmentLoading {
    type: ShipmentActionsTypes.SHIPMENT_LOADING;
}

export interface IShipmentLoaded {
    type: ShipmentActionsTypes.SHIPMENT_LOADED;
    shipment: IShipmentDeep;
}

export interface IShipmentLoadingFailed {
    type: ShipmentActionsTypes.SHIPMENT_LOADING_FAILED;
    error: Error;
}

export type ShipmentAction = IShipmentLoad | IShipmentLoading | IShipmentLoaded | IShipmentLoadingFailed;

export function shipmentLoad(shipmentId: string) : IShipmentLoad {
    return {
        type: ShipmentActionsTypes.SHIPMENT_LOAD,
        shipmentId,
    }
}

export function shipmentLoading() : IShipmentLoading {
    return {
        type: ShipmentActionsTypes.SHIPMENT_LOADING
    }
}

export function shipmentLoaded(shipment: IShipmentDeep) : IShipmentLoaded {
    return {
        type: ShipmentActionsTypes.SHIPMENT_LOADED,
        shipment
    }
}

export function shipmentLoadingFailed(error: Error) : IShipmentLoadingFailed {
    return {
        type: ShipmentActionsTypes.SHIPMENT_LOADING_FAILED,
        error,
    }
}
