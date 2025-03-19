import {MinionResp, Error} from "../models/deployapi"

export enum MinionsActionsTypes {
    MINIONS_LIST = 'MINIONS/LIST',
    MINIONS_LISTING = 'MINIONS/LISTING',
    MINIONS_LISTED = 'MINIONS/LISTED',
    MINIONS_LISTING_FAILED = 'MINIONS/LISTING_FAILED',
}

export interface IMinionsList {
    type: MinionsActionsTypes.MINIONS_LIST;
}

export interface IMinionsListing {
    type: MinionsActionsTypes.MINIONS_LISTING;
}

export interface IMinionsListed {
    type: MinionsActionsTypes.MINIONS_LISTED;
    payload: MinionResp[] | undefined;
}

export interface IMinionsListingFailed {
    type: MinionsActionsTypes.MINIONS_LISTING_FAILED;
    error: Error;
}

export type MinionsAction = IMinionsList | IMinionsListing | IMinionsListed | IMinionsListingFailed;

export function minionsList() : IMinionsList {
    return {
        type: MinionsActionsTypes.MINIONS_LIST
    }
}

export function minionsListing() : IMinionsListing {
    return {
        type: MinionsActionsTypes.MINIONS_LISTING
    }
}

export function minionsListed(payload: MinionResp[] | undefined) : IMinionsListed {
    return {
        type: MinionsActionsTypes.MINIONS_LISTED,
        payload: payload ? payload : []
    }
}

export function minionsListingFailed(error: Error) : IMinionsListingFailed {
    return {
        type: MinionsActionsTypes.MINIONS_LISTING_FAILED,
        error: error,
    }
}
