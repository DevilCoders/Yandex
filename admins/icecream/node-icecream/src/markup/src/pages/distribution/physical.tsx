import { ProgressBar } from "../../common/components/progressbar";
import { PhysicalHostInfo, ContainerInfo } from "../../common";
import { Container } from "./container";

interface PhysicalHostProps extends PhysicalHostInfo {
    filter: string;
    projectFilter: string;
    pushState: (url: string) => void;
}

export class PhysicalHost extends React.Component<PhysicalHostProps, {}> {
    static hasVisibleContainers(conts: ContainerInfo[],
                                filter: string,
                                projectFilter: string): boolean {
        for (let i in conts) {
            if (PhysicalHost.isVisibleContainer(conts[i],
                                                filter,
                                                projectFilter))
                                                return true;
        }
        return false;
    }

    static isVisibleContainer(cont: ContainerInfo,
                              filter: string,
                              projectFilter: string) {
        return cont.fqdn.search(filter) > -1
               && (projectFilter === ""
                  || cont.project === projectFilter);
    }

    goto(e: MouseEvent) {
        if (!e.ctrlKey && !e.shiftKey) {
            this.props.pushState(`/host/${this.props.fqdn}`);
        }
    }


    render(): JSX.Element {
        let containers: JSX.Element[] = [];
        let cpu = 0;
        let ram = 0;
        let disk = 0;
        for (let i in this.props.containers) {
            if (PhysicalHost.isVisibleContainer(this.props.containers[i],
                                                this.props.filter,
                                                this.props.projectFilter)) {
                containers.push(<Container key={i}
                                            {...this.props.containers[i]}/>);
            }
            cpu += this.props.containers[i].cpu;
            ram += this.props.containers[i].ram;
            disk += this.props.containers[i].diskSize;
        }

        if (this.props.filter !== "" && containers.length === 0) {
            return null;
        }

        return <tbody>
            <tr className="physical">
                <th><a href={`/host/${this.props.fqdn}`}
                            onClick={this.goto.bind(this)}>
                { this.props.fqdn }</a>
                </th>
                <td className="equicol">
                    { cpu } / { this.props.cpu }
                    <ProgressBar values={[ cpu / this.props.cpu * 100]}/>
                </td>
                <td className="equicol">
                    { (ram / 1073741824).toFixed(1) }
                       / { (this.props.ram / 1073741824).toFixed(1) }
                    <ProgressBar values={[ ram / this.props.ram * 100]}/>
                </td>
                <td className="equicol">
                    { (disk / 1073741824).toFixed(1) }
                       / { (this.props.diskSize / 1073741824).toFixed(1) }
                    <ProgressBar values={[ disk / this.props.diskSize * 100]}/>
                </td>
                <td>
                { this.props.pool }
                </td>
            </tr>
            { containers }
        </tbody>;
    }
}

