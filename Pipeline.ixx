export module rapid.Pipeline;

import rapid.Packet;
import rapid.ReadWritePeer;
import rapid.VirtualPipeline;
import rapid.Device;
import std;

template <DeviceType IType, DeviceType OType>
consteval size_t PIPELINE_LENGTH(size_t IID, size_t OID)
{
    size_t start_stage { IID * (FB_PIR + PIR_PIW) + FB_PIR };
    if constexpr (IType == DeviceType::Write) {
        start_stage += PIR_PIW;
    }
    size_t end_stage { OID * (FB_PIR + PIR_PIW) };
    if constexpr (OType == DeviceType::Write) {
        end_stage += FB_PIR + PIR_PIW;
    }
    return end_stage - start_stage;
}

template <size_t PROC_NUM>
class PipelineBuilder {
public:
    static std::unique_ptr<Device> generate_device(DeviceType IType, DeviceType OType, size_t IID, size_t OID)
    {
        if (OID - IID == PROC_NUM) {
            if (IType == DeviceType::Read && OType == DeviceType::Write) {
                return std::make_unique<VirtualPipeline<PIPELINE_LENGTH<DeviceType::Read, DeviceType::Write>(0, PROC_NUM)>>();
            }
            if constexpr (PROC_NUM > 1) {
                if (IType == DeviceType::Write && OType == DeviceType::Read) {
                    return std::make_unique<VirtualPipeline<PIPELINE_LENGTH<DeviceType::Write, DeviceType::Read>(0, PROC_NUM)>>();
                }
            }
            if (IType == DeviceType::Read && OType == DeviceType::Read) {
                return std::make_unique<VirtualPipeline<PIPELINE_LENGTH<DeviceType::Read, DeviceType::Read>(0, PROC_NUM)>>();
            }
            if (IType == DeviceType::Write && OType == DeviceType::Write) {
                return std::make_unique<VirtualPipeline<PIPELINE_LENGTH<DeviceType::Write, DeviceType::Write>(0, PROC_NUM)>>();
            }
        }
        return PipelineBuilder<PROC_NUM - 1>::generate_device(IType, OType, 0, OID - IID);
    }
};

template <>
class PipelineBuilder<0> {
public:
    static std::unique_ptr<Device> generate_device(DeviceType IType, DeviceType OType, size_t IID, size_t OID)
    {
        return nullptr;
    }
};

export template <size_t N, size_t K = 2, size_t PROC_NUM = 4>
class Pipeline : public Device {
    constexpr const static size_t m_proc_num { PROC_NUM };

    struct ProcStatus {
        size_t m_device_id_as_pir {};
        size_t m_device_id_as_piw {};
        size_t m_dual_device_id_as_pir {};
        size_t m_dual_device_id_as_piw {};
    };

    std::array<ProcStatus, m_proc_num> m_proc_status;
    std::vector<std::unique_ptr<DualPortDevice>> m_dual_port_devices;
    std::vector<std::unique_ptr<Device>> m_devices;
    std::vector<std::unique_ptr<Device>> m_pipeline;
    std::vector<std::pair<DualPortDevice*, DualPortDevice*>> m_locks;

    PipelineBuilder<m_proc_num + 1> m_pipeline_builder;

    void create_device(DeviceType last_type, DeviceType cur_type, size_t last_id, size_t cur_id)
    {
        if (auto device { m_pipeline_builder.generate_device(last_type, cur_type, last_id, cur_id) }; device != nullptr) {
            m_pipeline.push_back(std::move(device));
        }
    }

    void build_pipeline()
    {
        size_t last_id { 0 };
        DeviceType last_type { DeviceType::Write };
        for (size_t i { 0 }; i < m_proc_num; ++i) {
            auto pir { m_proc_status.at(i).m_device_id_as_pir };
            auto piw { m_proc_status.at(i).m_device_id_as_piw };
            if (pir != 0 || piw != 0) {
                size_t cur_id { i + 1 };
                DeviceType cur_type { pir ? DeviceType::Read : DeviceType::Write };
                create_device(last_type, cur_type, last_id, cur_id);
                m_pipeline.push_back(std::move(m_devices.at(pir | piw)));
                last_id = cur_id;
                last_type = cur_type;
            }
        }
        create_device(last_type, DeviceType::Read, last_id, m_proc_num + 1);
    }

protected:
    template <size_t RID, size_t WID, size_t EXPLICIT_CLOCK_MAX = 0, bool ENABLE_UNWRITEABLE = false>
    void add_read_write_peer()
    {
        std::unique_ptr<DualPortDevice> peer { std::make_unique<ReadWritePeer<RID, WID, N, K, EXPLICIT_CLOCK_MAX, ENABLE_UNWRITEABLE>>() };
        auto read_device { std::make_unique<VirtualDevice<DeviceType::Read>>(peer) };
        auto write_device { std::make_unique<VirtualDevice<DeviceType::Write>>(peer) };
        m_proc_status.at(RID).m_device_id_as_pir = m_devices.size();
        m_devices.push_back(std::move(read_device));
        m_proc_status.at(WID).m_device_id_as_piw = m_devices.size();
        m_devices.push_back(std::move(write_device));
        m_proc_status.at(RID).m_dual_device_id_as_pir = m_dual_port_devices.size();
        m_proc_status.at(WID).m_dual_device_id_as_piw = m_dual_port_devices.size();
        m_dual_port_devices.push_back(std::move(peer));
    }

    template <size_t RID, size_t WID>
    void add_lock() {
        auto pir_id { m_proc_status.at(RID).m_dual_device_id_as_pir };
        auto piw_id { m_proc_status.at(WID).m_dual_device_id_as_piw };
        m_locks.push_back(std::make_pair(m_dual_port_devices.at(pir_id).get(), m_dual_port_devices.at(piw_id).get()));
    }

    virtual void initialize_peers() = 0;

public:
    Pipeline() = default;

    void reset() {
        for (auto& dual_port_device : m_dual_port_devices) {
            dual_port_device->reset();
        }
    }

    virtual void initialize()
    {
        m_devices.push_back(nullptr);
        this->initialize_peers();
        build_pipeline();
        // std::cout << m_pipeline.size() << std::endl;
    }

    Packet next(Packet&& pkt) override
    {
        for (auto& device : m_pipeline) {
            pkt = device->next(std::move(pkt));
        }
        for (auto& [pir, piw] : m_locks) {
            piw->unlock_key(pir->get_lock_key());
        }
        return pkt;
    }
};