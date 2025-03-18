from dssclient.signing_params import XmlSigningParams, PdfSigningParams, CmsSigningParams


def test_basic():

    params = XmlSigningParams(20, '1122').set_type_templated()
    as_dict = params._asdict()

    assert as_dict == {
        'Signature': {'CertificateId': 20, 'Parameters': {'XMLDsigType': 'XMLTemplate'}, 'Type': 0, 'PinCode': '1122'}}

    params = CmsSigningParams().make_detached().sign_hash()
    as_dict = params._asdict()

    assert as_dict == {
        'Signature': {
            'CertificateId': 0, 'Parameters': {'Hash': 'True', 'IsDetached': 'True'}, 'PinCode': '', 'Type': 5}}

    params.make_attached()
    as_dict = params._asdict()

    assert as_dict == {
        'Signature': {
            'CertificateId': 0, 'Parameters': {'Hash': 'True', 'IsDetached': 'False'}, 'PinCode': '', 'Type': 5}}


def test_pdf():

    params = PdfSigningParams()
    params.set_stamp(PdfSigningParams.cls_stamp('sometext'), page=11)

    params.reason = 'reason'
    params.location = 'location'

    d = params._asdict()

    assert d['Signature']['CertificateId'] == 0
    assert d['Signature']['Parameters']['PDFFormat'] == 'CMS'
    assert d['Signature']['Parameters']['PDFLocation'] == 'location'
    assert d['Signature']['Parameters']['PDFReason'] == 'reason'
    assert d['Signature']['Parameters']['PdfSignatureTemplateId'] == 1
    assert d['Signature']['Parameters']['PdfSignatureAppearance']

    params = PdfSigningParams(stamp='onestring')
    assert isinstance(params.stamp, params.cls_stamp)
