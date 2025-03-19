package yandex.cloud.team.integration.idm.model;

import lombok.Value;

@Value(staticConstructor = "of")
public class Subject {

    String subjectType;
    String subjectId;

}
