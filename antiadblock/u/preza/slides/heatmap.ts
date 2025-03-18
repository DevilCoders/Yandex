import pptxgen from "pptxgenjs";
import { makeCall } from "../utils/utils";
import { TITLE_FONT_SIZE, INNER_PARTNERS, OUTER_PARTNERS } from '../const';

const CHART_ID = 'u323wd2ag5e4q';
const RELATIVE = 30; // days
const IMAGE_WIDTH = 1440;
const IMAGE_HEIGHT = 720;
const FONT_SIZE = 9;
const CELL_FONT_SIZE = 9;

const SLIDES_ARR = [{
    title: 'Desktop Inner HeatMap',
    device: 'desktop',
    partners: INNER_PARTNERS,
    image: ''
}, {
    title: 'Desktop Outer HeatMap',
    device: 'desktop',
    partners: OUTER_PARTNERS,
    image: ''
}, {
    title: 'Mobile Inner HeatMap',
    device: 'mobile',
    partners: INNER_PARTNERS,
    image: ''
}, {
    title: 'Mobile Outer HeatMap',
    device: 'mobile',
    partners: OUTER_PARTNERS,
    image: ''
}];

async function requestImage(device: string, domains: string[]) {
    console.log('FETCHING IMAGE....');
    const result = await makeCall(`https://charts.yandex-team.ru/api/scr/v1/screenshots/preview/editor/${CHART_ID}?scaleValue=d_by_i_sum&scale=d_by_i_sum&deviceValue=desktop&device=${device}&weekendValue=1&hide_holidays=1&rangeDatepickerTo=__relative_-0d&date_max=__relative_-0d&rangeDatepickerFrom=__relative_-${RELATIVE}d&date_min=__relative_-${RELATIVE}d&_no_controls=true&__scr_width=${IMAGE_WIDTH}&__scr_height=${IMAGE_HEIGHT}&__scr_filename=preza_chart&_embedded=1&__scr_download=1&fontSize=${FONT_SIZE}&cellFontSize=${CELL_FONT_SIZE}&domains=${domains.reduce((acc, domain) => `${acc}&domains=${domain}`, '')}`);
    console.log(`IMAGE SIZE (base64): ${result.length}`);
    return `data:image/png;base64,${result}`;
}

function makeSlide(slide: pptxgen.Slide, title: string, image: string) {
    slide.addText(title, { x: 0.5, y: 0.5, fontSize: TITLE_FONT_SIZE, color: '363636', fill: { color:'F1F1F1' } });
    slide.addImage({x:0.1, y:0.8, w:9.0, h:4.5, data: image});
}

export async function* createSlide() {
    console.log('/--------------------------------------------/');
    console.log('START HEATMAP');

    for (let i = 0; i < SLIDES_ARR.length; i++) {
        const slideInfo = SLIDES_ARR[i];
        SLIDES_ARR[i].image = await requestImage(slideInfo.device, slideInfo.partners);
    }

    for (let i = 0; i < SLIDES_ARR.length; i++) {
        const slide: pptxgen.Slide = yield;
        const slideInfo = SLIDES_ARR[i];
        makeSlide(slide, slideInfo.title, slideInfo.image);
    }

    console.log('/--------------------------------------------/');
}
