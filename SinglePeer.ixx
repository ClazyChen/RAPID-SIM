export module rapid.experimental.SinglePeer;

import rapid.Pipeline;

export template <size_t N, size_t RID, size_t WID, size_t K = 2, size_t PROC_NUM = 4>
    requires(RID < WID)
class SinglePeer : public Pipeline<N, K, PROC_NUM> {
protected:
    void initialize_peers() override {
        this->add_read_write_peer<RID, WID>();
    }
};