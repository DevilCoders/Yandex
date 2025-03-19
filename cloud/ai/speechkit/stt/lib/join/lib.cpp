//
// Created by Mikhail Yutman on 30.03.2020.
//

#include "lib.h"

TVector<TRecordBitsMarkups> ReadRecordsBitsMarkupsFromTxt(TStringBuf filename, int offset) {
    TIFStream inp((TString)filename);

    TVector<TRecordBitsMarkups> recordsBitsMarkups;
    TString line;
    while (inp.ReadLine(line)) {
        int n;
        TString id;
        TStringStream sstr;
        sstr << line;

        sstr >> n >> id;
        int from;
        int last = -offset;

        TVector<TText> cur;
        TRecordBitsMarkups recordBitsMarkups(id);
        for (int i = 0; i < n; i++) {
            inp >> from;
            if (from != last) {
                if (last != -offset) {
                    recordBitsMarkups.BitsMarkups.push_back(cur);
                    cur.clear();
                }
                while (last + offset < from) {
                    recordBitsMarkups.BitsMarkups.push_back({});
                    last += offset;
                }
            }
            last = from;

            TString s = inp.ReadLine();
            TVector<TString> vars;
            for (auto& _token : StringSplitter(s).Split(' ')) {
                TString token = TString(_token);
                if (token != "-") {
                    vars.push_back(token);
                }
            }
            cur.push_back(TText(vars, from));
        }
        recordBitsMarkups.BitsMarkups.push_back(cur);

        recordsBitsMarkups.push_back(recordBitsMarkups);
    }
    return recordsBitsMarkups;
}

TVector<TRecordBitsMarkups> ReadRecordsBitsMarkupsFromJson(TStringBuf filename, int offset) {
    TIFStream inp((TString)filename);
    NJson::TJsonValue recordsBitsMarkupsJson;

    NJson::ReadJsonTree(&inp, &recordsBitsMarkupsJson);

    TVector<TRecordBitsMarkups> recordsBitsMarkups;
    TString line;

    for (auto recordBitsMarkupsJson : recordsBitsMarkupsJson.GetArray()) {
        long long int from;
        long long int last = -offset;

        TString id = recordBitsMarkupsJson["id"].GetStringRobust();
        TString s3Key = recordBitsMarkupsJson["s3_key"].GetStringRobust();

        if (id == "031109b1-d4b3-4c2e-a4c8-a4d566bbd4fe") {
            Cerr << "kek" << Endl;
        }

        TVector<TText> cur;
        TRecordBitsMarkups recordBitsMarkups(id, s3Key);

        for (auto recordBitMarkupJson : recordBitsMarkupsJson["bit_info"].GetArray()) {
            if (!recordBitMarkupJson[1]["words"][0].GetMap().contains("phones")) {
                continue;
            }
            from = recordBitMarkupJson[0]["start_ms"].GetInteger();

            if (from != last) {
                if (last != -offset) {
                    recordBitsMarkups.BitsMarkups.push_back(cur);
                    cur.clear();
                }
                while (last + offset < from) {
                    recordBitsMarkups.BitsMarkups.push_back({});
                    last += offset;
                }
            }
            last = from;

            TVector<TString> vars;
            TVector<ui32> splits;
            TVector<ui32> onsets;
            TVector<ui32> letterOnsets;

            ui32 curIndex = 0;
            int nWord = 0;
            ui32 end = 0;
            Cerr << "token sizes: ";
            for (auto word : recordBitMarkupJson[1]["words"].GetArray()) {
                TString token = word["text"].GetStringRobust();
                if (token == "yandexttsspecialpauseword" || token == "-") {
                    continue;
                }

                Cerr << " " << token.size();
                if (word.GetMap().contains("phones")) {
                    ui32 onset = word["phones"][0]["onset"].GetInteger();
                    ui32 count = 0;
                    for (NJson::TJsonValue letter : word["phones"].GetArray()) {
                        end = letter["onset"].GetInteger() + letter["duration"].GetInteger();
                        if (count + 2 > token.size() + 1) {
                            break;
                        }
                        letterOnsets.push_back(letter["onset"].GetInteger());
                        letterOnsets.push_back(letter["onset"].GetInteger());
                        count += 2;
                    }
                    while (count < token.size() + 1) {
                        count++;
                        letterOnsets.push_back(end);
                    }

                    ui32 chunkIndex = onset / offset;

                    onsets.push_back(onset);
                    while (curIndex < chunkIndex) {
                        splits.push_back(nWord);
                        curIndex++;
                    }
                } else {
                    ui32 count = 0;
                    while (count < token.size() + 1) {
                        count++;
                        letterOnsets.push_back(end);
                    }
                }

                vars.push_back(token);
                nWord++;
            }

            //TODO: constant value in code, remove
            while (curIndex < 3) {
                splits.push_back(nWord);
                curIndex++;
            }

            if (!letterOnsets.empty()) {
                letterOnsets.pop_back();
            }
            TText curText(vars, from, splits, onsets, letterOnsets);

            Cerr << Endl << "union size: " << curText.JoinWithWhitespaces().size() << " " << id << Endl;

            if (curText.JoinWithWhitespaces().size() != curText.LetterOnsets.size()) {
                Cerr << "Sizes are not equal, str=" << curText.JoinWithWhitespaces().size() << ", onsets=" << curText.LetterOnsets.size() << Endl;
            }

            cur.push_back(curText);
        }
        recordBitsMarkups.BitsMarkups.push_back(cur);

        recordsBitsMarkups.push_back(recordBitsMarkups);
    }
    return recordsBitsMarkups;
}

void OutputRecordsMarkupsToOutputStream(TConstArrayRef<TRecordJoin> recordsMarkups, IOutputStream& stream) {
    TVector<TString> recordMarkup;
    TString recordId;
    for (auto& recordJoin : recordsMarkups) {
        stream << recordJoin.RecordId << " " << recordJoin.JoinedText << Endl;
    }
}

void OutputRecordsMarkupsAsJson(TConstArrayRef<TRecordJoin> recordsMarkups, IOutputStream& stream, const TString& tag, const TString& idTag) {
    Y_UNUSED(tag);
    NJson::TJsonMap recordsMarkupsJson;
    for (auto& recordJoin : recordsMarkups) {
        recordsMarkupsJson[recordJoin.RecordId + "_" + idTag] = recordJoin.JoinedText.JoinWithWhitespaces();
    }
    NJson::WriteJson(&stream, &recordsMarkupsJson, true, true);
}
