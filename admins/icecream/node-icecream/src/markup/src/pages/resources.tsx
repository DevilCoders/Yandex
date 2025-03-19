import { API, ContainerInfo, PhysicalHostInfo, MetaDataInfo } from "../common";
import { ProjectRow,
         ProjectInfo,
         ProjectResources } from "./resources/projectrow";
import { DcRow, DcRowInfo } from "./resources/dcrow";
import { getUrl, stdCatch } from "../common/ajax";

interface ProjectInfoMap {
    [name: string]: ProjectInfo;
}

interface DistributionState {
    projects: ProjectInfoMap;
    physical: ProjectResources;
    dcinfo: Hash;
}

interface Hash {
    [dc: string]: DcRowInfo;
}

// Sum field of two objects
function sumObjects(obj1: DcRowInfo, obj2: DcRowInfo) {
    for (let key of Object.keys(obj2)) {
        (obj1 as any)[key] += (obj2 as any)[key];
    }
}

export class Resources extends React.Component<{}, DistributionState> {
    constructor() {
        super();
        this.state = {
            projects: {},
            dcinfo: {},
            physical: {
                cpu: 0,
                ram: 0,
                disk: 0,
            }
        };
    }

    componentDidMount() {
        Promise.all([
            getUrl(`${API}/list`),
            getUrl(`${API}/info`),
        ])
        .then(([physicalList, meta]: [PhysicalHostInfo[], MetaDataInfo]) => {
            // prepare vars
            let physicalTotal: ProjectResources = {
                cpu: 0,
                ram: 0,
                disk: 0,
            };

            let projects: ProjectInfoMap = {};
            for (let i of meta.projects) {
                projects[i] = {
                    name: i,
                    cpu: 0,
                    ram: 0,
                    disk: 0,
                };
            }

            // fill data
            for (let physName in physicalList) {
                physicalTotal.cpu += physicalList[physName].cpu;
                physicalTotal.ram += physicalList[physName].ram;
                physicalTotal.disk += physicalList[physName].diskSize;

                for (let contName in physicalList[physName].containers) {
                    let cont = physicalList[physName].containers[contName];
                    projects[cont.project].cpu += cont.cpu;
                    projects[cont.project].ram += cont.ram;
                    projects[cont.project].disk += cont.diskSize;
                }
            }

            let totalCount: number = 0;
            let dcCount: Hash = {};


            for (let physName of physicalList) {
                let dc = physName.datacenter;
                let diskType = physName.diskType;
                let used = physName.containers.length > 0 ? true : false;

                if (dc in dcCount !== true) {
                    dcCount[dc] = {
                        total: 0,
                        free: 0,
                        hdd_free: 0,
                        hdd_used: 0,
                        ssd_free: 0,
                        ssd_used: 0
                    };
                }

                dcCount[dc].total += 1;

                if (used === true) {
                    if (diskType === "hdd") {
                        dcCount[dc].hdd_used += 1;
                    } else {
                        dcCount[dc].ssd_used += 1;
                    }
                } else {
                    if (diskType === "hdd") {
                        dcCount[dc].hdd_free += 1;
                    } else {
                        dcCount[dc].ssd_free += 1;
                    }

                }
            }

            let allKey = "All";
            dcCount[allKey] = {
                total: 0,
                free: 0,
                hdd_free: 0,
                hdd_used: 0,
                ssd_free: 0,
                ssd_used: 0
            };
            for (let rec in dcCount) {
                if (rec !== allKey) {
                    let hdd_used = dcCount[rec].hdd_used;
                    let ssd_used = dcCount[rec].ssd_used;
                    let total = dcCount[rec].total;
                    dcCount[rec].free = total - hdd_used - ssd_used;

                    sumObjects(dcCount[allKey], dcCount[rec]);
                }
            }

            this.setState({
                projects: projects,
                physical: physicalTotal,
                dcinfo: dcCount,
            });
        })
        .catch(stdCatch);
    }

    render(): JSX.Element {
        // calculate total project used
        let projectTotal: ProjectInfo = {
            name: "Итого",
            cpu: 0,
            ram: 0,
            disk: 0,
        };
        let projectNames: string[] = [];
        for (let project in this.state.projects) {
            projectTotal.cpu += this.state.projects[project].cpu;
            projectTotal.ram += this.state.projects[project].ram;
            projectTotal.disk += this.state.projects[project].disk;
            projectNames.push(project);
        }
        projectNames.sort();


        // create project rows
        let projectRow: JSX.Element[] = [];
        for (let project of projectNames) {
            projectRow.push(<ProjectRow
                                key={project}
                                info={this.state.projects[project]}
                                totalPhysical={this.state.physical}
                                totalProjects={projectTotal}
                                />);
        }

        // create dcrows
        let dcRow: JSX.Element[] = [];
        for (let dc in this.state.dcinfo) {
            dcRow.push(<DcRow
                        key={dc}
                        info={this.state.dcinfo[dc]}
                        location={dc}
            />);
        }
        dcRow.reverse();

        // render out
        return <div className="resources">
            <h1>Ресурсы</h1>
            <div className="flex">
                <table>
                    <tbody>
                        <tr>
                            <th></th>
                            <th>CPU</th>
                            <th>Память</th>
                            <th>Диск</th>
                            <th>Проектов, %</th>
                            <th>Железа, %</th>
                        </tr>
                        {projectRow}
                        <ProjectRow
                            info={projectTotal}
                            totalProjects={projectTotal}
                            totalPhysical={this.state.physical}
                            />
                    </tbody>
                </table>
            </div>
            <h1>Железо</h1>
            <div className="flex">
                <table>
                    <tbody>
                        <tr>
                            <th></th>
                            <th>Total</th>
                            <th>Unused</th>
                            <th>HDD Used</th>
                            <th>HDD Free</th>
                            <th>SSD Used</th>
                            <th>SSD Free</th>
                        </tr>
                        {dcRow}
                    </tbody>
                </table>
            </div>
        </div>;

    }
}
