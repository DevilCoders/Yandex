import {combineEpics, Epic} from "redux-observable";
import {
    ShipmentAction,
    ShipmentActionsTypes,
    shipmentLoaded,
    shipmentLoading,
    shipmentLoadingFailed
} from "../actions/shipmentAction";
import {IState} from "../reducers";
import {isOfType} from "typesafe-actions"
import {from, of} from "rxjs";
import {catchError, filter, map, startWith, switchMap} from "rxjs/operators";
import {Api, Error} from "../models/deployapi";
import {IShipmentDeep} from "../models"

const getShipmentDeep = async (shipmentId: string, deployAPI: Api): Promise<IShipmentDeep> => {
    const shipment = await deployAPI.v1.getShipment(shipmentId);
    const commandsResp = await deployAPI.v1.getCommandsList({shipmentId});
    const commands = commandsResp.commands || [];
    const jobsResp = await deployAPI.v1.getJobsList({shipmentId: shipment.id});
    const jobs = jobsResp.jobs || [];
    const jobResultsResp = await Promise.all(jobs.map(async job => deployAPI.v1.getJobResultsList({jobId: job.extId})));
    const jobResults = jobResultsResp.flatMap(jrr => jrr.jobResults || [])

    return {
        shipment,
        commands,
        jobs,
        jobResults
    };
};

const shipmentLoadEpic: Epic<ShipmentAction, ShipmentAction, IState, {deployAPI: Api}> = (action$, _, {deployAPI}) =>
    action$.pipe(
        filter(isOfType(ShipmentActionsTypes.SHIPMENT_LOAD)),
        switchMap(action =>
            from(getShipmentDeep(action.shipmentId, deployAPI)).pipe(
                map(response => shipmentLoaded(response)),
                startWith(shipmentLoading()),
                catchError((e: Error) => of(shipmentLoadingFailed(e)))
            )
        )
    );

export default combineEpics(shipmentLoadEpic)
