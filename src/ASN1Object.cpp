#include "ASN1Object.h"
#include <string.h>

ASN1Object::ASN1Object(Type type) : type_(type), data_length_(0) {
    memset(&value_, 0, sizeof(value_));
}

bool ASN1Object::decode(const uint8_t* buffer, uint16_t size, uint16_t& offset) {
    if (offset >= size) return false;
    
    // Read type
    type_ = static_cast<Type>(buffer[offset++]);
    
    // Read length
    uint16_t length;
    if (!decodeLength(buffer, size, offset, length)) return false;
    
    // Read value based on type
    switch (type_) {
        case Type::INTEGER:
            return decodeInteger(buffer, size, offset);
        case Type::OCTET_STRING:
            return decodeString(buffer, size, offset);
        case Type::OBJECT_IDENTIFIER:
            return decodeOID(buffer, size, offset);
        case Type::NULL_TYPE:
            return true;
        case Type::SEQUENCE:
            offset += length;
            return true;
        default:
            return false;
    }
}

uint16_t ASN1Object::encode(uint8_t* buffer, uint16_t maxSize) const {
    if (maxSize < 2) return 0;
    
    uint16_t totalLength = 0;
    
    // Write type
    buffer[totalLength++] = static_cast<uint8_t>(type_);
    
    // Write value based on type
    uint16_t valueLength = 0;
    switch (type_) {
        case Type::INTEGER:
            valueLength = encodeInteger(buffer + totalLength + 1, maxSize - totalLength - 1);
            break;
        case Type::OCTET_STRING:
            valueLength = encodeString(buffer + totalLength + 1, maxSize - totalLength - 1);
            break;
        case Type::OBJECT_IDENTIFIER:
            valueLength = encodeOID(buffer + totalLength + 1, maxSize - totalLength - 1);
            break;
        case Type::NULL_TYPE:
            valueLength = 0;
            break;
        default:
            return 0;
    }
    
    // Write length
    uint16_t lengthBytes = encodeLength(buffer + totalLength, maxSize - totalLength, valueLength);
    if (lengthBytes == 0) return 0;
    
    totalLength += lengthBytes + valueLength;
    return totalLength;
}

int32_t ASN1Object::getInteger() const {
    return (type_ == Type::INTEGER) ? value_.integer : 0;
}

const char* ASN1Object::getString() const {
    return (type_ == Type::OCTET_STRING) ? value_.string.text : "";
}

size_t ASN1Object::getStringLength() const {
    return (type_ == Type::OCTET_STRING) ? value_.string.length : 0;
}

const uint32_t* ASN1Object::getOID() const {
    return (type_ == Type::OBJECT_IDENTIFIER) ? value_.oid.components : nullptr;
}

size_t ASN1Object::getOIDLength() const {
    return (type_ == Type::OBJECT_IDENTIFIER) ? value_.oid.count : 0;
}

void ASN1Object::setInteger(int32_t value) {
    type_ = Type::INTEGER;
    value_.integer = value;
}

void ASN1Object::setString(const char* value, size_t length) {
    type_ = Type::OCTET_STRING;
    size_t copyLength = (length < MAX_STRING_LENGTH) ? length : MAX_STRING_LENGTH - 1;
    memcpy(value_.string.text, value, copyLength);
    value_.string.text[copyLength] = '\0';
    value_.string.length = copyLength;
}

void ASN1Object::setOID(const uint32_t* oid, size_t length) {
    type_ = Type::OBJECT_IDENTIFIER;
    size_t copyLength = (length < MAX_OID_LENGTH) ? length : MAX_OID_LENGTH;
    memcpy(value_.oid.components, oid, copyLength * sizeof(uint32_t));
    value_.oid.count = copyLength;
}

bool ASN1Object::decodeLength(const uint8_t* buffer, uint16_t size, uint16_t& offset, uint16_t& length) const {
    if (offset >= size) return false;
    
    uint8_t firstByte = buffer[offset++];
    if (firstByte < 0x80) {
        length = firstByte;
        return true;
    }
    
    uint8_t numBytes = firstByte & 0x7F;
    if (numBytes > 2 || offset + numBytes > size) return false;
    
    length = 0;
    for (uint8_t i = 0; i < numBytes; i++) {
        length = (length << 8) | buffer[offset++];
    }
    
    return true;
}

