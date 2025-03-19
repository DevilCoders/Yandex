#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc54() {
    // work
    {
        TInfo info(SE_HH, ST_JOB, "java programmer", ESearchFlags(SF_LOCAL | SF_SEARCH));

        KS_TEST_URL("http://hh.ru/applicant/searchvacancyresult.xml?notWithoutSalary=&text=java+programmer&professionalAreaId=0&desireableCompensation=&compensationCurrencyCode=RUR", info);
        info.Name = SE_RABOTA_RU;
        KS_TEST_URL("http://www.rabota.ru/v3_searchVacancyByParamsResults.html?action=search&area=v3_searchVacancyByParamsResults&p=30&ot=t&c=1&w=java+programmer", info);
        info.Name = SE_YANDEX;
        KS_TEST_URL("http://rabota.yandex.ru/search.xml/?text=java+programmer&rid=", info);
        KS_TEST_URL("http://rabota.yandex.ua/search.xml/?text=java+programmer&rid=", info);
        KS_TEST_URL("http://rabota.yandex.kz/search.xml/?text=java+programmer&rid=", info);
        KS_TEST_URL("http://rabota.yandex.by/search.xml/?text=java+programmer&rid=", info);
        KS_TEST_URL("https://rabota.yandex.ru/search?text=java%20programmer&rid=", info);

        info.Flags = ESearchFlags(SF_LOCAL);
        KS_TEST_URL("https://rabota.yandex.ru/salary?text=java%20programmer", info);
        info.Flags = ESearchFlags(SF_LOCAL | SF_SEARCH);

        info.Name = SE_SUPERJOB;
        KS_TEST_URL("http://www.superjob.ru/vacancy/search/?sbmit=1&keywords%5B0%5D%5Bkeys%5D=java+programmer&keywords%5B0%5D%5Bskwc%5D=and&keywords%5B0%5D%5Bsrws%5D=10&exclude_words=&period=0&place_of_work=0&t%5B%5D=0&catalogues=&pay1=&pay2=&type_of_work=0&age=&pol=0&education=0&lng=0&agency=0", info);
        info.Name = SE_JOB_RU;
        KS_TEST_URL("http://www.job.ru/seeker/job/?q=java+programmer&period=0&rgnc=B2&srch=1", info);
        info.Flags = ESearchFlags(SF_SOCIAL | SF_SEARCH | SF_LOCAL);
        info.Name = SE_MOIKRUG;
        KS_TEST_URL("http://moikrug.ru/vacancies/?keywords=java+programmer&salary_from=&submitted=1&in_results=s1&city_did=678&salary_currency=rur", info);
        info.Type = ST_PEOPLE;
        KS_TEST_URL("http://moikrug.ru/persons/?keywords=java+programmer&submitted=1", info);
        info.Type = ST_COM;
        KS_TEST_URL("http://moikrug.ru/companies/search/?keywords=java+programmer&submitted=1", info);
        KS_TEST_URL("http://moikrug.ru/services/?keywords=java+programmer&submitted=1", info);
    }
}
