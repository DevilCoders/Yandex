/* tslint:disable */
/* eslint-disable */

/*
 * ---------------------------------------------------------------
 * ## THIS FILE WAS GENERATED VIA SWAGGER-TYPESCRIPT-API        ##
 * ##                                                           ##
 * ## AUTHOR: acacode                                           ##
 * ## SOURCE: https://github.com/acacode/swagger-typescript-api ##
 * ---------------------------------------------------------------
 */
type PageToken = string;
export interface Paging {
  token?: PageToken;
}

export type GroupsListResp = Paging & { groups?: GroupResp[] };

export type GroupResp = Group & { mastersCount?: number; minionsCount?: number };

export interface Group {
  /**
   * Group ID
   */
  id?: number;

  /**
   * Groups's name
   */
  name?: string;
}

export type MastersListResp = Paging & { masters?: MasterResp[] };

export type MasterResp = Master & {
  createdAt?: number;
  aliveCheckAt?: number;
  isAlive?: boolean;
  minionsCount?: number;
};

export interface Master {
  /**
   * Masters's fqdn
   */
  fqdn?: string;

  /**
   * List of host aliases
   */
  aliases?: string[];

  /**
   * Which group of masters this master belongs to
   */
  group?: string | null;

  /**
   * Is it allowed to auto-assign minions in its group to this master?
   */
  isOpen?: boolean | null;

  /**
   * Master's description
   */
  description?: string | null;
}

export type MinionsListResp = Paging & { minions?: MinionResp[] };

export type MinionResp = Minion &
  MinionPublicKey & {
    createdAt?: number;
    updatedAt?: number;
    registerUntil?: number;
    master?: string;
    registered?: boolean;
    deleted?: boolean;
  };

export interface Minion {
  /**
   * Minion's fqdn
   */
  fqdn?: string;

  /**
   * Which group of masters this minion belongs to
   */
  group?: string | null;

  /**
   * Is it allowed to auto-assign this minion to masters in its group?
   */
  autoReassign?: boolean | null;
}

export type MinionMaster = MinionPublicKey & { master?: string };

export interface MinionPublicKey {
  /**
   * Minion's public key
   */
  publicKey?: string;
}

export interface MinionChangeList {
  masters?: MinionChange[];
}

export interface MinionChange {
  /**
   * Minion's fqdn
   */
  fqdn?: string;

  /**
   * Master's fqdn
   */
  master?: string;
  action?: "create" | "delete" | "reassign";
  timestamp?: number;
}

export type ShipmentsListResp = Paging & { shipments?: ShipmentResp[] };

export type ShipmentResp = Shipment & {
  id?: string;
  status?: ShipmentStatus;
  otherCount?: number;
  doneCount?: number;
  errorsCount?: number;
  totalCount?: number;
  createdAt?: number;
  updatedAt?: number;
};

export enum ShipmentStatus {
  unknown = "unknown",
  inprogress = "inprogress",
  done = "done",
  error = "error",
  timeout = "timeout",
}

export interface Shipment {
  /**
   * Commands definitions
   */
  commands?: CommandDef[];

  /**
   * FQDNs of target minions for this shipment
   */
  fqdns?: string[];

  /**
   * Number of commands allowed to run simultaneously
   */
  parallel?: number;

  /**
   * Number of commands allowed to run simultaneously
   */
  batchSize?: number;

  /**
   * Number of failed commands after which entire shipment fails. Zero means do not fail on error count.
   */
  stopOnErrorCount?: number;

  /**
   * Timeout in seconds for entire shipment
   */
  timeout?: number;
}

export interface CommandDef {
  /**
   * Command type
   */
  type?: string;

  /**
   * Command arguments
   */
  arguments?: string[];

  /**
   * Timeout in seconds for command
   */
  timeout?: number;
}

export type CommandsListResp = Paging & { commands?: CommandResp[] };

export type CommandResp = CommandDef & {
  id?: string;
  shipmentID?: string;
  fqdn?: string;
  status?: CommandStatus;
  createdAt?: number;
  updatedAt?: number;
};

export enum CommandStatus {
  unknown = "unknown",
  available = "available",
  running = "running",
  done = "done",
  error = "error",
  canceled = "canceled",
  timeout = "timeout",
}

export type JobsListResp = Paging & { jobs?: JobResp[] };

export interface JobResp {
  /**
   * ID of this job in deploy
   */
  id?: string;

  /**
   * ID of this job in underlying deploy system (SaltStack)
   */
  extId?: string;

  /**
   * ID of command that spawned this job
   */
  commandID?: string;
  status?: JobStatus;

