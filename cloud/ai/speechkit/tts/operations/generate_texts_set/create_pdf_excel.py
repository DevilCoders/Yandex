from fpdf import FPDF
import pandas as pd

import nirvana.job_context as nv


class FPDF_bold_multicell(FPDF):
    def write_multicell_with_styles(self, max_width, cell_height, text_list):
        start_width = self.get_string_width('0000. ')
        startx = self.get_x()
        current_pos = startx
        startx = startx + start_width
        self.set_font('basic', '', 14)

        # loop through differenct sections in different styles
        for text_part in text_list:
            self.set_x(current_pos)
            # check and set style
            if 'style' in text_part:
                current_style = text_part['style']
                self.set_font('basic', current_style, 14)
            else:
                self.set_font('basic', '', 14)

            # loop through words and write them down
            space_width = self.get_string_width(' ')
            values = text_part['text'].split(' ')
            for i, word in enumerate(values):
                if i < len(values) - 1:
                    word = word + ' '
                current_pos = self.get_x()
                word_width = self.get_string_width(word)
                current_pos = current_pos + word_width
                # check for newline
                if (current_pos + word_width) > (startx + max_width):
                    # return
                    self.set_y(self.get_y() + cell_height)
                    self.set_x(startx)
                    current_pos = self.get_x()
                self.cell(word_width, 4, word)
                # add a space
                # self.set_x(self.get_x() + space_width)


def create_pdf(texts, output_path):
    pdf = FPDF_bold_multicell()
    pdf.add_page()
    pdf.add_font('basic', '', './font/Literation Mono Powerline.ttf', uni=True)
    pdf.add_font('basic', 'B', './font/Literation Mono Powerline Bold.ttf', uni=True)
    pdf.set_font('basic', '', 14)

    for counter, x in enumerate(texts):
        text_id = str(counter + 1).zfill(4) + '. '
        text_list = [{'text': text_id}]
        if '*' in x:
            values = x.split('**')
            for i, value in enumerate(values):
                if i % 3 == 1:
                    text_list.append({'style': 'B', 'text': value})
                else:
                    text_list.append({'text': value})
        else:
            text_list.append({'text': x})

        joined_text_list = [text_list[0]]
        for i in range(1, len(text_list)):
            if 'style' not in joined_text_list[-1] and 'style' not in text_list[i]:
                joined_text_list[-1] = {'text': joined_text_list[-1]['text'] + text_list[i]['text']}
            else:
                joined_text_list.append(text_list[i])

        pdf.write_multicell_with_styles(180, 10, joined_text_list)
        pdf.ln(10)

    pdf.output(output_path)


def create_excel(sample_ids, texts, output_path):
    lines = []
    for counter, text in enumerate(texts):
        lines.append((str(counter + 1).zfill(4) + '.wav', text, sample_ids[counter]))
    df = pd.DataFrame(lines, columns=['audio_name', 'text', 'sample_id'])
    df.to_excel(output_path, index=False)


if __name__ == '__main__':
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()

    ids, texts = [], []
    with open(inputs.get('texts')) as f:
        for line in f:
            text_id, text = line.strip().split('\t')
            ids.append(text_id)
            texts.append(text)

    create_excel(ids, texts, outputs.get('text_audio_match'))
    create_pdf(texts, outputs.get('texts'))
