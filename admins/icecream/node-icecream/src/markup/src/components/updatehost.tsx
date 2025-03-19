import { EditBox } from "../common/components/editbox";
import { Selector } from "../common/components/selector";
import { Slider } from "../common/components/slider";
import { Button } from "../common/components/button";
import { getUrl, stdCatch } from "../common/ajax";
import { API, ContainerInfo } from "../common";

interface UpdateHostState {
    diskSize: number;
    ram: number;
    cpu: number;
    enabledButton: boolean;
    currDiskSize: number;
    maxDiskSize: number;
    maxMem: number;
    maxCpu: number;
    noResources: boolean;
    noResElement: JSX.Element;
}

interface UpdateHostProps {
    info: ContainerInfo;
    diskSize: number;
    mem: number;
    cpu: number;
    availableCpu: number;
    availableMem: number;
    availableDisk: number;
    onUpdatedContainer: (fqdn: string, cpu: number,
                         mem: number, diskSize: number) => void;
}


export class UpdateHost extends React.Component<UpdateHostProps,
                                                UpdateHostState> {
    constructor() {
        super();
        this.state = {
            diskSize: -1,
            ram: -1,
            cpu: -1,
            enabledButton: true,
            maxDiskSize: 1024,
            currDiskSize: 0,
            maxMem: 0,
            maxCpu: 56,
            noResources: false,
            noResElement: null,
        };
    }

    onChange(field: string, value: string|number) {
        let state = {} as any;
        state[field] = value;
        this.setState(state as UpdateHostState);
    }

    onUpdate() {
        if (this.state.enabledButton) {
            this.setState({ enabledButton: !this.state.enabledButton
                            } as UpdateHostState);
            let data = {
                "resize": {
                    "diskSize": this.state.diskSize * 1073741824,
                    "ram": this.state.ram * 1073741824,
                    "cpu": this.state.cpu,
                }
            };

            getUrl(`${API}/container/${this.props.info.fqdn}`,
                    "PUT",
                    JSON.stringify(data))
                .then(function () {
                    this.props.onUpdatedContainer(this.props.info.fqdn,
                                                  this.state.cpu,
                                                  this.state.ram,
                                                  this.state.diskSize);
                }.bind(this))
                .catch(stdCatch).then(function () {
                    this.setState({ enabledButton: !this.state.enabledButton });
                }.bind(this)).catch(stdCatch);
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
                    <h2>{this.props.info.fqdn}</h2>
                    <h1>На dom0 хосте не хватает {msg}</h1>
                </div>;

        return noRes;

    }

    componentWillMount() {
        this.setState({
            cpu: this.props.info.cpu,
            ram: Math.round(this.props.info.ram / 1073741824),
            diskSize: Math.round(this.props.info.diskSize / 1073741824),
        });

        if (this.props.availableMem !== undefined) {
            let currRam = this.props.info.ram / 1073741824;
            if (this.props.availableMem === 0) {
                this.setState({
                    maxMem: currRam
                });
            } else {
                let maxMem = this.props.availableMem + this.props.info.ram;
                maxMem = maxMem / 1073741824;
                this.setState({
                    maxMem: maxMem
                });
            }
        }

        // Set correct diskPoints for disk picker
        if (this.props.availableDisk !== undefined) {
            let currDiskSize = this.props.info.diskSize / 1073741824;
            this.setState({
                currDiskSize: currDiskSize
            });
            if (this.props.availableDisk === 0) {
                this.setState({
                    maxDiskSize: currDiskSize,
                });
            } else {
                let maxDiskSize = 0;
                maxDiskSize = (this.props.availableDisk
                               + this.props.info.diskSize) / 1073741824;
                this.setState({
                    maxDiskSize: maxDiskSize
                });

            }
        } else {
            let maxDiskSize = this.state.maxDiskSize / 1073741824 ;
            this.setState({
                maxDiskSize: maxDiskSize
            });
        }

        // Set correct maxCpu for disk picker
        if (this.props.availableCpu !== undefined) {
            this.setState({
                maxCpu: this.props.availableCpu + this.props.info.cpu
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


    render(): JSX.Element {
        if (this.state.noResources === true) {
            return <div>{this.state.noResElement}</div>;
        } else {
            let errorMsg: JSX.Element;
            return <div className="editHost">
                <h2>{this.props.info.fqdn}</h2>
                <form>
                    <div className="hostSlider">
                        <label>Процессоров</label>
                        <Slider
                            min={0}
                            max={this.state.maxCpu}
                            showInput={true}
                            value={this.state.cpu}
                            onChange={this.onChange.bind(this, "cpu")} />
                    </div>
                    <div className="hostSlider">
                        <label>Памяти, Gb</label>
                        <Slider
                            min={0.5}
                            max={this.state.maxMem}
                            step={0.5}
                            showInput={true}
                            value={this.state.ram}
                            onChange={this.onChange.bind(this, "ram")} />
                    </div>
                    <div className="hostSlider">
                        <label>Размер дисков, Gb</label>
                        <Slider
                            min={this.state.currDiskSize}
                            max={this.state.maxDiskSize}
                            showInput={true}
                            value={this.state.diskSize}
                            onChange={this.onChange.bind(this, "diskSize")} />
                    </div>
                    <div>
                        <Button
                            text="Изменить"
                            primary={true}
                            disabled={!this.state.enabledButton}
                            onClick={this.onUpdate.bind(this)} />
                    </div>
                </form>
            </div>;

        }
    }
}
