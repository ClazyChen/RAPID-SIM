export module rapid.experimental.OverlapPeer;

import rapid.Pipeline;

export template <size_t N, size_t RID1, size_t WID1, size_t RID2, size_t WID2, size_t T1 = 0, size_t K = 33, size_t PROC_NUM = 8>
    requires(RID1 < RID2 && RID2 < WID1 && WID1 < WID2)
class OverlapPeer : public Pipeline<N, K, PROC_NUM> {
protected:
    void initialize_peers() override
    {
        this->add_read_write_peer<RID1, WID1, T1, true>();
        this->add_read_write_peer<RID2, WID2>();
        this->add_lock<RID2, WID1>();
    }
};