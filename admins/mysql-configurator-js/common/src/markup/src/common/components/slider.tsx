interface Props {
    min?: number;
    max?: number;
    points?: number[];
    value: number;
    onChange: (value: number) => void;
}
export class Slider extends React.Component<Props, {}> {
    bar: HTMLElement;
    pts: number[];

    onMouseMove(e: React.MouseEvent<HTMLElement>) {
        // calculate the position
        let frac = (e.clientX - this.bar.offsetLeft + this.bar.offsetParent.scrollLeft) / this.bar.offsetWidth;
        if (frac < 0) frac = 0;
        if (frac > 1) frac = 1;

        let idx = Math.round((this.pts.length - 1) * frac);
        this.props.onChange(this.pts[idx]);
    }

    onMouseDown(e: React.MouseEvent<HTMLElement>) {
        e.preventDefault();
        document.onmousemove = this.onMouseMove.bind(this);
        document.onmouseup = this.onMouseUp.bind(this);
        this.onMouseMove(e);
    }

    onMouseUp(e: React.MouseEvent<HTMLElement>) {
        e.preventDefault();
        document.onmousemove = null;
        document.onmouseup = null;
    }

    onDragStart() {
        return false;
    }

    setBar(bar: HTMLElement) {
        this.bar = bar;
    }

    genPoints() {
        // Generates equally spaced points
        let pts: number[] = [];
        for (let i = this.props.min; i <= this.props.max; i++) {
            pts.push(i);
        }
        return pts;
    }

    render(): JSX.Element {
        if (!this.pts) {
            this.pts = this.props.points || this.genPoints();
        }

        let pos: number = 0;
        for (let i = 0; i < this.pts.length; i++) {
            if (this.pts[i] >= this.props.value) {
                pos = i;
                break;
            }
        }
        let style = {left: `${pos / (this.pts.length - 1) * 100}%`};

        return <div className="slider">
            <div ref={this.setBar.bind(this)}
                onMouseDown={this.onMouseDown.bind(this)}
                onDragStart={this.onDragStart}
                >
                <div
                    style={style}/>
            </div>
            <div>{ this.props.value }</div>
        </div>;
    }
}
