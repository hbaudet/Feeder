#pragma once

#include "Events/event.hpp"

class PlaySoundEvent : public Event {
    public:
        static bool isRegistered;

                    PlaySoundEvent(JsonObjectConst);
        void        overrideParam(int value, bool oneTime) override;
        int         getValue() const override;

    private:
        void        doStuff() override;
};
