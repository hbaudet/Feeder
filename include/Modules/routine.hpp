#pragma once

#include <time.h>
#include <ArduinoJson.h>
#include <vector>

#include "Events/event.hpp"
#include "localSetup.hpp"
#include "magic_enum/magic_enum.hpp"
#include "Modules/outputManager.hpp"
#include "debug.hpp"

class Routine : public IObserver<OutputEvent>, public Observable<OutputEvent> {
    public:
                                Routine() = delete;
        virtual                 ~Routine();
                                Routine(const JsonDocument&);
        int                     update();
        void                    reset();
        Event                   *nextEvent(const std::string &type);
        void                    notify(const OutputEvent &, uint16_t) override;
        void                    run();

        // TODO :
        void                    updateSchedule(const JsonDocument &);
        const JsonDocument      &getSchedule()const;

    private:
        int                     msBeforeNext;
        std::vector<Event*>     events;
        static void             routineTask(void* param);
};
