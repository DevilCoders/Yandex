import {combineEpics, Epic} from "redux-observable";
import {
    MinionsAction,
    MinionsActionsTypes,
    minionsListed,
    minionsListing,
    minionsListingFailed
} from "../actions/minionsAction";
import {IState} from "../reducers";
import {isOfType} from "typesafe-actions"
import {from, of} from "rxjs";
import {catchError, filter, map, startWith, switchMap} from "rxjs/operators";
import {Api, Error} from "../models/deployapi";

const minionsListEpic: Epic<MinionsAction, MinionsAction, IState, {deployAPI: Api}> = (action$, _, {deployAPI}) =>
    action$.pipe(
        filter(isOfType(MinionsActionsTypes.MINIONS_LIST)),
        switchMap(action =>
            from(deployAPI.v1.getMinionsList()).pipe(
                map(response => minionsListed(response.minions)),
                startWith(minionsListing()),
                catchError((e: Error) => of(minionsListingFailed(e))),
            )
        )
    );

export default combineEpics(minionsListEpic)
