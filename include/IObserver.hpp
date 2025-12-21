#pragma once

#include <unordered_set>
#include "debug.hpp"

template<class T>
class IObserver {
    public:
        ~IObserver() = default;
        virtual void    notify(const T &event, uint16_t value) = 0;
};

template<class T>
class Observable {
    public:
        virtual ~Observable() = default;
        bool    addSubscriber(IObserver<T> *sub, const std::string &label) {
            ESP_LOGD("observable", "%s", label.c_str());
            return observers.emplace(sub).second;
        }
        void    removeSubscriber(IObserver<T> *sub) {
            observers.erase(sub);
        }

    protected:
        std::unordered_set<IObserver<T>*> observers;
};
