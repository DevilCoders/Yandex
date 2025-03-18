namespace NAppHost::NLbs;

struct TLbsLocation {
    found: bool (required);
    latitude: double (default = 0.0);
    longitude: double (default = 0.0);
    radius: double (default = 0.0);
    source: string (allowed = ["gsm", "wifi"]);
};
