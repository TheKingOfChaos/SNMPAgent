#include "ASN1Types.h"
#include <string.h>

namespace ASN1 {

// Type base class implementation
size_t Type::encodeLength(uint8_t* buffer, size_t length) {
    return BER::encodeLength(buffer, length);
}

bool Type::decodeLength(const uint8_t* buffer, size_t& length, size_t& bytesRead) {
    return BER::decodeLength(buffer, length, bytesRead);
}

size_t Type::encodedLengthSize(size_t length) {
    if (length < 128) {
        return 1;
    }
    size_t bytes = 1;
    while (length > 0) {
        length >>= 8;
        bytes++;
    }
    return bytes;
}

// INTEGER implementation
size_t Integer::encode(uint8_t* buffer, size_t size) const {
    if (!buffer || size < 2) return 0;
    
    // Write tag
    buffer[0] = BER::encodeTag(TagClass::Universal, EncodingType::Primitive, static_cast<uint8_t>(UniversalTag::Integer));
    
    // Determine number of bytes needed
    uint8_t valueBytes[4];
    size_t valueSize = 0;
    int32_t val = value;
    
    // Handle special case for zero
    if (val == 0) {
        valueBytes[0] = 0;
        valueSize = 1;
    } else {
        // Convert to bytes, handling both positive and negative numbers
        bool needsLeadingZero = (val > 0 && (val & 0x80000000));
        do {
            valueBytes[valueSize++] = val & 0xFF;
            val >>= 8;
        } while (val != 0 && valueSize < sizeof(valueBytes));
        
        if (needsLeadingZero) {
            valueBytes[valueSize++] = 0;
        }
    }
    
    // Write length
    size_t lengthBytes = encodeLength(buffer + 1, valueSize);
    if (lengthBytes == 0) return 0;
    
    // Write value bytes in reverse order (big-endian)
    for (size_t i = 0; i < valueSize; i++) {
        buffer[1 + lengthBytes + i] = valueBytes[valueSize - 1 - i];
    }
    
    return 1 + lengthBytes + valueSize;
}

bool Integer::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    if (!buffer || size < 2) return false;
    
    // Validate tag
    if (!BER::validateTag(buffer[0], TagClass::Universal, EncodingType::Primitive, 
                         static_cast<uint8_t>(UniversalTag::Integer))) {
        return false;
    }
    
    // Decode length
    size_t length;
    size_t lengthBytes;
    if (!decodeLength(buffer + 1, length, lengthBytes)) {
        return false;
    }
    
    // Validate total size
    if (1 + lengthBytes + length > size || length > MAX_INT_LENGTH) {
        return false;
    }
    
    // Decode value
    value = 0;
    const uint8_t* valueStart = buffer + 1 + lengthBytes;
    bool isNegative = (valueStart[0] & 0x80) != 0;
    
    for (size_t i = 0; i < length; i++) {
        value = (value << 8) | valueStart[i];
    }
    
    bytesRead = 1 + lengthBytes + length;
    return true;
}

// OCTET STRING implementation
OctetString::OctetString(const char* str) : length(0) {
    setValue(str);
}

void OctetString::setValue(const char* str) {
    if (!str) {
        length = 0;
        data[0] = '\0';
        return;
    }
    
    length = strlen(str);
    if (length >= MAX_STRING_LENGTH) {
        length = MAX_STRING_LENGTH - 1;
    }
    
    memcpy(data, str, length);
    data[length] = '\0';
}

size_t OctetString::encode(uint8_t* buffer, size_t size) const {
    if (!buffer || size < 2) return 0;
    
    // Write tag
    buffer[0] = BER::encodeTag(TagClass::Universal, EncodingType::Primitive, 
                              static_cast<uint8_t>(UniversalTag::OctetString));
    
    // Write length
    size_t lengthBytes = encodeLength(buffer + 1, length);
    if (lengthBytes == 0) return 0;
    
    // Write string data
    if (1 + lengthBytes + length > size) return 0;
    memcpy(buffer + 1 + lengthBytes, data, length);
    
    return 1 + lengthBytes + length;
}

bool OctetString::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    if (!buffer || size < 2) return false;
    
    // Validate tag
    if (!BER::validateTag(buffer[0], TagClass::Universal, EncodingType::Primitive,
                         static_cast<uint8_t>(UniversalTag::OctetString))) {
        return false;
    }
    
    // Decode length
    size_t lengthBytes;
    if (!decodeLength(buffer + 1, length, lengthBytes)) {
        return false;
    }
    
    // Validate total size and string length
    if (1 + lengthBytes + length > size || length >= MAX_STRING_LENGTH) {
        return false;
    }
    
    // Copy string data
    memcpy(data, buffer + 1 + lengthBytes, length);
    data[length] = '\0';
    
    bytesRead = 1 + lengthBytes + length;
    return true;
}

