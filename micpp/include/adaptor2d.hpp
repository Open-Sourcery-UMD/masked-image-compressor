#ifndef MICPP_ADAPTOR2D_HPP
#define MICPP_ADAPTOR2D_HPP
#include <algorithm>

template <typename T>
concept FlatContainer = requires(T candidate) {
    // Can index
    candidate[0];

    // We have a size type
    typename T::size_type;

    // We have a value type
    typename T::value_type;

    // We have iterators
    typename T::iterator;
    typename T::const_iterator;
    typename T::reverse_iterator;
    typename T::const_reverse_iterator;

    candidate.begin();
    candidate.cbegin();
    candidate.end();
    candidate.cend();
    candidate.rbegin();
    candidate.crbegin();
    candidate.rend();
    candidate.crend();
};

// Utility class that stores a single long indexable type (array, vector, etc.) that's meant to be a 2d array.
// Adaptor2D provides convenient methods for accessing and manipulating the data.
template <typename T> requires FlatContainer<T>
class Adaptor2D {
    T m_container;
    T::size_type m_width;
    T::size_type m_height;

    using value_t = T::value_type;
    using size_t = T::size_type;
    using index_return_t = std::conditional_t<std::is_trivially_copyable_v<value_t>, value_t, const value_t&>;

public:
    Adaptor2D(T&& indexable, size_t width, size_t height) : m_container{ std::move(indexable) }, m_width{ width },
                                                            m_height{ height } {}

    // We get a regular copy if our value is trivially copyable and a reference if not
    index_return_t operator[](size_t x, size_t y) const {
        return m_container[y * m_width + x];
    }


    // Mutable reference to the value type if we are not a const object
    value_t& operator[](size_t x, size_t y) {
        return m_container[y * m_width + x];
    }

    // Getters
    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    T::iterator begin() { return m_container.begin(); }
    T::const_iterator cbegin() const { return m_container.cbegin(); }
    T::iterator end() { return m_container.end(); }
    T::const_iterator cend() const { return m_container.cend(); }
    T::iterator rbegin() { return m_container.rbegin(); }
    T::const_iterator crbegin() const { return m_container.crbegin(); }
    T::iterator rend() { return m_container.rend(); }
    T::const_iterator crend() const { return m_container.crend(); }

};

#endif //MICPP_ADAPTOR2D_HPP
