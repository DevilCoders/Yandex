interface Props {
    info: DcRowInfo;
    location: string;
}

export interface DcRowInfo {
    total: number;
    free: number;
    hdd_free: number;
    hdd_used: number;
    ssd_free: number;
    ssd_used: number;
}

export class DcRow extends React.Component<Props, {}> {
    render() {
        return <tr>
            <th>{this.props.location}</th>
            <td className="equicol">
                {this.props.info.total}
            </td>
            <td className="equicol">
                {this.props.info.free}
            </td>
            <td className="equicol">
                {this.props.info.hdd_used}
            </td>
            <td className="equicol">
                {this.props.info.hdd_free}
            </td>
            <td className="equicol">
                {this.props.info.ssd_used}
            </td>
            <td className="equicol">
                {this.props.info.ssd_free}
            </td>
        </tr>;
    }
}
