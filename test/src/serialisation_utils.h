/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#ifndef META_TEST_SERIALISATION_UTILS_HEADER
#define META_TEST_SERIALISATION_UTILS_HEADER

#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/io/intf_file.h>
#include <core/log.h>

#include <meta/interface/intf_object.h>
#include <meta/interface/serialization/intf_global_serialization_data.h>

META_BEGIN_NAMESPACE()
namespace Test {

struct MemFile : public CORE_NS::IFile {
    explicit MemFile(BASE_NS::vector<char> vec = {});

    Mode GetMode() const override;
    void Close() override;
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    uint64_t Append(const void* buffer, uint64_t count, uint64_t flushSize) override;
    uint64_t GetLength() const override;
    bool Seek(uint64_t offset) override;
    uint64_t GetPosition() const override;

    BASE_NS::vector<char> Data() const;

private:
    void Destroy() override;
    BASE_NS::vector<char> data_;
    size_t pos_ {};
};

struct TestSerialiser {
    explicit TestSerialiser(BASE_NS::vector<char> data = {});

    bool Export(const IObject::Ptr& object);
    IObject::Ptr Import();

    template<typename Interface>
    bool Export(const BASE_NS::shared_ptr<Interface>& p)
    {
        return Export(interface_pointer_cast<IObject>(p));
    }

    template<typename Interface>
    typename Interface::Ptr Import()
    {
        return interface_pointer_cast<Interface>(Import());
    }

    bool LoadFile(BASE_NS::string_view path);
    void Dump(BASE_NS::string_view file);
    BASE_NS::string Get() const;

    void SetSerializationSettings(SerializationSettings s);

    BASE_NS::unordered_map<InstanceId, InstanceId> GetInstanceIdMapping() const;
    void SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId>);

private:
    MemFile data_;
    std::size_t writePos_ {};
    std::size_t readPos_ {};
    BASE_NS::unordered_map<InstanceId, InstanceId> mapping_;
};

void WriteToFile(const BASE_NS::vector<char>&, BASE_NS::string_view file);

} // namespace Test

META_END_NAMESPACE()

#endif
