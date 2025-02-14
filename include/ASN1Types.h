#ifndef ASN1_TYPES_H
#define ASN1_TYPES_H

#include <Arduino.h>

namespace ASN1 {

// ASN.1 Type Tags (Universal)
constexpr uint8_t INTEGER_TAG = 0x02;
constexpr uint8_t OCTET_STRING_TAG = 0x04;
constexpr uint8_t NULL_TAG = 0x05;
constexpr uint8_t OBJECT_IDENTIFIER_TAG = 0x06;
constexpr uint8_t SEQUENCE_TAG = 0x30;

// SNMP Application-specific Tags
constexpr uint8_t COUNTER_TAG = 0x41;  // [APPLICATION 1]
constexpr uint8_t GAUGE_TAG = 0x42;    // [APPLICATION 2]
constexpr uint8_t TIMETICKS_TAG = 0x43; // [APPLICATION 3]

// ASN.1 Tag Classes
enum class TagClass : uint8_t {
    Universal       = 0b00000000,
    Application    = 0b01000000,
    ContextSpecific = 0b10000000,
    Private        = 0b11000000
};

// ASN.1 Encoding Type
enum class EncodingType : uint8_t {
    Primitive   = 0b00000000,
    Constructed = 0b00100000
};

// Universal Type Tags
enum class UniversalTag : uint8_t {
    Boolean     = 1,
    Integer     = 2,
    BitString   = 3,
    OctetString = 4,
    Null        = 5,
    ObjectID    = 6,
    Sequence    = 16,
    Set         = 17
};

// Maximum sizes
constexpr size_t MAX_INT_LENGTH = 4;        // 32-bit integers
constexpr size_t MAX_OID_LENGTH = 32;       // Maximum OID components
constexpr size_t MAX_STRING_LENGTH = 256;   // Maximum string length
constexpr size_t MAX_SEQUENCE_LENGTH = 512; // Maximum sequence length

// Forward declarations
class Encoder;
class Decoder;

// Base class for all ASN.1 types
class Type {
public:
    virtual ~Type() = default;
    virtual size_t encode(uint8_t* buffer, size_t size) const = 0;
    virtual bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) = 0;
    virtual UniversalTag getTag() const = 0;
    
protected:
    static size_t encodeLength(uint8_t* buffer, size_t length);
    static bool decodeLength(const uint8_t* buffer, size_t& length, size_t& bytesRead);
    static size_t encodedLengthSize(size_t length);
};

// INTEGER type
class Integer : public Type {
public:
    Integer() : value(0) {}
    explicit Integer(int32_t val) : value(val) {}
    
    UniversalTag getTag() const override { return UniversalTag::Integer; }
    size_t encode(uint8_t* buffer, size_t size) const override;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) override;
    
    int32_t getValue() const { return value; }
    void setValue(int32_t val) { value = val; }
    
private:
    int32_t value;
};

// OCTET STRING type
class OctetString : public Type {
public:
    OctetString() : length(0) { data[0] = '\0'; }
    explicit OctetString(const char* str);
    
    UniversalTag getTag() const override { return UniversalTag::OctetString; }
    size_t encode(uint8_t* buffer, size_t size) const override;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) override;
    
    const char* getValue() const { return data; }
    void setValue(const char* str);
    
private:
    char data[MAX_STRING_LENGTH];
    size_t length;
};

// NULL type
class Null : public Type {
public:
    UniversalTag getTag() const override { return UniversalTag::Null; }
    size_t encode(uint8_t* buffer, size_t size) const override;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) override;
};

// OBJECT IDENTIFIER type
class ObjectIdentifier : public Type {
public:
    ObjectIdentifier() : numComponents(0) {}
    ObjectIdentifier(const uint32_t* components, size_t count);
    
    UniversalTag getTag() const override { return UniversalTag::ObjectID; }
    size_t encode(uint8_t* buffer, size_t size) const override;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) override;
    
    const uint32_t* getComponents() const { return components; }
    size_t getComponentCount() const { return numComponents; }
    bool setComponents(const uint32_t* newComponents, size_t count);
    
private:
    uint32_t components[MAX_OID_LENGTH];
    size_t numComponents;
};

// SEQUENCE type
class Sequence : public Type {
public:
    Sequence() : length(0) {}
    
    UniversalTag getTag() const override { return UniversalTag::Sequence; }
    size_t encode(uint8_t* buffer, size_t size) const override;
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) override;
    
    bool addData(const uint8_t* data, size_t dataLength);
    void clear() { length = 0; }
    
private:
    uint8_t data[MAX_SEQUENCE_LENGTH];
    size_t length;
};

// Helper functions for BER encoding/decoding
namespace BER {
    // Tag encoding/decoding
    uint8_t encodeTag(TagClass tagClass, EncodingType encoding, uint8_t tagNumber);
    bool decodeTag(uint8_t byte, TagClass& tagClass, EncodingType& encoding, uint8_t& tagNumber);
    
    // Length encoding/decoding
    size_t encodeLength(uint8_t* buffer, size_t length);
    bool decodeLength(const uint8_t* buffer, size_t& length, size_t& bytesRead);
    
    // Validation
    bool validateTag(uint8_t byte, TagClass expectedClass, EncodingType expectedEncoding, uint8_t expectedTag);
    bool validateLength(size_t length, size_t maxAllowed);
}

} // namespace ASN1

#endif // ASN1_TYPES_H
