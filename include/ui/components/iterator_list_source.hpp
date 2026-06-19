#pragma once

#include <cstddef>
#include <vector>

namespace lunar3d {
namespace ui {
namespace components {

template <typename T>
class IteratorListSource {
public:
    IteratorListSource();

    template <typename Iterator>
    IteratorListSource(Iterator first, Iterator last);

    template <typename Iterator>
    void setItems(Iterator first, Iterator last);

    size_t getCount() const;
    const T* getItem(size_t position) const;

private:
    std::vector<const T*> items_;
};

template <typename T>
IteratorListSource<T>::IteratorListSource() {}

template <typename T>
template <typename Iterator>
IteratorListSource<T>::IteratorListSource(Iterator first, Iterator last) {
    setItems(first, last);
}

template <typename T>
template <typename Iterator>
void IteratorListSource<T>::setItems(Iterator first, Iterator last) {
    items_.clear();
    for (Iterator current = first; current != last; ++current) {
        items_.push_back(&*current);
    }
}

template <typename T>
size_t IteratorListSource<T>::getCount() const {
    return items_.size();
}

template <typename T>
const T* IteratorListSource<T>::getItem(size_t position) const {
    return position < items_.size() ? items_[position] : nullptr;
}

} // namespace components
} // namespace ui
} // namespace lunar3d
