p = ["catboost", "xgboost", "lightgbm", "tensorflow", "scipy", "keras", "mxnet", "numpy", "pandas", "matplotlib", "cntk", "torch"]

mess = ""
for pp in p:
    m = __import__(pp)
    mess += pp + "\t" + m.__version__ + "\n"
print(mess)