uint16_t ASN1Object::encodeLength(uint8_t* buffer, uint16_t maxSize, uint16_t length) const {
    if (length < 0x80) {
        if (maxSize < 1) return 0;
        buffer[0] = length;
        return 1;
    }
    
    if (length < 0x100) {
        if (maxSize < 2) return 0;
        buffer[0] = 0x81;
        buffer[1] = length;
        return 2;
    }
    
    if (maxSize < 3) return 0;
    buffer[0] = 0x82;
    buffer[1] = (length >> 8) & 0xFF;
    buffer[2] = length & 0xFF;
    return 3;
}

bool ASN1Object::decodeInteger(const uint8_t* buffer, uint16_t size, uint16_t& offset) {
    uint16_t length;
    if (!decodeLength(buffer, size, offset, length) || length > 4 || offset + length > size) return false;
    
    value_.integer = 0;
    if (buffer[offset] & 0x80) {
        value_.integer = -1;  // Sign extend
    }
    
    for (uint16_t i = 0; i < length; i++) {
        value_.integer = (value_.integer << 8) | buffer[offset++];
    }
    
    return true;
}

bool ASN1Object::decodeString(const uint8_t* buffer, uint16_t size, uint16_t& offset) {
    uint16_t length;
    if (!decodeLength(buffer, size, offset, length) || offset + length > size) return false;
    
    size_t copyLength = (length < MAX_STRING_LENGTH - 1) ? length : MAX_STRING_LENGTH - 1;
    memcpy(value_.string.text, buffer + offset, copyLength);
    value_.string.text[copyLength] = '\0';
    value_.string.length = copyLength;
    
    offset += length;
    return true;
}

bool ASN1Object::decodeOID(const uint8_t* buffer, uint16_t size, uint16_t& offset) {
    uint16_t length;
    if (!decodeLength(buffer, size, offset, length) || offset + length > size) return false;
    
    // First byte encodes first two OID components
    if (length < 1) return false;
    uint8_t firstByte = buffer[offset++];
    value_.oid.components[0] = firstByte / 40;
    value_.oid.components[1] = firstByte % 40;
    value_.oid.count = 2;
    
    // Decode remaining components
    uint32_t value = 0;
    for (uint16_t i = 1; i < length && value_.oid.count < MAX_OID_LENGTH; i++) {
        uint8_t byte = buffer[offset++];
        value = (value << 7) | (byte & 0x7F);
        if ((byte & 0x80) == 0) {
            value_.oid.components[value_.oid.count++] = value;
            value = 0;
        }
    }
    
    return true;
}

uint16_t ASN1Object::encodeInteger(uint8_t* buffer, uint16_t maxSize) const {
    // Determine minimum number of bytes needed
    uint8_t length = 1;
    int32_t value = value_.integer;
    while ((value < -128 || value > 127) && length < 4) {
        length++;
        value >>= 8;
    }
    
    if (maxSize < length) return 0;
    
    // Write integer bytes
    value = value_.integer;
    for (int i = length - 1; i >= 0; i--) {
        buffer[i] = value & 0xFF;
        value >>= 8;
    }
    
    return length;
}

uint16_t ASN1Object::encodeString(uint8_t* buffer, uint16_t maxSize) const {
    if (maxSize < value_.string.length) return 0;
    
    memcpy(buffer, value_.string.text, value_.string.length);
    return value_.string.length;
}

uint16_t ASN1Object::encodeOID(uint8_t* buffer, uint16_t maxSize) const {
    if (value_.oid.count < 2 || maxSize < 1) return 0;
    
    uint16_t length = 0;
    
    // Encode first two components in first byte
    buffer[length++] = (value_.oid.components[0] * 40) + value_.oid.components[1];
    
    // Encode remaining components
    for (size_t i = 2; i < value_.oid.count; i++) {
        uint32_t value = value_.oid.components[i];
        uint8_t bytes[5];  // Maximum 5 bytes for 32-bit value
        int numBytes = 0;
        
        // Convert to base-128 encoding
        do {
            uint8_t byte = (value & 0x7F) | (numBytes > 0 ? 0x80 : 0);
            bytes[numBytes] = byte;
            numBytes++;
            value >>= 7;
        } while (value > 0 && numBytes < 5);
        
        // Write bytes in reverse order
        if (length + numBytes > maxSize) return 0;
        for (int j = numBytes - 1; j >= 0; j--) {
            buffer[length++] = bytes[j];
        }
    }
    
    return length;
}
