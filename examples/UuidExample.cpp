#include <iostream>

#include "educelab/core/types/Uuid.hpp"

using namespace educelab;

auto main() -> int
{
    // Null UUID
    Uuid uuid;
    std::cout << std::boolalpha;
    std::cout << "UUID: " << uuid;
    std::cout << ", Is Null: " << uuid.is_nil() << "\n";

    // Generate a UUID using the v4 generator
    uuid = Uuid::Uuid4();
    std::cout << "UUID: " << uuid;
    std::cout << ", Is Null: " << uuid.is_nil() << "\n";

    // To/From std::string
    auto uuidStr = uuid.string();
    std::cout << "uuid.string(): " << uuidStr << "\n";
    auto uuidCopy = Uuid::FromString(uuidStr);
    std::cout << "UUID::FromString(): " << uuidCopy << "\n";
    std::cout << "Matches original UUID: " << (uuid == uuidCopy) << "\n";

    // Reset UUID
    uuid.reset();
    std::cout << "Resetting UUID...\n";
    std::cout << "UUID: " << uuid;
    std::cout << ", Is Null: " << uuid.is_nil() << "\n";
}