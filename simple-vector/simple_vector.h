#pragma once

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <initializer_list>

#include "array_ptr.h"

class ReserveProxyObj {
    size_t capacity_to_reserve_ = 0;
public:
    ReserveProxyObj(size_t capacity_to_reserve) :capacity_to_reserve_(capacity_to_reserve) {
    }
    size_t GetCapacity() {
        return capacity_to_reserve_;
    }
};
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // ������ ������ �� size ���������, ������������������ ��������� �� ���������
    explicit SimpleVector(size_t size) : capacity_(size), size_(size), simple_vector_ptr_(size) {
        std::fill(begin(), end(), 0);
    }

    // ������ ������ �� size ���������, ������������������ ��������� value
    SimpleVector(size_t size, const Type& value) : capacity_(size), size_(size), simple_vector_ptr_(size) {
        std::fill(begin(), end(), value);
    }

    // ������ ������ �� std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : capacity_(init.size()), size_(init.size()), simple_vector_ptr_(init.size()) {
        int i = 0;
        for (auto it = init.begin(); it != init.end(); ++it) {
            simple_vector_ptr_[i] = *it;
            ++i;
        }
    }

    SimpleVector(const SimpleVector& other) {
        if (this != &other) {
            SimpleVector<Type> copy(other.size_);
            std::copy(other.begin(), other.end(), copy.begin());
            swap(copy);
        }
    }

    SimpleVector(ReserveProxyObj new_capacity) {
        Reserve(new_capacity.GetCapacity());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector copy(rhs);
            swap(copy);
        }
        return *this;
    }
    ~SimpleVector() {}
    //----------M-O-V-E---------------------
    //----------M-O-V-E---------------------
    //----------M-O-V-E---------------------

    SimpleVector(SimpleVector&& other) {
        if (this != &other) {
        SimpleVector<Type> copy(other.size_);
        std::move(other.begin(), other.end(), copy.begin());
        swap(copy);
        other.size_ = other.capacity_ = 0;
        }
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            SimpleVector copy(std::move(rhs));
            swap(copy);
        }
        return *this;
    }
    //----------E-N-D--M-O-V-E---------------------

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            SimpleVector<Type> temp(new_capacity);
            std::move(begin(), end(), temp.begin());
            temp.size_ = size_;
            swap(temp);
        }
    }

    // ��������� ������� � ����� �������
    // ��� �������� ����� ����������� ����� ����������� �������
    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            simple_vector_ptr_[size_] = item;
        }
        else {
            size_t new_capacity = NewCapacity();
            ArrayPtr<Type> arr_copy(new_capacity);
            std::copy(begin(), end(), arr_copy.Get());
            arr_copy[size_] = item;
            arr_copy.swap(simple_vector_ptr_);
            capacity_ = new_capacity;
        }
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            simple_vector_ptr_[size_] = std::move(item);
        }
        else {
            size_t new_capacity = NewCapacity();
            ArrayPtr<Type> arr_copy(new_capacity);
            std::move(begin(), end(), arr_copy.Get());
            arr_copy[size_] = std::move(item);
            arr_copy.swap(simple_vector_ptr_);
            capacity_ = new_capacity;
        }
        ++size_;
    }

    // ��������� �������� value � ������� pos.
    // ���������� �������� �� ����������� ��������
    // ���� ����� �������� �������� ������ ��� �������� ���������,
    // ����������� ������� ������ ����������� �����, � ��� ������� ������������ 0 ����� ������ 1

    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= cbegin() && pos <= cend());
        auto pos_element = std::distance(cbegin(), pos);
        if (size_ < capacity_) {
            std::copy_backward(pos, cend(), begin() + size_ + 1);
            simple_vector_ptr_[pos_element] = value;
        }
        else {
            size_t new_capacity = NewCapacity();
            ArrayPtr<Type> arr_ptr(new_capacity);
            std::copy(cbegin(), cend(), arr_ptr.Get());
            std::copy_backward(pos, cend(), begin() + size_ + 1);
            arr_ptr[pos_element] = value;
            simple_vector_ptr_.swap(arr_ptr);
            capacity_ = new_capacity;
        }
        ++size_;
        return Iterator{ &simple_vector_ptr_[pos_element] };
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= cbegin() && pos <= cend());
        auto pos_element = std::distance(cbegin(), pos);
        if (size_ < capacity_) {
            std::move_backward(begin() + pos_element, end(), begin() + size_ + 1);
            simple_vector_ptr_[pos_element] = std::move(value);
        }
        else {
            auto new_capacity = std::max(size_t(1), 2 * capacity_);
            ArrayPtr<Type> arr_ptr(new_capacity);
            std::move(begin(), begin() + pos_element, arr_ptr.Get());
            std::move_backward(begin() + pos_element, end(), arr_ptr.Get() + size_ + 1);
            arr_ptr[pos_element] = std::move(value);
            simple_vector_ptr_.swap(arr_ptr);
            capacity_ = new_capacity;
        }
        ++size_;
        return Iterator{ &simple_vector_ptr_[pos_element] };
    }
    
    // "�������" ��������� ������� �������. ������ �� ������ ���� ������
    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
        }
    }

    // ������� ������� ������� � ��������� �������
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        std::move(const_cast<Iterator>(pos) + 1, end(), const_cast<Iterator>(pos));
        --size_;
        return const_cast<Iterator>(pos);
    }

    // ���������� �������� � ������ ��������
    void swap(SimpleVector& other) noexcept {
        std::swap(other.capacity_, capacity_);
        std::swap(other.size_, size_);
        other.simple_vector_ptr_.swap(simple_vector_ptr_);
    }

    // ���������� ���������� ��������� � �������
    size_t GetSize() const noexcept {
        return size_;
    }

    // ���������� ����������� �������
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // ��������, ������ �� ������
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // ���������� ������ �� ������� � �������� index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return simple_vector_ptr_[index];
    }

    // ���������� ����������� ������ �� ������� � �������� index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return simple_vector_ptr_[index];
    }

    // ���������� ����������� ������ �� ������� � �������� index
    // ����������� ���������� std::out_of_range, ���� index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("index >= size");
        }
        return simple_vector_ptr_[index];
    }

    // ���������� ����������� ������ �� ������� � �������� index
    // ����������� ���������� std::out_of_range, ���� index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index >= size");
        }
        return simple_vector_ptr_[index];
    }

    // �������� ������ �������, �� ������� ��� �����������
    void Clear() noexcept {
        size_ = 0;
    }

    // �������� ������ �������.
    // ��� ���������� ������� ����� �������� �������� �������� �� ��������� ��� ���� Type
    void Resize(size_t new_size) {
        auto i = size_;
        if (new_size > capacity_) {
            SimpleVector copy(new_size);
            std::move(begin(), end(), copy.begin());
            while (i != new_size) {
                copy[i] = std::move(Type());
                ++i;
            }
            size_ = new_size;
            capacity_ = new_size;
            swap(copy);
        }
        else if (new_size < size_) {
            size_ = new_size;
        }
        else {
            while (i != new_size) {
                simple_vector_ptr_[i] = std::move(Type());
                ++i;
            }
            size_ = new_size;
        }
    }

    // ���������� �������� �� ������ �������
    // ��� ������� ������� ����� ���� ����� (��� �� �����) nullptr
    Iterator begin() noexcept {
        return &simple_vector_ptr_[0];
    }

    // ���������� �������� �� �������, ��������� �� ���������
    // ��� ������� ������� ����� ���� ����� (��� �� �����) nullptr
    Iterator end() noexcept {
        return &simple_vector_ptr_[size_];
    }

    // ���������� ����������� �������� �� ������ �������
    // ��� ������� ������� ����� ���� ����� (��� �� �����) nullptr
    ConstIterator begin() const noexcept {
        return &simple_vector_ptr_[0];
    }

    // ���������� �������� �� �������, ��������� �� ���������
    // ��� ������� ������� ����� ���� ����� (��� �� �����) nullptr
    ConstIterator end() const noexcept {
        return &simple_vector_ptr_[size_];
    }

    // ���������� ����������� �������� �� ������ �������
    // ��� ������� ������� ����� ���� ����� (��� �� �����) nullptr
    ConstIterator cbegin() const noexcept {
        return &simple_vector_ptr_[0];
    }

    // ���������� �������� �� �������, ��������� �� ���������
    // ��� ������� ������� ����� ���� ����� (��� �� �����) nullptr
    ConstIterator cend() const noexcept {
        return &simple_vector_ptr_[size_];
    }
private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    ArrayPtr<Type> simple_vector_ptr_;

    size_t NewCapacity() {
        return capacity_ == 0 ? 1 : 2 * capacity_;
    }

};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() == rhs.GetSize()) {
        for (size_t i = 0; i != lhs.GetSize(); ++i) {
            if (lhs[i] != rhs[i]) {
                return false;
            }
        }
    }
    return true;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() < rhs.GetSize()) {
        return true;
    }
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend()) == true;
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}