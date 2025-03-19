# Building

```
python setup.py clean && \
    python setup.py preprocess && \
    cd distribute && \
    python -m pip install grpcio-tools && \
    python setup.py build_package_protos && \
    python setup.py [ sdist | bdist_wheel ])
```
