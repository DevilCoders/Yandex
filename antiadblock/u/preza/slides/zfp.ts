import pptxgen from "pptxgenjs";
import {makeCall} from "../utils/utils";
import { TITLE_FONT_SIZE } from '../const';


export async function* createSlide() {
    console.log('/--------------------------------------------/');
    console.log('START ZFP SLIDE');
    let download_link = 'https://charts.yandex-team.ru/api/scr/v1/screenshots/preview/editor/uzinyk1gt58ap?_no_controls=true&__scr_width=1600&__scr_height=720&_embedded=1';
    let image: string = await makeCall(download_link);

    const slide: pptxgen.Slide = yield;
    slide.addImage({x: 0.0, y: 0.0, w: 10, h: 5.63, data: `data:image/png;base64,${image}`});
    let textboxText = "Дежурство (алерты)";
    let textboxOpts = { x: 0.5, y: 0.3, fontSize: TITLE_FONT_SIZE, color: '363636', fill: { color:'F1F1F1' } };    
    slide.addText(textboxText, textboxOpts);
    console.log('/--------------------------------------------/');
    return slide;
}
