#pragma once

#include <cassert>

namespace VE::Core {

template <typename T>
class Singleton {
public:
    Singleton() {
        assert(!mSingleton);
        mSingleton = static_cast<T*>(this);
    }

    ~Singleton() {
        assert(mSingleton);
        mSingleton = nullptr;
    }

    static T& GetSingleton() {
        assert(mSingleton);
        return *mSingleton;
    }

    static T* GetSingletonPtr() {
        return mSingleton;
    }

protected:
    static T* mSingleton;

private:
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;
};

}