// NULL implementation
size_t Null::encode(uint8_t* buffer, size_t size) const {
    if (!buffer || size < 2) return 0;
    
    buffer[0] = BER::encodeTag(TagClass::Universal, EncodingType::Primitive,
                              static_cast<uint8_t>(UniversalTag::Null));
    buffer[1] = 0; // NULL type always has zero length
    
    return 2;
}

bool Null::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    if (!buffer || size < 2) return false;
    
    // Validate tag and length
    if (!BER::validateTag(buffer[0], TagClass::Universal, EncodingType::Primitive,
                         static_cast<uint8_t>(UniversalTag::Null)) ||
        buffer[1] != 0) {
        return false;
    }
    
    bytesRead = 2;
    return true;
}

// OBJECT IDENTIFIER implementation
ObjectIdentifier::ObjectIdentifier(const uint32_t* components, size_t count) : numComponents(0) {
    setComponents(components, count);
}

bool ObjectIdentifier::setComponents(const uint32_t* newComponents, size_t count) {
    if (!newComponents || count == 0 || count > MAX_OID_LENGTH) {
        return false;
    }
    
    numComponents = count;
    memcpy(components, newComponents, count * sizeof(uint32_t));
    return true;
}

size_t ObjectIdentifier::encode(uint8_t* buffer, size_t size) const {
    if (!buffer || size < 2 || numComponents < 2) return 0;
    
    // Write tag
    buffer[0] = BER::encodeTag(TagClass::Universal, EncodingType::Primitive,
                              static_cast<uint8_t>(UniversalTag::ObjectID));
    
    // Calculate encoded size for first two components
    uint8_t encodedOID[MAX_OID_LENGTH * 5]; // Max 5 bytes per component
    size_t encodedSize = 0;
    
    // First two components are encoded as: first * 40 + second
    uint32_t firstTwo = components[0] * 40 + components[1];
    do {
        encodedOID[encodedSize++] = (firstTwo & 0x7F) | (firstTwo > 0x7F ? 0x80 : 0);
        firstTwo >>= 7;
    } while (firstTwo > 0);
    
    // Encode remaining components
    for (size_t i = 2; i < numComponents; i++) {
        uint32_t value = components[i];
        uint8_t temp[5];
        size_t tempSize = 0;
        
        do {
            temp[tempSize++] = (value & 0x7F) | (value > 0x7F ? 0x80 : 0);
            value >>= 7;
        } while (value > 0);
        
        // Copy in reverse order
        while (tempSize > 0) {
            encodedOID[encodedSize++] = temp[--tempSize];
        }
    }
    
    // Write length
    size_t lengthBytes = encodeLength(buffer + 1, encodedSize);
    if (lengthBytes == 0) return 0;
    
    // Write encoded OID
    if (1 + lengthBytes + encodedSize > size) return 0;
    memcpy(buffer + 1 + lengthBytes, encodedOID, encodedSize);
    
    return 1 + lengthBytes + encodedSize;
}

bool ObjectIdentifier::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    if (!buffer || size < 2) return false;
    
    // Validate tag
    if (!BER::validateTag(buffer[0], TagClass::Universal, EncodingType::Primitive,
                         static_cast<uint8_t>(UniversalTag::ObjectID))) {
        return false;
    }
    
    // Decode length
    size_t length;
    size_t lengthBytes;
    if (!decodeLength(buffer + 1, length, lengthBytes)) {
        return false;
    }
    
    // Validate total size
    if (1 + lengthBytes + length > size) {
        return false;
    }
    
    // Decode components
    const uint8_t* curr = buffer + 1 + lengthBytes;
    const uint8_t* end = curr + length;
    numComponents = 0;
    
    // Decode first value (contains first two components)
    uint32_t value = 0;
    while (curr < end && (*curr & 0x80)) {
        value = (value << 7) | (*curr++ & 0x7F);
    }
    if (curr >= end) return false;
    value = (value << 7) | *curr++;
    
    // Split into first two components
    components[numComponents++] = value / 40;
    components[numComponents++] = value % 40;
    
    // Decode remaining components
    while (curr < end && numComponents < MAX_OID_LENGTH) {
        value = 0;
        while (curr < end && (*curr & 0x80)) {
            value = (value << 7) | (*curr++ & 0x7F);
        }
        if (curr >= end) return false;
        value = (value << 7) | *curr++;
        components[numComponents++] = value;
    }
    
    bytesRead = 1 + lengthBytes + length;
    return true;
}

