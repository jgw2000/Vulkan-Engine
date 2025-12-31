#pragma once

namespace VE {

/**
 * PrivateImplementation is used to hide the implementation details of a class and ensure a higher
 * level of backward binary compatibility.
 */
template <typename T>
class PrivateImplementation {
public:
    template <typename ... ARGS>
    explicit PrivateImplementation(ARGS&& ...) noexcept;
    PrivateImplementation() noexcept;
    ~PrivateImplementation() noexcept;
    PrivateImplementation(const PrivateImplementation& rhs) noexcept;
    PrivateImplementation& operator=(const PrivateImplementation& rhs) noexcept;

    PrivateImplementation(PrivateImplementation&& rhs) noexcept : impl(rhs.impl) { rhs.impl = nullptr; }
    PrivateImplementation& operator=(PrivateImplementation&& rhs) noexcept {
        auto temp = impl;
        impl = rhs.impl;
        rhs.impl = temp;
        return *this;
    }

protected:
    T* impl = nullptr;
    inline T* operator->() noexcept { return impl; }
    inline const T* operator->() const noexcept { return impl; }
};

} // namespace VE