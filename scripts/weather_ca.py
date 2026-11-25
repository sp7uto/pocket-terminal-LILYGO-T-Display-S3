Import("env")
import certifi
import ssl
import pathlib
import re
import tempfile

bundle = pathlib.Path(certifi.where()).read_text()
selected = []
for pem in re.findall(r"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", bundle, re.S):
    with tempfile.NamedTemporaryFile(mode="w", suffix=".pem", delete=False) as f:
        f.write(pem + "\n")
        name = f.name
    try:
        info = ssl._ssl._test_decode_cert(name)
        cn = [v for rdn in info["subject"] for k,v in rdn if k == "commonName"]
        if any(x.startswith("ISRG Root") for x in cn):
            selected.append(pem)
    finally:
        pathlib.Path(name).unlink()
if not selected:
    raise RuntimeError("ISRG trust roots missing")
out = pathlib.Path(env.subst("$PROJECT_DIR")) / "include" / "weather_ca.h"
out.write_text('#pragma once\nstatic const char WEATHER_CA[] = R"PEM(' + "\n".join(selected) + '\n)PEM";\n')
