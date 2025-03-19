import { Section, Status } from "./section";
import { Selector } from "./common/components/selector";

interface StringString {
    [name: string]: string;
}

interface SectionedStatus {
    [sectionName: string]: Status;
}

interface PageState {
    timestamps: number[];
    status: SectionedStatus;
    scale: number;
}

interface SocketDiff {
    [name: string]: number;
}

export class GraphsPage extends React.Component<{}, PageState> {
    static displayName = "Page";

    prefixMap: StringString = {
        "InnoDB": "Innodb_",
        "MyISAM": "Key_",
        "Handler": "Handler_",
        "Query cache": "Qcache_",
        "Commands": "Com_",
        "Generic": "",
    };

    constructor() {
        super({});
        this.state = {
            timestamps: [],
            status: {},
            scale: 1,
        };
    }

    findSection(valName: string): string {
        for (let prefix in this.prefixMap) {
            if (valName.search(this.prefixMap[prefix]) === 0) {
                return prefix;
            }
        }
    }

    componentDidMount() {
        fetch("/data")
        .then(function(data: Response){
            return data.json();
        })
        .then(function(data: Status){
            let status: SectionedStatus = {};
            for (let prefix in this.prefixMap) {
                status[prefix] = {};
            }
            for (let val in data) {
                if (val === "timestamp") continue;
                let prefix = this.findSection(val);
                status[prefix][val] = data[val];
            }
            this.setState({
                timestamps: data.timestamp,
                status: status,
            });
        }.bind(this))
        .catch(function(err: Error){
            console.log(err);
        });

        let socket = io.connect(undefined);
        socket.on("data", function(data: SocketDiff) {
            this.state.timestamps.push(data.timestamp);
            if (this.state.timestamps.length > 300) this.state.timestamps.shift();

            for (let val in data) {
                if (val === "timestamp") continue;
                let prefix = this.findSection(val);
                if (this.state.status[prefix][val]) {
                    this.state.status[prefix][val].push(data[val]);
                    if (this.state.status[prefix][val].length > 300)
                        this.state.status[prefix][val].shift();
                }
            }

            this.forceUpdate();
        }.bind(this));
    }

    setScale(scale: number) {
        this.setState({ scale: scale });
    }

    render(): JSX.Element {
        let sections: JSX.Element[] = [];
        let scales = [1, 5, 10, 30];
        let scaleTexts = [];
        for (let i in scales) {
            scaleTexts.push(`${scales[i]} мин`);
        }

        // Need to sort so that generic is in the beginning
        let statusNames = Object.keys(this.state.status);
        statusNames.unshift("Generic");
        statusNames.pop();

        let collapsed = false;
        for (let section of statusNames) {
            sections.push(<Section
                                key={section}
                                name={section}
                                values={this.state.status[section]}
                                timestamps={this.state.timestamps}
                                scale={scales[this.state.scale]}
                                collapsed={collapsed}
                                />);
            collapsed = true;
        }

        return <div>
            <Selector values={scaleTexts}
                      selected={this.state.scale}
                      onChange={this.setScale.bind(this)}/>
            { sections }
        </div>;
    }
}
