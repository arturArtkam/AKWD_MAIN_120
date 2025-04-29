#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include <cstring>

class Iserializable
{
protected:
    ~Iserializable() = default;

public:
    virtual std::size_t serialize(uint8_t* buf) = 0;
    virtual std::size_t deserialize(const uint8_t* buf) = 0;
    virtual std::size_t serialized_size() const = 0;

    template<typename T>
    size_t encode(uint8_t* buf, T value)
    {
        std::memcpy(buf, reinterpret_cast<const char*>(&value), sizeof(value));
        return sizeof(value);
    }
};

#endif /* SERIALIZE_H_INCLUDED */
