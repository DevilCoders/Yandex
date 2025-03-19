import { EditBox } from "../common/components/editbox";
import { Selector } from "../common/components/selector";
import { getUrl, stdCatch } from "../common/ajax";
import { API,
         ContainerInfo,
         PhysicalHostInfo,
         MetaDataInfo,
         Project } from "../common";
import { CreateHost } from "../components/createhost";
import { PhysicalHost } from "./distribution/physical";

interface DistributionState {
    physicalHosts: PhysicalHostInfo[];
    filter: string;
    dcSelected: number;
    projectSelected: number;
    dom0ProjectSelected: number;
    hddTypeSelected: number;

    dcList: string[];
    projectList: string[];
    fileSystemList: string[];
    hddList: string[];
    imageList: string[];
    profileList: string[];
    projects: Project;
}

interface DistributionProps {
    pushState: (url: string) => void;
}

const dom0Projects = ["Все", "alet", "media", "music", "tokk"];

export class Distribution extends React.Component<DistributionProps,
                                                  DistributionState> {

    constructor() {
        super();
        this.state = {
            filter: "",
            dcSelected: 0,
            projectSelected: 0,
            dom0ProjectSelected: 0,
            hddTypeSelected: 0,
            physicalHosts: [],
            dcList: [],
            hddList: [],
            imageList: [],
            projectList: [],
            fileSystemList: [],
            profileList: [],
            projects: null,
        };
    }

    componentDidMount() {
        document.title = "Общее распределение — Яндекс.Мороженое";

        Promise.all([
            getUrl(`${API}/list`),
            getUrl(`${API}/info`),
            getUrl(`${API}/project`),
        ])
        .then(function(data: [PhysicalHostInfo[], MetaDataInfo, Project]){
            for (let i in data[0]) {
                for (let c in data[0][i].containers) {
                    data[0][i].containers[c].fqdn = data[0][i]
                                                    .containers[c]
                                                    .name
                                                    .replace(/--/g, ".")
                                                    + ".yandex.net";
                }
            }
            this.setState({
                physicalHosts: data[0],
                dcList: data[1].datacenters,
                hddList: data[1].diskType,
                imageList: data[1].images,
                profileList: data[1].profiles,
                projectList: data[1].projects,
                fileSystemList: data[1].filesystems,
                projects: data[2],
            });
        }.bind(this))
        .catch(stdCatch);
    }

    onFilterChange(data: string) {
        this.setState({filter: data} as DistributionState);
    }

    onDcChange(index: number) {
        this.setState({dcSelected: index} as DistributionState);
    }

    onProjectChange(index: number) {
        this.setState({projectSelected: index} as DistributionState);
    }

    onDom0ProjectChange(index: number) {
        this.setState({dom0ProjectSelected: index} as DistributionState);
    }

    onHddTypeChange(index: number) {
        this.setState({hddTypeSelected: index} as DistributionState);
    }

    onCreateContainer(info: ContainerInfo, physicalFqdn: string) {
        // Run when the container is created, updates the markup
        for (let p in this.state.physicalHosts) {
            if (this.state.physicalHosts[p].fqdn === physicalFqdn) {
                this.state.physicalHosts[p].containers.push(info);
                this.setState({} as DistributionState);
                return;
            }
        }
    }

    render(): JSX.Element {
        let hosts: JSX.Element[] = [];
        let projectFilter = "";
        if (this.state.projectSelected > 0) {
            projectFilter = this.state.projectList[this.state.projectSelected - 1];
        }
        let dom0ProjectFilter = "";
        if (this.state.dom0ProjectSelected > 0) {
            dom0ProjectFilter = dom0Projects[this.state.dom0ProjectSelected];
        }
        for (let i in this.state.physicalHosts) {
            if (this.state.dom0ProjectSelected !== 0) {
                if (this.state.physicalHosts[i].pool !== dom0ProjectFilter) {
                    continue;
                }
            }
            if (this.state.dcSelected === 0
                || this.state.dcList[this.state.dcSelected - 1]
                   === this.state.physicalHosts[i].datacenter) {
                if (this.state.hddTypeSelected === 0
                    || this.state.hddList[this.state.hddTypeSelected - 1]
                       === this.state.physicalHosts[i].diskType) {
                    if (this.state.filter === ""
                        // tslint:disable-next-line:max-line-length
                        || PhysicalHost.hasVisibleContainers(this.state.physicalHosts[i].containers,
                            this.state.filter,
                            projectFilter)) {
                        hosts.push(<PhysicalHost key={i}
                            filter={this.state.filter}
                            projectFilter={projectFilter}
                            pushState={this.props.pushState}
                            {...this.state.physicalHosts[i]} />);
                    }
                }
            }
        }

        if (hosts.length === 0) {
            hosts.push(<tbody key="none">
                <tr>
                    <td colSpan={6}>Нет контейнеров,
                                    подходящих по условиям фильтров.</td>
                </tr>
            </tbody>);
        }

        let dcs = this.state.dcList.slice();
        dcs.unshift("Все");
        let projects = this.state.projectList.slice();
        projects.unshift("Все");
        let hddtype = this.state.hddList.slice();
        hddtype.unshift("Все");

        return <div className="distribution">
            <h1>Распределение контейнеров</h1>
            <div className="flex">
                <table>
                    <tbody>
                        <tr>
                            <th></th>
                            <th>CPU</th>
                            <th>Память</th>
                            <th>Диск</th>
                            <th>Проект</th>
                            <th/>
                        </tr>
                    </tbody>
                    { hosts }
                </table>
                <div>
                    <h1>Фильтры</h1>
                    <EditBox
                        value={this.state.filter}
                        onChange={this.onFilterChange.bind(this)}
                        placeholder="Фильтр по названию"
                    />
                    <form>
                        <div>
                            <label>Датацентр</label>
                            <Selector
                                values={dcs}
                                selected={this.state.dcSelected}
                                onChange={this.onDcChange.bind(this)}
                            />
                        </div>
                        <div>
                            <label>Проект виртуалки</label>
                            <Selector
                                values={projects}
                                selected={this.state.projectSelected}
                                onChange={this.onProjectChange.bind(this)}
                            />
                        </div>
                        <div>
                            <label>Проект dom0</label>
                            <Selector
                                values={dom0Projects}
                                selected={this.state.dom0ProjectSelected}
                                onChange={this.onDom0ProjectChange.bind(this)}
                            />
                        </div>
                        <div>
                            <label>Диск</label>
                            <Selector
                                values={hddtype}
                                selected={this.state.hddTypeSelected}
                                onChange={this.onHddTypeChange.bind(this)}
                            />
                        </div>
                    </form>
                    <hr />
                    <CreateHost
                        onCreateContainer={this.onCreateContainer.bind(this)}
                        dcList={this.state.dcList}
                        projectList={this.state.projectList}
                        showInput={ true }
                        imageList={this.state.imageList}
                        fileSystemList={this.state.fileSystemList}
                        isUpdate={false}
                        projects={this.state.projects} />
                </div>
            </div>
        </div>;
    }
}
