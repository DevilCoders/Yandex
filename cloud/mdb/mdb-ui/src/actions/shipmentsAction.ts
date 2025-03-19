import {ShipmentResp, Error} from "../models/deployapi"

export enum ShipmentsActionsTypes {
    SHIPMENTS_LIST = 'SHIPMENTS/LIST',
    SHIPMENTS_LISTING = 'SHIPMENTS/LISTING',
    SHIPMENTS_LISTED = 'SHIPMENTS/LISTED',
    SHIPMENTS_LISTING_FAILED = 'SHIPMENTS/LISTING_FAILED',
}

export interface IShipmentsList {
    type: ShipmentsActionsTypes.SHIPMENTS_LIST;
    showErrors: boolean
}

export interface IShipmentsListing {
    type: ShipmentsActionsTypes.SHIPMENTS_LISTING;
}

export interface IShipmentsListed {
    type: ShipmentsActionsTypes.SHIPMENTS_LISTED;
    payload: ShipmentResp[] | undefined
}

export interface IShipmentsListingFailed {
    type: ShipmentsActionsTypes.SHIPMENTS_LISTING_FAILED;
    error: Error;
}

export type ShipmentsAction = IShipmentsList | IShipmentsListing | IShipmentsListed | IShipmentsListingFailed;

export function shipmentsList(showErrors: boolean) : IShipmentsList {
    return {
        type: ShipmentsActionsTypes.SHIPMENTS_LIST,
        showErrors
    }
}

export function shipmentsListing() : IShipmentsListing {
    return {
        type: ShipmentsActionsTypes.SHIPMENTS_LISTING
    }
}

export function shipmentsListed(payload: ShipmentResp[] | undefined) : IShipmentsListed {
    return {
        type: ShipmentsActionsTypes.SHIPMENTS_LISTED,
        payload: payload ? payload : []
    }
}

export function shipmentsListingFailed(error: Error) : IShipmentsListingFailed {
    return {
        type: ShipmentsActionsTypes.SHIPMENTS_LISTING_FAILED,
        error: error,
    }
}