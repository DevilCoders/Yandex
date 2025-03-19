import { EditBox } from "../common/components/editbox";
import { Button } from "../common/components/button";
import { Selector } from "../common/components/selector";
import { getUrl, stdCatch } from "../common/ajax";
import { API, ContainerInfo } from "../common";

interface MigrateHostState {
    physicalName: string;
    filesystem: number;
}

interface MigrateHostProps {
    name: string;
    filesystem: string;
    fileSystemList: string[];
    onFinishedMigration: (name: string) => void;
}

export class MigrateHost extends React.Component<MigrateHostProps,
                                                MigrateHostState> {
    constructor() {
        super();
        this.state = {
            physicalName: "",
            filesystem: 0,
        };
    }

    componentDidMount() {
        this.setState({
            filesystem: this.props.fileSystemList.indexOf(this.props.filesystem)
        });

    }
    onStart() {
        const map = ["hdd", "ssd"];
        getUrl(`${API}/container/${this.props.name}`,
                "PUT",
                JSON.stringify({
                    migrate: "move",
                    target_fqdn: this.state.physicalName,
                    target_fs: this.props.fileSystemList[this.state.filesystem],
                }))
            .then(function () {
                this.props.onFinishedMigration(this.props.name);
            }.bind(this))
            .catch(stdCatch);

    }

    onChange(field: string, value: string|number) {
        let state = {} as any;
        if (typeof value === "string") {
            value = String(value).trim();
        }
        state[field] = value;
        this.setState(state as MigrateHostState);
    }

    onDefaultMigrationStart() {
        getUrl(`${API}/container/${this.props.name}`,
                "PUT",
                JSON.stringify({
                    migrate: "move",
                    target_fqdn: "",
                    target_fs: "",
                }))
            .then(function () {
                this.props.onFinishedMigration(this.props.name);
            }.bind(this))
            .catch(stdCatch);

    }

    onSetPhysicalName(name: string) {
        this.setState({ physicalName: name });
    }

    render(): JSX.Element {
        return <div className="editHost">
            <h2>Миграция</h2>
            <form>
                <div className="hostSlider">
                    <label>Виртуалка</label>
                    <div>
                        { this.props.name }
                    </div>
                </div>
                <hr/>
                <div className="hostSlider">
                    <label>Новая dom0</label>
                    <EditBox
                        value={this.state.physicalName}
                        onChange={this.onSetPhysicalName.bind(this)}/>
                </div>
                <div className="hostSlider">
                <label>Файловая система</label>
                        <Selector
                            values={this.props.fileSystemList}
                            selected={this.state.filesystem}
                            onChange={this.onChange.bind(this, "filesystem")}
                        />
                     </div>
                <div>
                    <Button
                        text="Поехали!"
                        primary={true}
                        onClick={this.onStart.bind(this)} />
                </div>
                <hr/>
                <div>
                    <Button
                        text="Просто увезите отсюда!"
                        primary={true}
                        onClick={this.onDefaultMigrationStart.bind(this)} />
                </div>
            </form>
        </div>;
    }
}
