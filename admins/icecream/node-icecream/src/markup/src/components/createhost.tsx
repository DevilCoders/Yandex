import { EditBox } from "../common/components/editbox";
import { Selector } from "../common/components/selector";
import { Slider } from "../common/components/slider";
import { Button } from "../common/components/button";
import { getUrl, stdCatch } from "../common/ajax";
import { API, ContainerInfo, Project } from "../common";

interface CreateHostState {
    fqdn: string;
    dc: number;
    project: number;
    image: number;
    profile: number;
    cpu: number;
    ram: number;
    diskType: number;
    diskSize: number;
    fileSystem: number;
    fileSystemList: string[];
    minDiskSize: number;
    profileList: string[];
    enabledButton: boolean;
    maxMem: number;
    maxDiskSize: number;
    maxCpu: number;
    noResources: boolean;
    noResElement: JSX.Element;
}

interface CreateHostProps {
    physicalFqdn?: string;
    dcList?: string[];
    availableCpu?: number;
    availableDisk?: number;
    availableMem?: number;
    projectList: string[];
    fileSystemList: string[];
    imageList: string[];
    isUpdate: boolean;
    showInput?: boolean;
    projects: Project;
    onCreateContainer: (info: ContainerInfo, physicalFqdn: number) => void;
}

interface HddTypeMap {
    [x: number]: string;
}


