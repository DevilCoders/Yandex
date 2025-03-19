import { getUrl, stdCatch } from "../../common/ajax";
import { EditBox } from "../../common/components/editbox";
import { Selector } from "../../common/components/selector";
import { Slider } from "../../common/components/slider";
import { UpdateHost } from "../../components/updatehost";
import { API, ContainerInfo } from "../../common";
import { ColorShower } from "../../common/components/colorshower";
import { PopupButton, popupClose } from "../../common/components/popup";
import { Button } from "../../common/components/button";

interface ContainerRowProps {
    info: ContainerInfo;
    color: string;
    onPausedContainer: (name: string) => void;
    onRunContainer: (name: string) => void;
    onUpdateContainer: (info: ContainerInfo, diskSize: number, mem: number, cpu: number) => void;
    onRestartedContainer: (name: string) => void;
    onRemovedContainer: (name: string) => void;
    onMigrateContainer: (name: string, filesystem: string) => void;
}

export class ContainerRow extends React.Component<ContainerRowProps, {}> {
    onPauseContainer() {
        getUrl(`${API}/container/${this.props.info.fqdn}`, "PUT", "{\"status\": \"stop\"}")
        .then(function(){
            this.props.onPausedContainer(this.props.info.fqdn);
        }.bind(this))
        .catch(stdCatch);
    }

    onRunContainer() {
        getUrl(`${API}/container/${this.props.info.fqdn}`, "PUT", "{\"status\": \"start\"}")
        .then(function(){
            this.props.onRunContainer(this.props.info.fqdn);
        }.bind(this))
        .catch(stdCatch);
    }

    onUpdateContainer() {
        this.props.onUpdateContainer(this.props.info,
                                        this.props.info.diskSize,
                                        this.props.info.diskSize,
                                        this.props.info.diskSize);
    }

    onRestartContainer() {
        getUrl(`${API}/container/${this.props.info.fqdn}`, "PUT", "{\"status\": \"restart\"}")
        .then(function(){
            this.props.onRestartedContainer(this.props.info.fqdn);
        }.bind(this))
        .catch(stdCatch);
    }

    onRemoveContainer() {
        getUrl(`${API}/container/${this.props.info.fqdn}`, "DELETE")
        .then(function(){
            this.props.onRemovedContainer(this.props.info.fqdn);
        }.bind(this))
        .catch(stdCatch);
    }

    onMigrateContainer() {
        this.props.onMigrateContainer(this.props.info.fqdn, this.props.info.filesystem);
    }

    render(): JSX.Element {
        let status = <i  className="fa pad container-up fa-arrow-circle-up" />;
        let startStop: JSX.Element = <PopupButton icon="pause-circle">
            <b>Вы уверены, что хотите остановить этот контейнер?</b>
            <span>
                <Button
                    text="Да, остановить!"
                    onClick={ this.onPauseContainer.bind(this) }
                    primary={true}/>
                <Button text="Нет, не надо" onClick={popupClose}/>
            </span>
        </PopupButton>;

        if (this.props.info.status === "Stopped") {
            startStop = <PopupButton icon="play-circle">
                <b>Вы уверены, что хотите запустить этот контейнер?</b>
                <span>
                    <Button
                        text="Да, запустить!"
                        onClick={ this.onRunContainer.bind(this) }
                        primary={true}/>
                    <Button text="Нет, не надо" onClick={popupClose}/>
                </span>
            </PopupButton>;
            status = <i  className="fa pad container-down fa-arrow-circle-down" />;
        }

        if (this.props.info.status === "Migrating") {
            status = <i className="fa pad container-migrating fa-location-arrow"/>;
            return <tr>
                <th>{ this.props.info.fqdn } { status }</th>
                <td colSpan={4}/>
                <th/>
            </tr>;
        }

        return <tr>
            <th>{ this.props.info.fqdn } { status }</th>
            <td>{ (this.props.info.ram / 1073741824).toFixed(1) }</td>
            <td>{ this.props.info.cpu }</td>
            <td>{ (this.props.info.diskSize / 1073741824).toFixed(1) }</td>
            <td>{ this.props.info.filesystem} </td>
            <th>
                <ColorShower color={this.props.color}/>
                {startStop}
                <PopupButton icon="refresh">
                    <b>Вы уверены, что хотите перезапустить этот контейнер?</b>
                    <span>
                        <Button
                            text="Да, перезапустить!"
                            onClick={ this.onRestartContainer.bind(this) }
                            primary={true}/>
                        <Button text="Нет, не надо" onClick={popupClose}/>
                    </span>
                </PopupButton>
                <i className={"fa fa-fw fa fa-cog"} onClick={ this.onUpdateContainer.bind(this) }/>
                <i className={"fa fa-fw fa fa-location-arrow"} onClick={ this.onMigrateContainer.bind(this) }/>
                <PopupButton icon="trash pad">
                    <b>Вы уверены, что хотите удалить этот контейнер?</b>
                    <span>
                        <Button
                            text="Да, удалить!"
                            onClick={ this.onRemoveContainer.bind(this) }
                            primary={true}/>
                        <Button text="Нет, пусть живет" onClick={popupClose}/>
                    </span>
                </PopupButton>
            </th>
        </tr>;
    }
}