  /**
   * Timestamp when job was created
   */
  createdAt?: number;

  /**
   * Timestamp when job was updated
   */
  updatedAt?: number;
}

export enum JobStatus {
  unknown = "unknown",
  running = "running",
  done = "done",
  error = "error",
  timeout = "timeout",
}

export type JobResultsListResp = Paging & { jobResults?: JobResultResp[] };

export type JobResultResp = JobResult & {
  id?: number;
  extID?: string;
  fqdn?: string;
  order?: number;
  status?: JobResultStatus;
  recordedAt?: number;
};

export interface JobResult {
  result?: string;
}

export enum JobResultStatus {
  unknown = "unknown",
  success = "success",
  failure = "failure",
  timeout = "timeout",
  notrunning = "notrunning",
}

/**
 * Collection of service stats
 */
export type Stats = object[][];

export interface Error {
  /**
   * Error description
   */
  message?: string;
}

export type RequestParams = Omit<RequestInit, "body" | "method"> & {
  secure?: boolean;
};

export type RequestQueryParamsType = Record<string, string | string[] | number | number[] | boolean | undefined>;

type ApiConfig<SecurityDataType> = {
  baseUrl?: string;
  baseApiParams?: RequestParams;
  securityWorker?: (securityData: SecurityDataType) => RequestParams;
};

/** Overrided Promise type. Needs for additional typings of `.catch` callback */
type TPromise<ResolveType, RejectType = any> = Omit<Promise<ResolveType>, "then" | "catch"> & {
  then<TResult1 = ResolveType, TResult2 = never>(
    onfulfilled?: ((value: ResolveType) => TResult1 | PromiseLike<TResult1>) | undefined | null,
    onrejected?: ((reason: RejectType) => TResult2 | PromiseLike<TResult2>) | undefined | null,
  ): TPromise<TResult1 | TResult2, RejectType>;
  catch<TResult = never>(
    onrejected?: ((reason: RejectType) => TResult | PromiseLike<TResult>) | undefined | null,
  ): TPromise<ResolveType | TResult, RejectType>;
};

class HttpClient<SecurityDataType> {
  public baseUrl: string = "";
  private securityData: SecurityDataType = null as any;
  private securityWorker: ApiConfig<SecurityDataType>["securityWorker"] = (() => {}) as any;

  private baseApiParams: RequestParams = {
    credentials: "same-origin",
    headers: {
      "Content-Type": "application/json",
    },
    redirect: "follow",
    referrerPolicy: "no-referrer",
  };

  constructor({ baseUrl, baseApiParams, securityWorker }: ApiConfig<SecurityDataType> = {}) {
    this.baseUrl = baseUrl || this.baseUrl;
    this.baseApiParams = baseApiParams || this.baseApiParams;
    this.securityWorker = securityWorker || this.securityWorker;
  }

  public setSecurityData = (data: SecurityDataType) => {
    this.securityData = data;
  };

  private addQueryParam(query: RequestQueryParamsType, key: string) {
    return (
      encodeURIComponent(key) +
      "=" +
      encodeURIComponent(Array.isArray(query[key]) ? (query[key] as any).join(",") : query[key])
    );
  }

  protected addQueryParams(query?: RequestQueryParamsType): string {
    const fixedQuery = query || {};
    const keys = Object.keys(fixedQuery).filter((key) => "undefined" !== typeof fixedQuery[key]);
    return keys.length === 0 ? "" : `?${keys.map((key) => this.addQueryParam(fixedQuery, key)).join("&")}`;
  }

  private mergeRequestOptions(params: RequestParams, securityParams?: RequestParams): RequestParams {
    return {
      ...this.baseApiParams,
      ...params,
      ...(securityParams || {}),
      headers: {
        ...(this.baseApiParams.headers || {}),
        ...(params.headers || {}),
        ...((securityParams && securityParams.headers) || {}),
      },
    };
  }

  private safeParseResponse = <T = any, E = any>(response: Response): TPromise<T, E> =>
    response
      .json()
      .then((data) => data)
      .catch((e) => response.text);

  public request = <T = any, E = any>(
    path: string,
    method: string,
    { secure, ...params }: RequestParams = {},
    body?: any,
    secureByDefault?: boolean,
  ): TPromise<T, E> =>
    fetch(`${this.baseUrl}${path}`, {
      // @ts-ignore
      ...this.mergeRequestOptions(params, (secureByDefault || secure) && this.securityWorker(this.securityData)),
      method,
      body: body ? JSON.stringify(body) : null,
    }).then(async (response) => {
      const data = await this.safeParseResponse<T, E>(response);
      if (!response.ok) throw data;
      return data;
    });
}

