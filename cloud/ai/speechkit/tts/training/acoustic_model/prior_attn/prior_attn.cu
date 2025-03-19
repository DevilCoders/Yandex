#include <torch/extension.h>

template <typename T>
__device__ T betaln(T a, T b) {
    return lgammaf(a) + lgammaf(b) - lgammaf(a + b);
}

template <typename T>
__device__ T pmf(T x, T n, T a, T b) {
    const auto combiln = -logf(n + 1) - betaln(n - x + 1, x + 1);
    return exp(combiln + betaln(x + a, n - x + b) - betaln(a, b));
}

template<class T>
__global__ void prior_attn_kernel(torch::PackedTensorAccessor32<T, 3, torch::RestrictPtrTraits> array,
                                  torch::PackedTensorAccessor32<int64_t, 1, torch::RestrictPtrTraits> m_lengths,
                                  torch::PackedTensorAccessor32<int64_t, 1, torch::RestrictPtrTraits> n_lengths) {
    const auto n = blockIdx.z; // batch idx
    const auto i = threadIdx.x + blockIdx.x * blockDim.x;
    const auto j = threadIdx.y + blockIdx.y * blockDim.y;

    const auto M = m_lengths[n];
    const auto N = n_lengths[n];

    if (i < M && j < N && i < array.size(1) && j < array.size(2)) {
        array[n][i][j] = pmf(T(j), T(N), T(i + 1), T(M - i));
    }
}

void prior_attn(torch::Tensor array, torch::Tensor m_lengths, torch::Tensor n_lengths) {
    AT_DISPATCH_FLOATING_TYPES(
        array.scalar_type(), "prior_attn", [&]() {
            const int64_t threads = 32;
            const int64_t batch_size = array.size(0);
            const int64_t M = array.size(1);
            const int64_t N = array.size(2);
            const int64_t n_blocks_x = (M % threads) ? M / threads + 1 : M / threads;
            const int64_t n_blocks_y = (N % threads) ? N / threads + 1 : N / threads;
            prior_attn_kernel<<<dim3(n_blocks_x, n_blocks_y, batch_size), dim3(threads, threads)>>>(
                array.packed_accessor32<scalar_t, 3, torch::RestrictPtrTraits>(),
                m_lengths.packed_accessor32<int64_t, 1, torch::RestrictPtrTraits>(),
                n_lengths.packed_accessor32<int64_t, 1, torch::RestrictPtrTraits>()
            );
        }
    );
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("prior_attn", &prior_attn, "generate prior attention matrix");
}
