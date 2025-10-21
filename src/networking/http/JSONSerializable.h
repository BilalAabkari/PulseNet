#pragma once

#include <string>

class JSONSerializable
{
  public:
    virtual ~JSONSerializable() = default;

    virtual std::string serialize() const = 0;
};
