import { ProgressBar } from "../common/components/progressbar";
import { CheckBox } from "../common/components/checkbox";
import { EditBox } from "../common/components/editbox";
import { Button } from "../common/components/button";
import { Selector } from "../common/components/selector";
import { Slider } from "../common/components/slider";

interface DemoState {
    edText: string;
    cBox: boolean;
    dc: number;
    cpu: number;
}
export class Demo extends React.Component<{}, DemoState> {

    constructor() {
        super();
        this.state = {
            edText: "foo",
            cBox: false,
            dc: 0,
            cpu: 20,
        };
    }

    onChange(field: string, value: any) {
        let x = {} as any;
        x[field] = value;
        this.setState(x as DemoState);
    }

    render(): JSX.Element {
        return <div>

            <h1>Заголовок первого уровня</h1>
            <h2>Заголовок второго уровня</h2>
            <h3>Заголовок третьего уровня</h3>

            <p>
            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas
            sagittis vel tellus at aliquet. Nunc condimentum rhoncus scelerisque.
            Ut at fermentum neque, sed iaculis urna. Praesent ligula lorem,
            mollis non ante ut, ultrices posuere quam. Nullam egestas mattis
            velit, in semper risus egestas rhoncus. In ipsum justo, mollis sed
            diam fringilla, tristique gravida est. Nullam sit amet ligula elementum,
            egestas justo id, lacinia nisi. Duis auctor malesuada dolor sed
            placerat. Sed sit amet pretium elit. Donec egestas neque non
            lobortis scelerisque. Nam sapien quam, blandit a mi et, auctor
            viverra est. In mauris felis, ultricies ac porta a, viverra vitae
            metus. Donec eget mauris id metus lobortis sagittis id ac ipsum.
            Nunc sit amet aliquet dolor.
            </p>

            <EditBox
                value={this.state.edText}
                onChange={this.onChange.bind(this, "edText")}
                placeholder="foo"
                />
            <Button text="Привет, мир" primary={true} onClick={() => {}} />
            <CheckBox
                value={this.state.cBox}
                onChange={this.onChange.bind(this, "cBox")}
                />


            <br/>

            <ProgressBar values={[75]}/>
            <ProgressBar values={[95]} animated={true}/>

            <table>
                <tbody>
                    <tr>
                        <th>Столбец</th>
                        <th>Столбец</th>
                        <th>Столбец</th>
                        <th>Столбец</th>
                    </tr>
                    <tr>
                        <td>Столбец</td>
                        <td>Столбец</td>
                        <td>Столбец</td>
                        <td>Столбец</td>
                    </tr>
                    <tr>
                        <td>Столбец</td>
                        <td>Столбец</td>
                        <td>Столбец</td>
                        <td>Столбец</td>
                    </tr>
                    <tr>
                        <td>Столбец</td>
                        <td>Столбец</td>
                        <td>Столбец</td>
                        <td>Столбец</td>
                    </tr>
                </tbody>
            </table>

            <form>
                <div>
                    <label>Датацентр</label>
                    <Selector
                        values={["myt", "sas", "fol"]}
                        selected={this.state.dc}
                        onChange={this.onChange.bind(this, "dc")}
                        />
                </div>
                <div>
                    <label>Ядер</label>
                    <Slider min={0} max={32} value={this.state.cpu}
                        onChange={this.onChange.bind(this, "cpu")} />
                </div>
            </form>

        </div>;
    }
}
