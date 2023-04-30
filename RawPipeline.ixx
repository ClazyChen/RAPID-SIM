export module rapid.experimental.RawPipeline;

import rapid.Pipeline;

export template <size_t N, size_t K = 2, size_t PROC_NUM = 4>
class RawPipeline : public Pipeline<N, K, PROC_NUM> {
protected:
    void initialize_peers() override { }
};