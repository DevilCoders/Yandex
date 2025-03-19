function hsb2rgb(hue: number, saturation: number, value: number) {
    // hue: 0..360
    // saturation: 0..1
    // value: 0..1

    hue = hue % 360;

    let rgb: number[];
    if (saturation === 0) {
        return [
            Math.round(255 * value),
            Math.round(255 * value),
            Math.round(255 * value)
        ];
    }

    let side = hue / 60;
    let chroma = value * saturation;
    let x = chroma * (1 - Math.abs(side % 2 - 1));
    let match = value - chroma;

    switch (Math.floor(side)) {
        case 0: rgb = [ chroma, x, 0 ]; break;
        case 1: rgb = [ x, chroma * 0.7, 0 ]; break;
        case 2: rgb = [ 0, chroma * 0.7, x ]; break;
        case 3: rgb = [ 0, x * 0.7, chroma ]; break;
        case 4: rgb = [ x, 0, chroma ]; break;
        case 5: rgb = [ chroma, 0, x ]; break;
        default: rgb = [ 0, 0, 0 ];
    }

    rgb[0] = Math.round(255 * (rgb[0] + match));
    rgb[1] = Math.round(255 * (rgb[1] + match));
    rgb[2] = Math.round(255 * (rgb[2] + match));

    return rgb;
}

function hsb2color(h: number, s: number, b: number) {
    let rgb = hsb2rgb(h, s, b);
    return `#${rgb[0].toString(16)}${rgb[1].toString(16)}${rgb[2].toString(16)}`;
}

export function generateColors(count: number): string[] {
    let result: string[] = [];
    let angle = 360 / count;
    let hue = 347;
    for (let i = 0; i < count; i ++) {
        result.push(hsb2color(hue, 0.6, 1));
        hue += angle;
    }
    return result;
}