export class CreateHost extends React.Component<CreateHostProps,
                                                CreateHostState> {
    public static defaultProps: Partial<CreateHostProps> = {
        showInput: false,
    };


    constructor(props: CreateHostProps) {
        super(props);
        this.state = {
            fqdn: "",
            dc: 0,
            project: 0,
            image: 0,
            profile: 0,
            fileSystem: 0,
            cpu: 1,
            ram: 4,
            diskType: 0,
            diskSize: 50,
            minDiskSize: 50,
            profileList: [],
            fileSystemList: [],
            enabledButton: true,
            maxMem: 256,
            maxDiskSize: 50700,
            maxCpu: 56,
            noResources: false,
            noResElement: null,
        };
    }


    onChange(field: string, value: string|number) {
        let state = {} as any;
        if (typeof value === "string") {
            value = String(value).trim();
        }
        state[field] = value;
        this.setState(state as CreateHostState);
        if (field === "project") {
            let project = Object.keys(this.props.projects)[Number(value)];
            let profiles = this.props.projects[project].profiles;
            this.setState({ profileList: profiles });
        }
    }

    componentWillReceiveProps(nextProps: CreateHostProps) {
        if (this.state.profileList.length === 0 ) {
            let project = Object.keys(nextProps.projects)[this.state.project];
            let profiles = nextProps.projects[project].profiles;
            this.setState({ profileList: profiles });
        }
    }

    noResources(ram: number, disk: number): JSX.Element {
        let noRes: JSX.Element;
        let msg: string;
        if (ram === 0) {
            msg = "свободной памяти";
        } else if (disk === 0) {
            msg = "свободного дискового места";
        }
        noRes = <div className="editHost">
                    <h2>Создание</h2>
                    <h1>На dom0 хосте не хватает {msg}</h1>
                </div>;

        return noRes;

    }


    componentWillMount() {
        // set correct memPoints for memory picker
        if (this.props.availableMem !== undefined) {
            if (this.props.availableMem === 0) {
                this.setState({
                    maxMem: 0
                });
            } else {
                let ram = (this.props.availableMem / 1073741824).toFixed(1);
                this.setState({
                    maxMem: Number(ram)
                });
            }
        }

        // Set correct diskPoints for disk picker
        if (this.props.availableDisk !== undefined) {
            if (this.props.availableDisk === 0) {
                this.setState({
                    maxDiskSize: 0,
                });
            } else {
                let maxDiskSize = this.props.availableDisk / 1073741824;
                this.setState({
                    maxDiskSize: maxDiskSize,
                });
            }
        }

        // Set correct maxCpu for disk picker
        if (this.props.availableCpu !== undefined) {
            this.setState({
                maxCpu: this.props.availableCpu
            });
        }

        if (this.props.availableDisk === 0
            || this.props.availableMem === 0) {
                this.setState({
                    noResources: true,
                    noResElement: this.noResources(this.props.availableMem,
                                                   this.props.availableDisk)
                });
            }
    }

    componentDidMount() {
        if (this.props.projects !== null) {
            this.componentWillReceiveProps(this.props);
        }

    }


    onCreate() {
        if (this.state.enabledButton) {
           this.setState({
               enabledButton: !this.state.enabledButton });
            let diskMap = {
                0: "hdd",
                1: "ssd",
                } as HddTypeMap;
            let project = Object.keys(this.props.projects)[this.state.project];
            let x: any = {
                fqdn: this.state.fqdn,
                project: project,
                image: this.props.imageList[this.state.image],
                profiles: [this.state.profileList[this.state.profile]],
                cpu: this.state.cpu,
                filesystem: this.props.fileSystemList[this.state.fileSystem],
                ram: this.state.ram * 1073741824,
                diskSize: this.state.diskSize * 1073741824,
            };
            if (this.state.dc) x["dc"] = this.props.dcList[this.state.dc - 1];
            if (this.props.physicalFqdn) {
                x["physicalFqdn"] = this.props.physicalFqdn;
            } else {
                x["diskType"] = diskMap[this.state.diskType];
            }

            getUrl(`${API}/container`, "POST", JSON.stringify(x))
                .then(function (data: any) {

                    let ci: ContainerInfo = {
                        fqdn: x.fqdn,
                        cpu: x.cpu,
                        filesystem: x.filesystem,
                        ram: x.ram,
                        diskSize: x.diskSize,
                        project: x.project,
                        network: "???",
                        image: x.image,
                        name: x.fqdn
                    };
                    this.props.onCreateContainer(ci, data);
                }.bind(this))
                .catch(stdCatch).then(function () {
                    this.setState({
                        enabledButton: !this.state.enabledButton
                                  } as CreateHostState);
                }.bind(this)).catch(stdCatch);
        }
    }

    render(): JSX.Element {
        if (this.state.fqdn === "" && !this.props.physicalFqdn) {
            return <div>
                <h1>Создание контейнера</h1>
                <EditBox
                    value={this.state.fqdn}
                    onChange={this.onChange.bind(this, "fqdn")}
                    placeholder="Название"
                />
            </div>;
        }

        if (this.state.noResources === true) {
            return <div>{this.state.noResElement}</div>;
        } else {
            let data: JSX.Element[] = [];
            if (this.props.isUpdate) {
                data.push(<h2 key="title">Обновлениe</h2>);
            } else {
                data.push(<h2 key="title">Создание</h2>);
            }

            data.push(<h3 key="fqdn">Hostname</h3>);

            let dcElt: JSX.Element = null;
            if (this.props.dcList) {
                let dcs = this.props.dcList.slice();
                dcs.unshift("Любой");
                dcElt = <div>
                    <label>Датацентр</label>
                    <Selector
                        values={dcs}
                        selected={this.state.dc}
                        onChange={this.onChange.bind(this, "dc")}
                    />
                </div>;
            }

            let diskElt: JSX.Element = null;
            if (!this.props.physicalFqdn) {
                let disks = ["HDD", "SSD"];
                diskElt = <div>
                    <label>Тип дисков</label>
                    <Selector
                        values={disks}
                        selected={this.state.diskType}
                        onChange={this.onChange.bind(this, "diskType")}
                    />
                </div>;
            }

            return <div className="createHost">
                {data}
                <EditBox
                    value={this.state.fqdn}
                    onChange={this.onChange.bind(this, "fqdn")}
                    placeholder="Название"
                />
                <form>
                    {dcElt}
                    <div>
                        <label>Проект</label>
                        <Selector
                            values={Object.keys(this.props.projects)}
                            selected={this.state.project}
                            onChange={this.onChange.bind(this, "project")}
                        />
                    </div>
                    <div>
                        <label>Образ ОС</label>
                        <Selector
                            values={this.props.imageList}
                            selected={this.state.image}
                            onChange={this.onChange.bind(this, "image")}
                        />
                    </div>
                    <div>
                        <label>Профиль</label>
                        <Selector
                            values={this.state.profileList}
                            selected={this.state.profile}
                            onChange={this.onChange.bind(this, "profile")}
                        />
                    </div>
                    <div>
                        <label>Файловая система</label>
                        <Selector
                            values={this.props.fileSystemList}
                            selected={this.state.fileSystem}
                            onChange={this.onChange.bind(this, "fileSystem")}
                        />
                     </div>
                    <div className="hostSlider">
                        <label>Процессоров</label>
                        <Slider
                            min={0}
                            max={this.state.maxCpu}
                            showInput={this.props.showInput}
                            value={this.state.cpu}
                            onChange={this.onChange.bind(this, "cpu")} />
                    </div>
                    <div className="hostSlider">
                        <label>Памяти, Gb</label>
                        <Slider
                            min={0.5}
                            max={this.state.maxMem}
                            step={0.5}
                            value={this.state.ram}
                            showInput={this.props.showInput}
                            onChange={this.onChange.bind(this, "ram")} />
                    </div>
                    {diskElt}
                    <div className="hostSlider">
                        <label>Размер дисков, Gb</label>
                        <Slider
                            min={this.state.minDiskSize}
                            max={this.state.maxDiskSize}
                            showInput={ this.props.showInput }
                            value={this.state.diskSize}
                            onChange={this.onChange.bind(this, "diskSize")} />
                    </div>
                    <div>
                        <Button
                            text="Сохранить"
                            primary={true}
                            disabled={!this.state.enabledButton}
                            onClick={this.onCreate.bind(this)} />
                    </div>
                </form>
            </div>;

        }

    }
}

