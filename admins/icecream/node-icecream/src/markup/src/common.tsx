export let API = "/v1";

export interface ContainerInfo {
    fqdn: string;
    cpu: number;
    ram: number;
    diskSize: number;
    project: string;
    network: string;
    filesystem: string;
    image: string;
    name: string;
    status?: string;
}

export interface Project {
    [name: string]: ProjectInfo;
}

export interface ProjectInfo {
    cpu: number;
    diskSize: number;
    ram: number;
    profiles: string[];
    resps: string[];
}

export interface PhysicalHostInfo {
    fqdn: string;
    cpu: number;
    ram: number;
    pool: string;
    datacenter: string;
    diskSize: number;
    diskType: string;
    containers: ContainerInfo[];
}

export interface MetaDataInfo {
    images: string[];
    profiles: string[];
    datacenters: string[];
    diskType: string[];
    projects: string[];
    filesystems: string[];
}

