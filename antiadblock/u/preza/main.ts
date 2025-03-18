import pptxgen from "pptxgenjs";

import { checkToken, getWeek, getYear } from "./utils/utils";
import {createMainSlide} from "./slides/main";
import {createSlide as createZFPSlide} from "./slides/zfp";
import {createSlides as createGoalsSlides} from "./slides/goals";
import {createSlide as createDetectSlide} from "./slides/detect";
import {createSlide as createHeatmapSlide} from "./slides/heatmap";
import {createSlide as createSupportSlide} from "./slides/support";
import {createDutySlide, createNoGoalsSlides} from "./slides/noGoals";
import {createSlideMoneyDesktop, createSlideMoneyMobile, createSlideMoneySdkAndroid} from "./slides/money";

const slidesGenerators = [createMainSlide, createSupportSlide, createZFPSlide, createSlideMoneyDesktop, createSlideMoneyMobile, createSlideMoneySdkAndroid, createDutySlide, createHeatmapSlide, createDetectSlide, createGoalsSlides, createNoGoalsSlides];
const presentation: pptxgen = new pptxgen();


async function makePrezaGreatAgain() {
    let week = getWeek()
    let week_start = week.start.format('DD.MM');
    let week_end = week.end.format('DD.MM');
    const generators = slidesGenerators.map(slide => {
        const generator = slide();
        const res = generator.next();
        return {
            generator,
            res
        };
    });

    for (let i = 0; i < generators.length; i++) {
        const element = generators[i];
        const { generator } = element;

        let res = await element.res;
        while (!res.done) {
            res = await generator.next(presentation.addSlide());
        }
    }

    presentation.writeFile({fileName: `Итоги_${week_start}-${week_end}_${getYear()}.pptx`});
}

for (let token of ["STAT_TOKEN", "STARTREK_TOKEN", "TOOLS_TOKEN"]) {
  checkToken(token);
}

makePrezaGreatAgain();
