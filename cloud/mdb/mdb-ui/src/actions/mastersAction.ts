import {MasterResp, Error} from "../models/deployapi"

export enum MastersActionsTypes {
    MASTERS_LIST = 'MASTERS/LIST',
    MASTERS_LISTING = 'MASTERS/LISTING',
    MASTERS_LISTED = 'MASTERS/LISTED',
    MASTERS_LISTING_FAILED = 'MASTERS/LISTING_FAILED',
}

export interface IMastersList {
    type: MastersActionsTypes.MASTERS_LIST;
}

export interface IMastersListing {
    type: MastersActionsTypes.MASTERS_LISTING;
}

export interface IMastersListed {
    type: MastersActionsTypes.MASTERS_LISTED;
    payload: MasterResp[] | undefined;
}

export interface IMastersListingFailed {
    type: MastersActionsTypes.MASTERS_LISTING_FAILED;
    error: Error;
}

export type MastersAction = IMastersList | IMastersListing | IMastersListed | IMastersListingFailed;

export function mastersList() : IMastersList {
    return {
        type: MastersActionsTypes.MASTERS_LIST
    }
}

export function mastersListing() : IMastersListing {
    return {
        type: MastersActionsTypes.MASTERS_LISTING
    }
}

export function mastersListed(payload: MasterResp[] | undefined) : IMastersListed {
    return {
        type: MastersActionsTypes.MASTERS_LISTED,
        payload: payload ? payload : []
    }
}

export function mastersListingFailed(error: Error) : IMastersListingFailed {
    return {
        type: MastersActionsTypes.MASTERS_LISTING_FAILED,
        error: error,
    }
}
