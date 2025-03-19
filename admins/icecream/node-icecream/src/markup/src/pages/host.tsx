/// <reference types="numeral" />


import { ProgressBar } from "../common/components/progressbar";
import { EditBox } from "../common/components/editbox";
import { generateColors } from "../common/colorgen";
import { getUrl, stdCatch } from "../common/ajax";
import { API,
         ContainerInfo,
         PhysicalHostInfo,
         MetaDataInfo,
         Project } from "../common";
import { CreateHost } from "../components/createhost";
import { UpdateHost } from "../components/updatehost";
import { ContainerRow } from "./host/containerrow";
import { MigrateHost } from "../components/migratehost";

function formatNumber(n: number): string {
    if (n === undefined) return "";
    return numeral(n).format("0.1").replace(",", " ");
}

interface HostPageProps {
    fqdn: string;
}


interface HostPageState {
    info: PhysicalHostInfo;
    extraShow: string;
    contFqdn: string;
    imageList: string[];
    availableCpu: number;
    availableMem: number;
    availableDisk: number;
    profileList: string[];
    fileSystemList: string[];
    projectList: string[];
    projects: Project;
    extraElement: JSX.Element;
}

export class HostPage extends React.Component<HostPageProps, HostPageState> {
    static displayName = "HostPage";

    constructor() {
        super();
        this.state = {
            info: {
                fqdn: "",
                cpu: 0,
                ram: 0,
                datacenter: "",
                diskSize: 0,
                diskType: "",
                pool: "",
                containers: [],
            },
            extraShow: "",
            contFqdn: "",
            imageList: [],
            profileList: [],
            projectList: [],
            fileSystemList: [],
            availableCpu: 0,
            availableMem: 0,
            availableDisk: 0,
            projects: null,
            extraElement: null,
        };
    }

    shortName() {
        return this.props.fqdn.replace(/(\.\w+)?\.yandex\.net$/, "");
    }

    componentDidMount() {
        document.title = `${this.shortName()} —
                            статистика сервера — Яндекс.Мороженое`;
        let usedRam = 0;
        let usedCpu = 0;
        let usedDisk = 0;

        Promise.all([
            getUrl(`${API}/list?fqdn=${this.props.fqdn}`),
            getUrl(`${API}/info`),
            getUrl(`${API}/project`),
        ])
            .then(function (data: [PhysicalHostInfo[], MetaDataInfo, Project]) {
                this.setState({
                    info: data[0][0],
                    imageList: data[1].images,
                    profileList: data[1].profiles,
                    projectList: data[1].projects,
                    fileSystemList: data[1].filesystems,
                    projects: data[2],
                });

                for (let i in this.state.info.containers) {
                    let cont = this.state.info.containers[i];
                    usedRam += cont.ram;
                    usedCpu += cont.cpu;
                    usedDisk += cont.diskSize;
                }
                this.setState({
                    availableCpu: this.state.info.cpu - usedCpu,
                    availableMem: this.state.info.ram - usedRam,
                    availableDisk: this.state.info.diskSize - usedDisk,
                });
            }.bind(this))
            .catch(stdCatch);

    }

    onRemovedContainer(fqdn: string) {
        for (let i in this.state.info.containers) {
            if (this.state.info.containers[i].fqdn === fqdn) {
                delete this.state.info.containers[i];
                this.forceUpdate();
                return;
            }
        }
    }

    onRestartedContainer(fqdn: string) {
        for (let i in this.state.info.containers) {
            if (this.state.info.containers[i].fqdn === fqdn) {
                this.state.info.containers[i].status = "Restart";
                this.forceUpdate();
                return;
            }
        }
    }

    onPausedContainer(fqdn: string) {
        for (let i in this.state.info.containers) {
            if (this.state.info.containers[i].fqdn === fqdn) {
                this.state.info.containers[i].status = "Stopped";
                this.forceUpdate();
                return;
            }
        }
    }

    onRunContainer(fqdn: string) {
        for (let i in this.state.info.containers) {
            if (this.state.info.containers[i].fqdn === fqdn) {
                this.state.info.containers[i].status = "Running";
                this.forceUpdate();
                return;
            }
        }
    }

    onCreatedContainer(info: ContainerInfo, physicalFqdn: string) {
        this.state.info.containers.push(info);
        this.forceUpdate();
    }

    onUpdatedContainer(fqdn: string,
                       cpu: number,
                       mem: number,
                       diskSize: number) {
        let ram = mem * 1073741824;
        let disk = diskSize * 1073741824;
        for (let i in this.state.info.containers) {
            if (this.state.info.containers[i].fqdn === fqdn) {
                this.state.info.containers[i].ram = ram;
                this.state.info.containers[i].cpu = cpu;
                this.state.info.containers[i].diskSize = disk;
                this.forceUpdate();
            }
        }
        this.forceUpdate();

    }

    onUpdateContainer(info: ContainerInfo,
                      diskSize: number,
                      mem: number,
                      cpu: number) {
        if (this.state.extraShow === "update") {
            this.setState({ extraShow: null } as HostPageState);
            this.setState({ extraElement: null } as HostPageState);

        } else {
            this.setState({ extraShow: "update" } as HostPageState);
            this.setState({
                extraElement: <UpdateHost
                    key="create"
                    info={info}
                    cpu={cpu}
                    diskSize={diskSize}
                    availableCpu={this.state.availableCpu}
                    availableMem={this.state.availableMem}
                    availableDisk={this.state.availableDisk}
                    onUpdatedContainer={this.onUpdatedContainer.bind(this)}
                    mem={mem} />
            } as HostPageState);

        }
    }

