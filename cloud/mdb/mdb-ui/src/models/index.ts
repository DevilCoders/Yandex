import {CommandResp, JobResp, JobResultResp, ShipmentResp} from "./deployapi";

export interface IShipmentDeep {
    shipment: ShipmentResp;
    commands: CommandResp[];
    jobs: JobResp[];
    jobResults: JobResultResp[];
}

export interface IJobResultState {
    name: string;
    __id__: string;
    result: boolean;
    __sls__: string;
    changes: any;
    comment: string;
}

export interface IJobResult {
    id: string;
    fun: string;
    jid: string;
    return: any;
    retcode: number;
    success: boolean;
}
