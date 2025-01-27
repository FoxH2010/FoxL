#include "interpreter.h"
#include <iostream>
#include <stdexcept>

// Utility function to print a Value
void Interpreter::printValue(const Value &value) const {
    if (std::holds_alternative<int>(value)) {
        std::cout << std::get<int>(value);
    } else if (std::holds_alternative<double>(value)) {
        std::cout << std::get<double>(value);
    } else if (std::holds_alternative<std::string>(value)) {
        std::cout << std::get<std::string>(value);
    } else if (std::holds_alternative<bool>(value)) {
        std::cout << (std::get<bool>(value) ? "true" : "false");
    } else if (std::holds_alternative<std::vector<Value>>(value)) {
        std::cout << "[ ";
        const auto &elements = std::get<std::vector<Value>>(value);
        for (size_t i = 0; i < elements.size(); ++i) {
            printValue(elements[i]);
            if (i < elements.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << " ]";
    } else {
        std::cout << "[Unsupported value type]";
    }
}

// Utility function to check if a Value is a number
bool Interpreter::isNumber(const Value &value) const {
    return std::holds_alternative<int>(value) || std::holds_alternative<double>(value);
}

// Utility function to check if a Value is a double
bool Interpreter::isDouble(const Value &value) const {
    return std::holds_alternative<double>(value);
}

// Utility function to convert a Value to a double
// Throws an error if the Value is not a number
double Interpreter::toDouble(const Value &value) const {
    if (std::holds_alternative<int>(value)) {
        return static_cast<double>(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    throw std::runtime_error("Value is not a number.");
}