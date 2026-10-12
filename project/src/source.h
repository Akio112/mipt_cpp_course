#ifndef NANO_EDR_SOURCE_H
#define NANO_EDR_SOURCE_H

namespace nano_edr {

class Agent;

class Source {
 public:
    virtual ~Source();
    virtual void Run(Agent* agent) = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_SOURCE_H
