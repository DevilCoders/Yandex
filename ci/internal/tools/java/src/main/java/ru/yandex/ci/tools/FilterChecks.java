package ru.yandex.ci.tools;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.oldci.CiCheck;
import ru.yandex.ci.client.oldci.OldCiClient;

@SuppressWarnings("DefaultCharset")
public class FilterChecks {
    public static void main(String[] args) throws Exception {
        OldCiClient oldCiClient = OldCiClient.create(HttpClientProperties.ofEndpoint(Misc.OLD_CI_API_URL));

        String baseDir = "/Users/andreevdm/tmp/fire/";

        BufferedReader reader = new BufferedReader(new FileReader(new File(baseDir, "builds")));

        FileWriter filteredWriter = new FileWriter(new File(baseDir, "filtered"), true);

        List<String> checks = new ArrayList<>();
        String line;
        while ((line = reader.readLine()) != null) {
//            if (line.compareTo("2zqaj") > 1){
            checks.add(line);
//            }
        }

        for (String check : checks) {
            if (check.contains("name")) {
                continue;
            }

            CiCheck ciCheck;
            try {
                ciCheck = oldCiClient.getCheck(check);
            } catch (HttpException e) {
                if (e.getHttpCode() == 404) {
                    continue;
                } else {
                    throw e;
                }
            }

            if (Boolean.TRUE.equals(ciCheck.getPessimized())) {
                continue;
            }


            filteredWriter.write(check);
            filteredWriter.write('\n');
            filteredWriter.flush();
            System.out.println(check);


        }


        filteredWriter.close();
//        ArcanumPullRequestActivity activity = client.getPullRequesActivities(startPr);
    }


}
