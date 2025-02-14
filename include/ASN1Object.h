#ifndef ASN1_OBJECT_H
#define ASN1_OBJECT_H

#include "ASN1Types.h"

// ASN1Object inherits from ASN1::Type to allow type conversion
class ASN1Object : public ASN1::Type {
public:
    virtual ~ASN1Object() = default;
    
    // Implement pure virtual methods from ASN1::Type
    size_t encode(uint8_t* buffer, size_t size) const override { return 0; }
    bool decode(const uint8_t* buffer, size_t size, size_t& bytesRead) override { return false; }
    ASN1::UniversalTag getTag() const override { return ASN1::UniversalTag::Null; }
};

#endif // ASN1_OBJECT_H
