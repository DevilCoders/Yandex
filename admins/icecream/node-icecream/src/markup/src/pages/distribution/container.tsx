import { ContainerInfo } from "../../common";

interface ContainerProps extends ContainerInfo {
}

export class Container extends React.Component<ContainerProps, {}> {
    render(): JSX.Element {
        let status = <i  className="fa pad container-down fa-arrow-circle-down" />;
        let ram = (this.props.ram / 1073741824).toFixed(1);
        let diskSize = (this.props.diskSize / 1073741824).toFixed(1);
        if (this.props.status === "Running") {
            status = <i className="fa pad container-up fa-arrow-circle-up" />;
        }

        return <tr>
            <th> {this.props.fqdn} {status} </th>
            <td className="equicol">{this.props.cpu}</td>
            <td className="equicol">{ram}</td>
            <td className="equicol">{diskSize}</td>
            <td>{this.props.project}</td>
        </tr>;
    }
}


