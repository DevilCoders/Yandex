package ru.yandex.ci.core.arc;

import java.io.IOException;

import com.google.gson.Gson;
import com.google.gson.TypeAdapter;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;


/**
 * Сериализует ArcBranch в строчку и из нет. Безопасно заменять String на ArcBranch в сериализуемых объектах.
 */
public class ArcBranchAdapter extends TypeAdapter<ArcBranch> {

    @Override
    public void write(JsonWriter out, ArcBranch value) throws IOException {
        out.value(value.asString());
    }

    @Override
    public ArcBranch read(JsonReader in) throws IOException {
        if (in.peek() == JsonToken.BEGIN_OBJECT) {
            // распакованный бранч
            return new Gson().fromJson(in, ArcBranch.class);
        }
        return ArcBranch.ofString(in.nextString());
    }
}