// SEQUENCE implementation
size_t Sequence::encode(uint8_t* buffer, size_t size) const {
    if (!buffer || size < 2) return 0;
    
    // Write tag
    buffer[0] = BER::encodeTag(TagClass::Universal, EncodingType::Constructed,
                              static_cast<uint8_t>(UniversalTag::Sequence));
    
    // Write length
    size_t lengthBytes = encodeLength(buffer + 1, length);
    if (lengthBytes == 0) return 0;
    
    // Write sequence data
    if (1 + lengthBytes + length > size) return 0;
    memcpy(buffer + 1 + lengthBytes, data, length);
    
    return 1 + lengthBytes + length;
}

bool Sequence::decode(const uint8_t* buffer, size_t size, size_t& bytesRead) {
    if (!buffer || size < 2) return false;
    
    // Validate tag
    if (!BER::validateTag(buffer[0], TagClass::Universal, EncodingType::Constructed,
                         static_cast<uint8_t>(UniversalTag::Sequence))) {
        return false;
    }
    
    // Decode length
    size_t lengthBytes;
    if (!decodeLength(buffer + 1, length, lengthBytes)) {
        return false;
    }
    
    // Validate total size and sequence length
    if (1 + lengthBytes + length > size || length > MAX_SEQUENCE_LENGTH) {
        return false;
    }
    
    // Copy sequence data
    memcpy(data, buffer + 1 + lengthBytes, length);
    
    bytesRead = 1 + lengthBytes + length;
    return true;
}

bool Sequence::addData(const uint8_t* newData, size_t dataLength) {
    if (!newData || length + dataLength > MAX_SEQUENCE_LENGTH) {
        return false;
    }
    
    memcpy(data + length, newData, dataLength);
    length += dataLength;
    return true;
}

// BER namespace implementation
namespace BER {

uint8_t encodeTag(TagClass tagClass, EncodingType encoding, uint8_t tagNumber) {
    return static_cast<uint8_t>(tagClass) | 
           static_cast<uint8_t>(encoding) | 
           (tagNumber & 0x1F);
}

bool decodeTag(uint8_t byte, TagClass& tagClass, EncodingType& encoding, uint8_t& tagNumber) {
    tagClass = static_cast<TagClass>(byte & 0xC0);
    encoding = static_cast<EncodingType>(byte & 0x20);
    tagNumber = byte & 0x1F;
    return true;
}

size_t encodeLength(uint8_t* buffer, size_t length) {
    if (!buffer) return 0;
    
    if (length < 128) {
        buffer[0] = length;
        return 1;
    }
    
    // Calculate number of bytes needed
    size_t numBytes = 0;
    size_t temp = length;
    while (temp > 0) {
        temp >>= 8;
        numBytes++;
    }
    
    // First byte indicates number of length bytes
    buffer[0] = 0x80 | numBytes;
    
    // Write length bytes in big-endian order
    for (size_t i = 0; i < numBytes; i++) {
        buffer[numBytes - i] = length & 0xFF;
        length >>= 8;
    }
    
    return numBytes + 1;
}

bool decodeLength(const uint8_t* buffer, size_t& length, size_t& bytesRead) {
    if (!buffer) return false;
    
    if (!(buffer[0] & 0x80)) {
        length = buffer[0];
        bytesRead = 1;
        return true;
    }
    
    size_t numBytes = buffer[0] & 0x7F;
    if (numBytes > sizeof(size_t)) return false;
    
    length = 0;
    for (size_t i = 0; i < numBytes; i++) {
        length = (length << 8) | buffer[i + 1];
    }
    
    bytesRead = numBytes + 1;
    return true;
}

bool validateTag(uint8_t byte, TagClass expectedClass, EncodingType expectedEncoding, uint8_t expectedTag) {
    TagClass tagClass;
    EncodingType encoding;
    uint8_t tagNumber;
    
    if (!decodeTag(byte, tagClass, encoding, tagNumber)) {
        return false;
    }
    
    return tagClass == expectedClass && 
           encoding == expectedEncoding && 
           tagNumber == expectedTag;
}

bool validateLength(size_t length, size_t maxAllowed) {
    return length <= maxAllowed;
}

} // namespace BER

} // namespace ASN1
