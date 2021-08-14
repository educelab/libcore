#include <iostream>
#include <string>

#include "educelab/core/types/Signals.hpp"

inline void NoParameter() { std::cout << "\tHello, World!" << std::endl; }

inline void SingleParameter(int i) { std::cout << "\t" << i << std::endl; }

inline void FloatParameter(float f) { std::cout << "\t" << f << std::endl; }

inline void MultiParameter(int i, float f, const std::string& s)
{
    std::cout << "\t" << i << " " << f << " " << s << std::endl;
}

using namespace educelab;

auto main() -> int
{
    // No parameter
    Signal<> event;
    event.connect(NoParameter);
    std::cout << "No parameter signal:\n";
    event.send();

    // Single parameter
    Signal<int> oneParam;
    oneParam.connect(SingleParameter);
    std::cout << "Single parameter signal:\n";
    oneParam.send(1);

    // Implicit conversion
    // Compiler will still warn about this
    Signal<float> floatParam;
    floatParam.connect(FloatParameter);
    floatParam.connect(SingleParameter);
    std::cout << "Implicit value conversion (sending 1.5):\n";
    floatParam.send(1.5);

    // Multiple parameters
    Signal<int, float, std::string> multiParam;
    multiParam.connect(MultiParameter);
    std::cout << "Multiple parameter signals:\n";
    multiParam.send(1, 2.0, "3");

    // Clear all connections
    oneParam.disconnect();
    floatParam.disconnect();
    multiParam.disconnect();

    // Any signal can connect to a no parameter function
    oneParam.connect(NoParameter);
    floatParam.connect(NoParameter);
    multiParam.connect(NoParameter);

    std::cout << "Any signal can connect to a no parameter function:\n";
    oneParam.send(0);
    floatParam.send(0);
    multiParam.send(0, 0, "");
}