    onMigrateContainer(name: string, filesystem: string) {
        if (this.state.extraShow === "migrate") {
            this.setState({ extraShow: null } as HostPageState);
            this.setState({ extraElement: null } as HostPageState);
        } else {
            this.setState({ extraShow: "migrate" } as HostPageState);
            this.setState({
                extraElement: <MigrateHost
                    key="migrate"
                    name={name}
                    filesystem={filesystem}
                    fileSystemList={this.state.fileSystemList}
                    onFinishedMigration={this.onFinishedMigration.bind(this)}
                    />
            } as HostPageState);
        }
    }

    onFinishedMigration(name: string) {
        for (let i in this.state.info.containers) {
            if (this.state.info.containers[i].fqdn === name) {
                delete this.state.info.containers[i];
                this.forceUpdate();
                break;
            }
        }
    }

    onCreateContainer() {
        if (this.state.extraShow === "create") {
            this.setState({ extraShow: null } as HostPageState);
            this.setState({ extraElement: null } as HostPageState);
        } else {
            this.setState({ extraShow: "create" } as HostPageState);
            this.setState({
                extraElement: <CreateHost
                    key="create"
                    isUpdate={false}
                    imageList={this.state.imageList}
                    projectList={this.state.projectList}
                    fileSystemList={this.state.fileSystemList}
                    physicalFqdn={this.props.fqdn}
                    projects={this.state.projects}
                    showInput={true}
                    availableCpu={this.state.availableCpu}
                    availableDisk={this.state.availableDisk}
                    availableMem={this.state.availableMem}
                    onCreateContainer={this.onCreatedContainer.bind(this)} />
            } as HostPageState);

        }
    }

    render(): JSX.Element {
        let usedRam = 0;
        let usedCpu = 0;
        let usedDisk = 0;

        let containers: JSX.Element[] = [];
        let valsRam: number[] = [];
        let valsCpu: number[] = [];
        let valsDisk: number[] = [];

        let colors = generateColors(this.state.info.containers.length);

        for (let i in this.state.info.containers) {
            let cont = this.state.info.containers[i];

            usedRam += cont.ram;
            usedCpu += cont.cpu;
            usedDisk += cont.diskSize;

            valsRam.push(cont.ram * 100 / this.state.info.ram);
            valsCpu.push(cont.cpu * 100 / this.state.info.cpu);
            valsDisk.push(cont.diskSize * 100 / this.state.info.diskSize);

            containers.push(<ContainerRow
                key={cont.fqdn}
                color={colors[i]}
                info={this.state.info.containers[i]}
                onRemovedContainer={this.onRemovedContainer.bind(this)}
                onUpdateContainer={this.onUpdateContainer.bind(this)}
                onRestartedContainer={this.onRestartedContainer.bind(this)}
                onPausedContainer={this.onPausedContainer.bind(this)}
                onRunContainer={this.onRunContainer.bind(this)}
                onMigrateContainer={this.onMigrateContainer.bind(this)}
            />);
        }

        if (containers.length === 0) {
            containers.push(<tr key="none">
                                <td colSpan={4}>Контейнеров нет</td>
                            </tr>);
        }

        return <div className="host">
            <ReactCSSTransitionGroup transitionName="animate"
                                     transitionEnterTimeout={150}
                                     transitionLeaveTimeout={150}>
                <div key="main">
                    <h1>Физический сервер {this.props.fqdn} ({this.state.info.pool})</h1>

                    <h2>Ресурсы</h2>
                    <table className="equicol">
                        <tbody>
                            <tr>
                                <th></th>
                                <th>Всего</th>
                                <th>Выделено</th>
                                <th>Свободно</th></tr>
                            <tr>
                                <th>Память</th>
                                <td>{formatNumber(this.state.info.ram
                                                  / 1073741824)}</td>
                                <td>{formatNumber(usedRam
                                                  / 1073741824)}</td>
                                <td>{formatNumber((this.state.info.ram
                                                   - usedRam)
                                                   / 1073741824)}</td>
                                <th><ProgressBar values={valsRam}
                                                 colors={colors}
                                                 big={true} /></th>
                            </tr>
                            <tr>
                                <th>Процессоры</th>
                                <td>{this.state.info.cpu}</td>
                                <td>{usedCpu}</td>
                                <td>{this.state.info.cpu - usedCpu}</td>
                                <th><ProgressBar values={valsCpu}
                                                 colors={colors}
                                                 big={true} /></th>
                            </tr>
                            <tr>
                                <th>Диски {this.state.info.diskType}</th>
                                <td>{formatNumber(this.state.info.diskSize
                                                  / 1073741824)}</td>
                                <td>{formatNumber(usedDisk / 1073741824)}</td>
                                <td>{formatNumber((this.state.info.diskSize
                                                   - usedDisk)
                                                   / 1073741824)}</td>
                                <th><ProgressBar values={valsDisk}
                                                 colors={colors}
                                                 big={true} /></th>
                            </tr>
                        </tbody>
                    </table>

                    <h2>Контейнеры</h2>
                    <table className="right">
                        <tbody>
                            <tr>
                                <th></th>
                                <th>Память</th>
                                <th>Процессоры</th>
                                <th>Диски</th>
                                <th>ФС</th></tr>
                            {containers}
                            <tr>
                                <th colSpan={4}></th>
                                <th style={{ textAlign: "right" }}>
                                    <i className="fa fa-fw fa-plus"
                                       onClick={
                                           this.onCreateContainer.bind(this)} />
                                </th>
                            </tr>
                        </tbody>
                    </table>
                </div>
                {this.state.extraElement}
            </ReactCSSTransitionGroup>
        </div>;
    }
}
