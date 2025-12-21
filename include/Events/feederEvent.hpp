#pragma once

#include "Events/event.hpp"

class FeederEvent : public Event {
    public:
                    FeederEvent(JsonObjectConst);
        void        overrideParam(int value, bool oneTime) override;
        int         getValue() const override;

    private:
        static bool isRegistered;

        void        doStuff() override;
        int         doses;
};
