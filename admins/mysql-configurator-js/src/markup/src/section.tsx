import { Graph } from "./graph";
import { CheckBox } from "./common/components/checkbox";

export interface Status {
    [varName: string]: number[];
}

interface SectionProps {
    name: string;
    values: Status;
    timestamps: number[];
    scale: number;
    collapsed: boolean;
}

interface SectionState {
    collapsed: boolean;
}

export class Section extends React.Component<SectionProps, SectionState> {
    static displayName = "Section";

    constructor() {
        super({} as SectionProps);
        this.state = {
            collapsed: true,
        };
    }

    componentDidMount() {
        this.setState({collapsed: this.props.collapsed});
    }

    onChange(val: boolean) {
        this.setState({ collapsed: !val });
    }

    render(): JSX.Element {
        let items: JSX.Element[] = [];

        let graphNames = Object.keys(this.props.values);
        graphNames.sort();

        for (let name of graphNames) {
            items.push(<Graph
                            key={name}
                            name={name}
                            values={this.props.values[name]}
                            timestamps={this.props.timestamps}
                            scale={this.props.scale}
                            />);
        }

        let graphs: JSX.Element = null;
        if (!this.state.collapsed) {
            graphs = <div className="graphs">
                { items }
            </div>;
        }

        return <div>
            <div className="toggler">
                <CheckBox value={!this.state.collapsed} onChange={this.onChange.bind(this)}/>
                <h2>{ this.props.name }</h2>
            </div>
            { graphs }
        </div>;
    }
}
