package yandex.cloud.team.integration.idm;

public interface TestResources {

    String ABC_SERVICE_SLUG = "home";

    long ABC_SERVICE_ID = 2;

    String ABC_FOLDER_ID = "abc-folder-id";

    long ABC_GROUP_ID = 42868;

    String SERVICE_SPEC = ABC_SERVICE_SLUG + "-" + ABC_SERVICE_ID + "-" + ABC_GROUP_ID;

    String EXTRA_FIELDS = "{\"service_spec\": \"" + SERVICE_SPEC + "\"}";

}
