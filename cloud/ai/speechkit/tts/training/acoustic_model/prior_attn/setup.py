from setuptools import setup
from torch.utils.cpp_extension import BuildExtension, CUDAExtension

setup(
    name="prior_attn_cuda",
    ext_modules=[
        CUDAExtension("prior_attn_cuda", [
            "prior_attn.cu"
        ])
    ],
    cmdclass={
        "build_ext": BuildExtension
    }
)
