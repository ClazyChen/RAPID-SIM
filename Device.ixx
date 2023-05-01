module;
#include <memory>

export module rapid.Device;

import rapid.Packet;

export class Device {
public:
    virtual Packet next(Packet&&) = 0;
};

export class DualPortDevice {
public:
    virtual Packet next1(Packet&&) = 0;
    virtual Packet next2(Packet&&) = 0;
    virtual short get_lock_key() {
        return 0;
    }
    virtual void unlock_key(short key) { }
};

export enum class DeviceType {
    Read,
    Write
};

export template <DeviceType D>
class VirtualDevice : public Device {
    DualPortDevice* m_device { nullptr };

public:
    VirtualDevice(std::unique_ptr<DualPortDevice>& device)
        : m_device { device.get() }
    {
    }

    Packet next(Packet&& pkt) override
    {
        if constexpr (D == DeviceType::Read) {
            return m_device->next1(std::move(pkt));
        }
        if constexpr (D == DeviceType::Write) {
            return m_device->next2(std::move(pkt));
        }
        return Packet {};
    }
};