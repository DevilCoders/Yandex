from amazon.aws import AmazonComputeUploader, AmazonStorageUploader
from azure.azure_compute import AzureLinuxTariffFactory, AzureWindowsTariffFactory
from azure.azure_compute_scrapper import AzureComputeScrapper
from azure.azure_storage import AzureStorageTariffFactory
from azure.azure_storage_scrapper import AzureStorageScrapperNew
from core.compute_dashboard import ComputeDashboard
from core.storage_dashboard import StorageDashboard
from core.utils import *
from google_cloud.google_compute import GoogleLinuxComputeTariffFactory, GoogleWindowsComputeTariffFactory
from google_cloud.google_compute_scrapper import GoogleComputeScrapper
from google_cloud.google_storage import GoogleStorageTariffFactory
from google_cloud.google_storage_scrapper import GoogleStorageScrapper
from selectel.selectel_compute import SelectedLinuxComputeFactory
from selectel.selectel_storage import SelectelStorageTariffFactory
from selectel.selectel_storage_scrapper import SelectelStorageScrapper
from vk.vk_compute import VkLinuxComputeTariffFactory, VkWindowsComputeTariffFactory
from yndx.yndx_compute import YndxLinuxComputeTariffFactory, YndxWinComputeTariffFactory
from yndx.yndx_compute_scrapper import YndxComputeScrapper


def run_compute_scrapping() -> ComputeDashboard:
    comp_dash = ComputeDashboard()
    # Yandex
    yndx_scrapper = YndxComputeScrapper('https://cloud.yandex.com/en/docs/compute/pricing')
    yndx_linux_factory = YndxLinuxComputeTariffFactory(yndx_scrapper)
    yndx_win_factory = YndxWinComputeTariffFactory(yndx_linux_factory)

    comp_dash.add_tariff_factory("Yandex Linux", yndx_linux_factory)
    comp_dash.add_tariff_factory("Yandex Windows", yndx_win_factory)

    # Azure

    azure_linux_factory = AzureLinuxTariffFactory('Azure', AzureComputeScrapper())
    azure_windows_factory = AzureWindowsTariffFactory('Azure', AzureComputeScrapper())

    comp_dash.add_tariff_factory("Azure Linux", azure_linux_factory)
    comp_dash.add_tariff_factory("Azure Windows", azure_windows_factory)

    # Google

    google_factory = GoogleLinuxComputeTariffFactory('Google', GoogleComputeScrapper())
    comp_dash.add_tariff_factory("Google linux", google_factory)

    win_factory = GoogleWindowsComputeTariffFactory('Google', GoogleComputeScrapper())
    comp_dash.add_tariff_factory("Google windows", win_factory)

    # Amazon
    comp_dash.add_tariff_factory("Amazon Linux", AmazonComputeUploader(OperSys.Linux))
    comp_dash.add_tariff_factory("Amazon Windows", AmazonComputeUploader(OperSys.Windows))

    # Selectel
    sel_linux = SelectedLinuxComputeFactory('Selectel')
    comp_dash.add_tariff_factory("Selectel Linux", sel_linux)

    # VK
    vk_linux = VkLinuxComputeTariffFactory('VK')
    comp_dash.add_tariff_factory('VK_linux', vk_linux)
    vk_win = VkWindowsComputeTariffFactory('VK')
    comp_dash.add_tariff_factory('VK_windows', vk_win)

    comp_dash.calculate()

    return comp_dash


def run_storage_scrapper() -> StorageDashboard:
    dash = StorageDashboard()
    # Amazon
    aws = AmazonStorageUploader(url="https://a0.p.awsstatic.com/pricing/1.0/s3/region/eu-central-1/index.json")
    dash.add_scrapper('Amazon', aws)
    # Azure
    azure = AzureStorageTariffFactory('Azure', AzureStorageScrapperNew())
    dash.add_scrapper('Azure', azure)
    # Google
    google = GoogleStorageTariffFactory('Google', GoogleStorageScrapper('https://cloud.google.com/storage/pricing#europe'))
    dash.add_scrapper('Google', google)
    # Selectel
    sel = SelectelStorageTariffFactory('Selectel')
    dash.add_scrapper('Selectel', sel)

    dash.calculate()

    return dash

if __name__ == "__main__":
    pandas_display_all()
    print('Fetching compute tariffs...')
    dash = run_compute_scrapping()
    dash.tariffs_flat_table.to_csv("output/Compute.csv", index_label='ID')
    dash.tariffs_flat_table.to_excel("output/Compute.xlsx", index_label='ID')

    print('Fetching storage tariffs...')
    dash = run_storage_scrapper()
    dash.storage_table.to_csv('output/Storage tariffs.csv', index_label='ID')
    dash.storage_table.to_excel('output/Storage tariffs.xlsx', index_label='ID')
