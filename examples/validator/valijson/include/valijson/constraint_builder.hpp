#pragma once
#ifndef __VALIJSON_CONSTRAINT_BUILDER_HPP
#define __VALIJSON_CONSTRAINT_BUILDER_HPP

namespace valijson {

namespace adapters {
    class Adapter;
}

namespace constraints {
    struct Constraint;
}

class ConstraintBuilder
{
public:
    virtual ~ConstraintBuilder() {}

    virtual constraints::Constraint * make(const adapters::Adapter &) const = 0;
};

}  // namespace valijson

#endif
