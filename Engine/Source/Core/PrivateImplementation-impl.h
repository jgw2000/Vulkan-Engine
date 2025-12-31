#pragma once

#include "PrivateImplementation.h"

#include <utility>

namespace VE {

template <typename T>
PrivateImplementation<T>::PrivateImplementation() noexcept
    : impl(new T) {
}

template <typename T>
template <typename ... ARGS>
PrivateImplementation<T>::PrivateImplementation(ARGS&& ... args) noexcept
    : impl(new T(std::forward<ARGS>(args)...)) {
}

template <typename T>
PrivateImplementation<T>::~PrivateImplementation() noexcept {
    delete impl;
}

#ifndef PRIVATE_IMPLEMENTATION_NON_COPYABLE

template <typename T>
PrivateImplementation<T>::PrivateImplementation(const PrivateImplementation& rhs) noexcept
    : impl(new T(*rhs.impl)) {
}

template <typename T>
PrivateImplementation<T>& PrivateImplementation<T>::operator=(const PrivateImplementation<T>& rhs) noexcept {
    if (this != &rhs) {
        *impl = *rhs.impl;
    }
    return *this;
}

#endif
} // namespace VE