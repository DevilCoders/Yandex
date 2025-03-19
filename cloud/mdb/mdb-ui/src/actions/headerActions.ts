import {Error} from "../models/deployapi";

export enum HeaderActionsTypes {
    HEADER_BEGIN_LOADING = 'HEADER/BEGIN_LOADING',
    HEADER_END_LOADING = 'HEADER/END_LOADING',
    HEADER_ADD_ERROR = 'HEADER/ADD_ERROR',
    HEADER_REMOVE_ERROR = 'HEADER/REMOVE_ERROR',
}

export interface IHeaderBeginLoading {
    type: HeaderActionsTypes.HEADER_BEGIN_LOADING;
}

export interface IHeaderEndLoading {
    type: HeaderActionsTypes.HEADER_END_LOADING;
}

export interface IHeaderAddError {
    type: HeaderActionsTypes.HEADER_ADD_ERROR;
    error: Error;
}

export interface IHeaderRemoveError {
    type: HeaderActionsTypes.HEADER_REMOVE_ERROR;
}

export type HeaderAction = IHeaderBeginLoading | IHeaderEndLoading | IHeaderAddError | IHeaderRemoveError;

export function headerBeginLoading() : IHeaderBeginLoading {
    return {
        type: HeaderActionsTypes.HEADER_BEGIN_LOADING
    }
}

export function headerEndLoading() : IHeaderEndLoading {
    return {
        type: HeaderActionsTypes.HEADER_END_LOADING
    }
}

export function headerAddError(error: Error) : IHeaderAddError {
    return {
        type: HeaderActionsTypes.HEADER_ADD_ERROR,
        error: error,
    }
}

export function headerRemoveError() : IHeaderRemoveError {
    return {
        type: HeaderActionsTypes.HEADER_REMOVE_ERROR
    }
}
