#pragma once

#include "Events/event.hpp"

class PlayLightEvent : public Event {
    public:
        static bool isRegistered;

                    PlayLightEvent(JsonObjectConst);
        void        overrideParam(int value, bool oneTime) override;
        int         getValue() const override;

    private:
        void        doStuff() override;
};
