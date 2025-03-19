#  type: ignore
#  All mypy just turned off because we did't write it and we don't know if it is necessary

import logging
import xmltodict
import zeep
import numpy as np

logger = logging.getLogger(__name__)


def xml_to_dict(xml):
    return xmltodict.parse(xml.xmlData, dict_constructor=dict)


class XmlToDictProxy:
    def __init__(self, service) -> None:
        self._service = service

    def __getattr__(self, attr):
        def wrapped_method(*args, **kwargs):
            result = xml_to_dict(getattr(self._service, attr)(*args, **kwargs))
            return result
        return wrapped_method


class InterFaxAdapter:
    def __init__(self, url="http://webservicefarm.interfax.ru/IfaxWebService/ifaxwebservice.asmx?WSDL", login='yandex', password='QZ0nEzj'):
        """SPARK (СПАРК) InterFax Adapter. Look https://st.yandex-team.ru/CLOUDANA-389
          and https://st.yandex-team.ru/ZORAHELPDESK-1504  for details.

          Documentation is here https://st.yandex-team.ru/CLOUDANA-389#5e466cef028da4181a44f329
        """
        self._client = zeep.Client(wsdl=url)
        self._client.service.Authmethod(login, password)

    @property
    def service(self):
        return XmlToDictProxy(self._client.service)

    def get_extended_report(self, inn: str):
        company_extended = None
        spark_company_extended = None
        try:
            company_extended = self.service.GetCompanyExtendedReport('', inn, '')['Response']['Data']
        except KeyError:
            logger.error(f'{spark_company_extended}')

        return company_extended

    @staticmethod
    def _period_revenue(finance):
        return sum([int(fin['@Value']) for fin in finance if fin['@Name']=='Выручка'])

    @staticmethod
    def get_company_name(company_extended):
        name = None
        try:
            name = company_extended['Report']['ShortNameRus']
        except Exception as e :
            logger.error(company_extended)
            logger.error(e, exc_info=True)
        return name

    @staticmethod
    def get_company_size(company_extended):
        company_size = None
        try:
            company_size = company_extended['Report']['CompanySize']['@Description']
        except Exception as e :
            logger.error(company_extended)
            logger.error(e, exc_info=True)
        return company_size

    @staticmethod
    def latest_year_revenue(company_extended):
        revenue = np.nan
        max_year = np.nan
        try:
            finance = company_extended["Report"]["Finance"]['FinPeriod']
            if isinstance(finance, list):
                years = np.array([int(per['@PeriodName']) for per in finance])
                max_year = np.max(years)
                last_idx = np.where(years == max_year)[0]

                if len(last_idx)>0:
                    revenue = InterFaxAdapter._period_revenue(finance[last_idx[0]]['StringList']['String'])
            else:
                max_year = int(finance['@PeriodName'])
                revenue = InterFaxAdapter._period_revenue(finance['StringList']['String'])
                if revenue > 0:
                    1==1

        except KeyError as e:
            cause = e.args[0]
            if not (cause in 'Data', 'Finance'):  # No info in SPARK
                logger.error(f'{cause}')
        except TypeError as e :
            logger.error(e, exc_info=True)
        except Exception as e :
            logger.error(e, exc_info=True)
        return {'last_known_year': max_year, 'last_known_year_revenue': revenue}
