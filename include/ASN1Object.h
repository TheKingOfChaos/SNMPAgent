#ifndef ASN1_OBJECT_H
#define ASN1_OBJECT_H

#include <cstdint>
#include <cstddef>

class ASN1Object {
public:
    enum class Type {
        INTEGER = 0x02,
        OCTET_STRING = 0x04,
        NULL_TYPE = 0x05,
        OBJECT_IDENTIFIER = 0x06,
        SEQUENCE = 0x30
    };
    
    static constexpr size_t MAX_DATA_SIZE = 256;
    static constexpr size_t MAX_OID_LENGTH = 32;
    static constexpr size_t MAX_STRING_LENGTH = 64;
    
    ASN1Object(Type type = Type::NULL_TYPE);
    
    // Encoding/Decoding
    bool decode(const uint8_t* buffer, uint16_t size, uint16_t& offset);
    uint16_t encode(uint8_t* buffer, uint16_t maxSize) const;
    
    // Type-specific getters
    int32_t getInteger() const;
    const char* getString() const;
    size_t getStringLength() const;
    const uint32_t* getOID() const;
    size_t getOIDLength() const;
    
    // Type-specific setters
    void setInteger(int32_t value);
    void setString(const char* value, size_t length);
    void setOID(const uint32_t* oid, size_t length);
    
    // Type information
    Type getType() const { return type_; }
    void setType(Type type) { type_ = type; }
    
private:
    Type type_;
    uint8_t data_[MAX_DATA_SIZE];
    size_t data_length_;
    
    // For OID and String types
    union {
        struct {
            uint32_t components[MAX_OID_LENGTH];
            size_t count;
        } oid;
        struct {
            char text[MAX_STRING_LENGTH];
            size_t length;
        } string;
        int32_t integer;
    } value_;
    
    // Helper methods
    bool decodeLength(const uint8_t* buffer, uint16_t size, uint16_t& offset, uint16_t& length) const;
    uint16_t encodeLength(uint8_t* buffer, uint16_t maxSize, uint16_t length) const;
    bool decodeInteger(const uint8_t* buffer, uint16_t size, uint16_t& offset);
    bool decodeString(const uint8_t* buffer, uint16_t size, uint16_t& offset);
    bool decodeOID(const uint8_t* buffer, uint16_t size, uint16_t& offset);
    uint16_t encodeInteger(uint8_t* buffer, uint16_t maxSize) const;
    uint16_t encodeString(uint8_t* buffer, uint16_t maxSize) const;
    uint16_t encodeOID(uint8_t* buffer, uint16_t maxSize) const;
};

#endif // ASN1_OBJECT_H
