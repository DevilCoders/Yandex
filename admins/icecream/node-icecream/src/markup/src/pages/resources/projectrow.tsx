import { API, ContainerInfo } from "../../common";
import { getUrl, stdCatch } from "../../common/ajax";

export interface ProjectResources {
    cpu: number;
    ram: number;
    disk: number;
}

export interface ProjectInfo extends ProjectResources {
    name: string;
}

interface Props {
    info: ProjectInfo;
    totalProjects: ProjectResources;
    totalPhysical: ProjectResources;
}

export class ProjectRow extends React.Component<Props, {}> {
    renderTotal() {
        return <tr>
            <th>
                {this.props.info.name}
            </th>
            <th className="equicol">
                {this.props.info.cpu}
            </th>
            <th className="equicol">
                {(this.props.info.ram / 1073741824 ).toFixed(1)}
            </th>
            <th className="equicol">
                {(this.props.info.disk / 1073741824 ).toFixed(1)}
            </th>
            <th className="equicol">
            </th>
        </tr>;
    }

    mkPerc(total: ProjectResources) {
        return (  this.props.info.cpu / total.cpu
                + this.props.info.ram / total.ram
               ) * 50;
    }

    render() {
        let percProjects = this.mkPerc(this.props.totalProjects);
        let percPhysical = this.mkPerc(this.props.totalPhysical);

        return <tr>
            <th>{this.props.info.name}</th>
            <td className="equicol">
                {this.props.info.cpu}
            </td>
            <td className="equicol">
                {(this.props.info.ram / 1073741824 ).toFixed(1)}
            </td>
            <td className="equicol">
                {(this.props.info.disk / 1073741824 ).toFixed(1)}
            </td>
            <td className="equicol">
                { percProjects.toFixed(1) }
            </td>
            <td className="equicol">
                { percPhysical.toFixed(1) }
            </td>
        </tr>;
    }
}