/**
 * @title MDB Deploy API
 * @version 1.0.0
 * Provides API for deploying via SaltStack.
 */
export class Api<SecurityDataType = any> extends HttpClient<SecurityDataType> {
  v1 = {
    /**
     * @tags groups
     * @name GetGroupsList
     * @summary Returns list of groups
     * @request GET:/v1/groups
     * @response `200` `GroupsListResp` List of groups
     * @response `default` `Error`
     */
    getGroupsList: (
      query?: { pageSize?: number; pageToken?: string; sortOrder?: "asc" | "desc" },
      params?: RequestParams,
    ) => this.request<GroupsListResp, Error>(`/v1/groups${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags groups
     * @name CreateGroup
     * @summary Creates new group
     * @request POST:/v1/groups
     * @response `200` `GroupResp` Created group
     * @response `default` `Error`
     */
    createGroup: (body: Group, params?: RequestParams) =>
      this.request<GroupResp, Error>(`/v1/groups`, "POST", params, body),

    /**
     * @tags groups
     * @name GetGroup
     * @summary Returns specific group
     * @request GET:/v1/groups/{groupname}
     * @response `200` `GroupResp` Group
     * @response `default` `Error`
     */
    getGroup: (groupname: string, params?: RequestParams) =>
      this.request<GroupResp, Error>(`/v1/groups/${groupname}`, "GET", params, null),

    /**
     * @tags groups
     * @name DeleteGroup
     * @summary Deletes group
     * @request DELETE:/v1/groups/{groupname}
     * @response `200` `any` Group deleted
     * @response `default` `Error`
     */
    deleteGroup: (groupname: string, params?: RequestParams) =>
      this.request<any, Error>(`/v1/groups/${groupname}`, "DELETE", params, null),

    /**
     * @tags masters
     * @name GetMastersList
     * @summary Returns list of masters
     * @request GET:/v1/masters
     * @response `200` `MastersListResp` List of masters
     * @response `default` `Error`
     */
    getMastersList: (query?: { pageSize?: number; pageToken?: string }, params?: RequestParams) =>
      this.request<MastersListResp, Error>(`/v1/masters${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags masters
     * @name CreateMaster
     * @summary Creates new master
     * @request POST:/v1/masters
     * @response `200` `MasterResp` Created master
     * @response `default` `Error`
     */
    createMaster: (body: Master, params?: RequestParams) =>
      this.request<MasterResp, Error>(`/v1/masters`, "POST", params, body),

    /**
     * @tags masters
     * @name GetMaster
     * @summary Returns specific master
     * @request GET:/v1/masters/{fqdn}
     * @response `200` `MasterResp` Master
     * @response `default` `Error`
     */
    getMaster: (fqdn: string, params?: RequestParams) =>
      this.request<MasterResp, Error>(`/v1/masters/${fqdn}`, "GET", params, null),

    /**
     * @tags masters
     * @name UpsertMaster
     * @summary Creates new master or updates the old one
     * @request PUT:/v1/masters/{fqdn}
     * @response `200` `MasterResp` Upserted master
     * @response `default` `Error`
     */
    upsertMaster: (fqdn: string, body: Master, params?: RequestParams) =>
      this.request<MasterResp, Error>(`/v1/masters/${fqdn}`, "PUT", params, body),

    /**
     * @tags masters
     * @name DeleteMaster
     * @summary Deletes master
     * @request DELETE:/v1/masters/{fqdn}
     * @response `200` `any` Master deleted
     * @response `default` `Error`
     */
    deleteMaster: (fqdn: string, params?: RequestParams) =>
      this.request<any, Error>(`/v1/masters/${fqdn}`, "DELETE", params, null),

    /**
     * @tags masters
     * @name GetMasterMinions
     * @summary Returns list of minions assigned to specific master
     * @request GET:/v1/masters/{fqdn}/minions
     * @response `200` `MinionsListResp` List of minions
     * @response `default` `Error`
     */
    getMasterMinions: (fqdn: string, query?: { pageSize?: number; pageToken?: string }, params?: RequestParams) =>
      this.request<MinionsListResp, Error>(
        `/v1/masters/${fqdn}/minions${this.addQueryParams(query)}`,
        "GET",
        params,
        null,
      ),

    /**
     * @tags masters
     * @name GetMasterMinionsChanges
     * @summary Returns changelog in minions for specified master
     * @request GET:/v1/masters/{fqdn}/minions/changes
     * @response `200` `MinionChangeList` List of minions changes
     * @response `default` `Error`
     */
    getMasterMinionsChanges: (
      fqdn: string,
      query?: { pageSize?: number; pageToken?: string; fromTimestamp?: number },
      params?: RequestParams,
    ) =>
      this.request<MinionChangeList, Error>(
        `/v1/masters/${fqdn}/minions/changes${this.addQueryParams(query)}`,
        "GET",
        params,
        null,
      ),

    /**
     * @tags minions
     * @name GetMinionsList
     * @summary Returns list of minions
     * @request GET:/v1/minions
     * @response `200` `MinionsListResp` List of minions
     * @response `default` `Error`
     */
    getMinionsList: (query?: { pageSize?: number; pageToken?: string }, params?: RequestParams) =>
      this.request<MinionsListResp, Error>(`/v1/minions${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags minions
     * @name CreateMinion
     * @summary Creates new minion
     * @request POST:/v1/minions
     * @response `200` `MinionResp` Created minion
     * @response `default` `Error`
     */
    createMinion: (body: Minion, params?: RequestParams) =>
      this.request<MinionResp, Error>(`/v1/minions`, "POST", params, body),

    /**
     * @tags minions
     * @name GetMinion
     * @summary Returns specific minion
     * @request GET:/v1/minions/{fqdn}
     * @response `200` `MinionResp` Minion
     * @response `default` `Error`
     */
    getMinion: (fqdn: string, params?: RequestParams) =>
      this.request<MinionResp, Error>(`/v1/minions/${fqdn}`, "GET", params, null),

    /**
     * @tags minions
     * @name UpsertMinion
     * @summary Creates new minion or updates the old one
     * @request PUT:/v1/minions/{fqdn}
     * @response `200` `MinionResp` Upserted minion
     * @response `default` `Error`
     */
    upsertMinion: (fqdn: string, body: Minion, params?: RequestParams) =>
      this.request<MinionResp, Error>(`/v1/minions/${fqdn}`, "PUT", params, body),

    /**
     * @tags minions
     * @name DeleteMinion
     * @summary Deletes minion
     * @request DELETE:/v1/minions/{fqdn}
     * @response `200` `any` Minion deleted
     * @response `default` `Error`
     */
    deleteMinion: (fqdn: string, params?: RequestParams) =>
      this.request<any, Error>(`/v1/minions/${fqdn}`, "DELETE", params, null),

    /**
     * @tags minions
     * @name GetMinionMaster
     * @summary Returns master of specific minion. Does NOT require authentication.
     * @request GET:/v1/minions/{fqdn}/master
     * @response `200` `MinionMaster` Minion's master
     * @response `default` `Error`
     */
    getMinionMaster: (fqdn: string, params?: RequestParams) =>
      this.request<MinionMaster, Error>(`/v1/minions/${fqdn}/master`, "GET", params, null),

    /**
     * @tags minions
     * @name RegisterMinion
     * @summary Registers minion's public key.
     * @request POST:/v1/minions/{fqdn}/register
     * @response `200` `MinionResp` Registered minion
     * @response `default` `Error`
     */
    registerMinion: (fqdn: string, body: MinionPublicKey, params?: RequestParams) =>
      this.request<MinionResp, Error>(`/v1/minions/${fqdn}/register`, "POST", params, body),

    /**
     * @tags minions
     * @name UnregisterMinion
     * @summary Unregisters minion's public key. Minion can be registered again as if it was just created.
     * @request POST:/v1/minions/{fqdn}/unregister
     * @response `200` `MinionResp` Unregistered minion
     * @response `default` `Error`
     */
    unregisterMinion: (fqdn: string, params?: RequestParams) =>
      this.request<MinionResp, Error>(`/v1/minions/${fqdn}/unregister`, "POST", params, null),

    /**
     * @tags commands
     * @name CreateJobResult
     * @summary Create job result for specific job.
     * @request POST:/v1/minions/{fqdn}/jobs/{jobId}/results
     * @response `200` `JobResultResp` Job result
     * @response `default` `Error`
     */
    createJobResult: (fqdn: string, jobId: string, body: JobResult, params?: RequestParams) =>
      this.request<JobResultResp, Error>(`/v1/minions/${fqdn}/jobs/${jobId}/results`, "POST", params, body),

    /**
     * @tags commands
     * @name GetShipmentsList
     * @summary Returns list of shipments
     * @request GET:/v1/shipments
     * @response `200` `ShipmentsListResp` List of shipments
     * @response `default` `Error`
     */
    getShipmentsList: (
      query?: {
        pageSize?: number;
        pageToken?: string;
        fqdn?: string;
        shipmentStatus?: string;
        sortOrder?: "asc" | "desc";
      },
      params?: RequestParams,
    ) => this.request<ShipmentsListResp, Error>(`/v1/shipments${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags commands
     * @name CreateShipment
     * @summary Creates new shipment
     * @request POST:/v1/shipments
     * @response `200` `ShipmentResp` Created shipment
     * @response `default` `Error`
     */
    createShipment: (body: Shipment, params?: RequestParams) =>
      this.request<ShipmentResp, Error>(`/v1/shipments`, "POST", params, body),

    /**
     * @tags commands
     * @name GetShipment
     * @summary Returns specific shipment
     * @request GET:/v1/shipments/{shipmentId}
     * @response `200` `ShipmentResp` Shipment
     * @response `default` `Error`
     */
    getShipment: (shipmentId: string, params?: RequestParams) =>
      this.request<ShipmentResp, Error>(`/v1/shipments/${shipmentId}`, "GET", params, null),

    /**
     * @tags commands
     * @name GetCommandsList
     * @summary Returns list of commands
     * @request GET:/v1/commands
     * @response `200` `CommandsListResp` List of commands
     * @response `default` `Error`
     */
    getCommandsList: (
      query?: {
        pageSize?: number;
        pageToken?: string;
        shipmentId?: string;
        fqdn?: string;
        commandStatus?: string;
        sortOrder?: "asc" | "desc";
      },
      params?: RequestParams,
    ) => this.request<CommandsListResp, Error>(`/v1/commands${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags commands
     * @name GetCommand
     * @summary Returns specific command
     * @request GET:/v1/commands/{commandId}
     * @response `200` `CommandResp` Command
     * @response `default` `Error`
     */
    getCommand: (commandId: string, params?: RequestParams) =>
      this.request<CommandResp, Error>(`/v1/commands/${commandId}`, "GET", params, null),

    /**
     * @tags commands
     * @name GetJobsList
     * @summary Returns list of jobs
     * @request GET:/v1/jobs
     * @response `200` `JobsListResp` List of jobs
     * @response `default` `Error`
     */
    getJobsList: (
      query?: {
        pageSize?: number;
        pageToken?: string;
        shipmentId?: string;
        fqdn?: string;
        extJobId?: string;
        jobStatus?: string;
        sortOrder?: "asc" | "desc";
      },
      params?: RequestParams,
    ) => this.request<JobsListResp, Error>(`/v1/jobs${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags commands
     * @name GetJob
     * @summary Returns specific job
     * @request GET:/v1/jobs/{jobId}
     * @response `200` `JobResp` Job
     * @response `default` `Error`
     */
    getJob: (jobId: string, params?: RequestParams) =>
      this.request<JobResp, Error>(`/v1/jobs/${jobId}`, "GET", params, null),

    /**
     * @tags commands
     * @name GetJobResultsList
     * @summary Returns job results
     * @request GET:/v1/jobresults
     * @response `200` `JobResultsListResp` Job results
     * @response `default` `Error`
     */
    getJobResultsList: (
      query?: {
        pageSize?: number;
        pageToken?: string;
        fqdn?: string;
        jobId?: string;
        jobResultStatus?: string;
        sortOrder?: "asc" | "desc";
      },
      params?: RequestParams,
    ) => this.request<JobResultsListResp, Error>(`/v1/jobresults${this.addQueryParams(query)}`, "GET", params, null),

    /**
     * @tags commands
     * @name GetJobResult
     * @summary Returns specific job result
     * @request GET:/v1/jobresults/{jobResultId}
     * @response `200` `JobResultResp` Job result
     * @response `default` `Error`
     */
    getJobResult: (jobResultId: string, params?: RequestParams) =>
      this.request<JobResultResp, Error>(`/v1/jobresults/${jobResultId}`, "GET", params, null),

    /**
     * @tags common
     * @name Ping
     * @summary Reports service status
     * @request GET:/v1/ping
     * @response `200` `any` Service is alive and well
     * @response `default` `Error`
     */
    ping: (params?: RequestParams) => this.request<any, Error>(`/v1/ping`, "GET", params, null),

    /**
     * @tags common
     * @name Stats
     * @summary Reports service stats
     * @request GET:/v1/stats
     * @response `200` `Stats` Reports service stats
     * @response `default` `Error`
     */
    stats: (params?: RequestParams) => this.request<Stats, Error>(`/v1/stats`, "GET", params, null),
  };
}
