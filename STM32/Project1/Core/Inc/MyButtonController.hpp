#ifndef MYBUTTONCONTROLLER_HPP_
#define MYBUTTONCONTROLLER_HPP_

#include <platform/driver/button/ButtonController.hpp>

class MyButtonController : public touchgfx::ButtonController
{
public:
    virtual void init();
    virtual bool sample(uint8_t& key);
private:
    uint8_t previousState;
};

#endif
