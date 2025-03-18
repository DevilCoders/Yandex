import pptxgen from "pptxgenjs";

import {getWeek} from "../utils/utils";

let week = getWeek()
let week_start = week.start.format('DD.MM');
let week_end = week.end.format('DD.MM');

export async function* createMainSlide() {
    const slide: pptxgen.Slide = yield;
    slide.addText(`ИТОГИ НЕДЕЛИ\n${week_start} - ${week_end}`, {align: 'center', bold: true, fontSize: 28, x: 1, y: 1});
}