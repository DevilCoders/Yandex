import {combineEpics, Epic} from "redux-observable";
import {
    MastersAction,
    MastersActionsTypes,
    mastersListed,
    mastersListing,
    mastersListingFailed
} from "../actions/mastersAction";
import {IState} from "../reducers";
import {isOfType} from "typesafe-actions"
import {from, of} from "rxjs";
import {catchError, filter, map, startWith, switchMap} from "rxjs/operators";
import {Api, Error} from "../models/deployapi";

const mastersListEpic: Epic<MastersAction, MastersAction, IState, {deployAPI: Api}> = (action$, _, {deployAPI}) =>
    action$.pipe(
        filter(isOfType(MastersActionsTypes.MASTERS_LIST)),
        switchMap(action =>
            from(deployAPI.v1.getMastersList()).pipe(
                map(response => mastersListed(response.masters)),
                startWith(mastersListing()),
                catchError((e: Error) => of(mastersListingFailed(e))),
            )
        )
    );

export default combineEpics(mastersListEpic)
