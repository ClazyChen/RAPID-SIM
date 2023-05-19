export module rapid.VirtualPipeline;

import rapid.Packet;
import rapid.BlockQueue;
import rapid.Device;
import std;

export template <size_t Len>
    requires(Len > 0)
class VirtualPipeline : public Device {
    constexpr const static size_t m_length { Len };

    BlockQueue<Packet, m_length> m_queue;

public:
    VirtualPipeline() {
        //std::cout << "create virtual pipeline len = " << m_length << std::endl;
    }
    Packet next(Packet&& pkt) override
    {
        m_queue.enqueue(std::move(pkt));
        return m_queue.next();
    }
};