#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <utility>

//default_delete
template <typename T>
struct default_delete {
    void operator()(T* ptr) const noexcept {
        static_assert(sizeof(T) > 0, "Can't delete incomplete type");
        delete ptr;
    }
};
template <typename T>
struct default_delete<T[]> {
    void operator()(T* ptr) const noexcept {
        static_assert(sizeof(T) > 0, "Can't delete incomplete type");
        delete[] ptr;
    }
};


//unique_ptr
template <typename T, typename Deleter = default_delete<T>>
class MyUniquePtr;

//partial specialization for array pointers
template<typename T, typename Deleter>
class MyUniquePtr<T[], Deleter>
{
private:
    T* ptr;
    Deleter deleter;
public:
    //constructor and destructor
    constexpr explicit MyUniquePtr(T* ptr = nullptr) noexcept : ptr(ptr) {};
    constexpr explicit MyUniquePtr(T* ptr, Deleter deleter) noexcept : ptr(ptr), deleter(deleter) {};

    MyUniquePtr(const MyUniquePtr& othe) = delete;
    MyUniquePtr& operator=(const MyUniquePtr& other) = delete;

    constexpr MyUniquePtr(MyUniquePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    constexpr MyUniquePtr& operator=(MyUniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.ptr);
            other.ptr = nullptr;
        }
        return *this;
    }

    ~MyUniquePtr() {
        reset();
    }

    //operators and data access methods
    T& operator[](size_t index) const noexcept {
        return ptr[index];
    }
    T* get() const {
        return ptr;
    }
    Deleter* getDeleter() const noexcept {
        return deleter;
    }

    //methods for resource management
    constexpr T* release() noexcept {
        T* releasedPtr = ptr;
        ptr = nullptr;
        return releasedPtr;
    }
    constexpr void reset(T* newPtr = nullptr) noexcept {
        if (ptr != newPtr) {
            deleter(ptr);
            ptr = newPtr;
        }
    }
    constexpr void swap(MyUniquePtr& other) noexcept {
        std::swap(ptr, other.ptr);
    }

    //bool overload
    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};
//partial specialization for ordinary pointers
template<typename T, typename Deleter>
class MyUniquePtr
{
private:
    T* ptr;
    Deleter deleter;
public:
    //constructor and destructor

    constexpr explicit MyUniquePtr(T* ptr = nullptr) noexcept : ptr(ptr) {};
    constexpr explicit MyUniquePtr(T* ptr, Deleter deleter) noexcept : ptr(ptr), deleter(deleter) {};

    MyUniquePtr(const MyUniquePtr& othe) = delete;
    MyUniquePtr& operator=(const MyUniquePtr& other) = delete;

    constexpr MyUniquePtr(MyUniquePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    constexpr MyUniquePtr& operator=(MyUniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.ptr);
            other.ptr = nullptr;
        }
        return *this;
    }

    ~MyUniquePtr() {
        reset();
    }

    //operators and data access methods
    T& operator*() const noexcept {
        return *ptr;
    }
    T* operator->() const noexcept {
        return ptr;
    }
    T* get() const {
        return ptr;
    }
    Deleter* getDeleter() const noexcept {
        return deleter;
    }

    //methods for resource management
    constexpr T* release() noexcept {
        T* releasedPtr = ptr;
        ptr = nullptr;
        return releasedPtr;
    }
    constexpr void reset(T* newPtr = nullptr) noexcept {
        if (ptr != newPtr) {
            deleter(ptr);
            ptr = newPtr;
        }
    }
    constexpr void swap(MyUniquePtr& other) noexcept {
        std::swap(ptr, other.ptr);
    }

    //bool overload
    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};

namespace detail
{
    template<class>
    constexpr bool is_unbounded_array_v = false;
    template<class T>
    constexpr bool is_unbounded_array_v<T[]> = true;

    template<class>
    constexpr bool is_bounded_array_v = false;
    template<class T, std::size_t N>
    constexpr bool is_bounded_array_v<T[N]> = true;
} // namespace detail

//make_unique
template<class T, class... Args>
std::enable_if_t<!std::is_array<T>::value, MyUniquePtr<T>>
make_my_unique(Args&&... args)
{
    return MyUniquePtr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
std::enable_if_t<detail::is_unbounded_array_v<T>, MyUniquePtr<T>>
make_my_unique(std::size_t n)
{
    return MyUniquePtr<T>(new std::remove_extent_t<T>[n]());
}


//shared_ptr control block
template<typename T, typename Deleter = default_delete<T>>
class ControlBlock
{
private:
    size_t strong_ref;      //strong ref count
    size_t weak_ref;        //weak ref count
    Deleter deleter;
public:
    constexpr explicit ControlBlock() noexcept : strong_ref(1), weak_ref(0) {};
    constexpr explicit ControlBlock(Deleter* deleter) noexcept : strong_ref(1), weak_ref(0), deleter(deleter) {};

    ControlBlock(const ControlBlock& other) {};
    ControlBlock& operator=(const ControlBlock& other) = delete;
    ControlBlock(ControlBlock&& other) = delete;
    ControlBlock& operator=(ControlBlock&& other) = delete;

    void incrementStrongRef() {
        strong_ref++;
    }
    void decrementStrongRef() {
        strong_ref--;
        if (strong_ref == 0 && weak_ref == 0) {
            delete this;
        }
    }

    void incrementWeakRef() {
        weak_ref++;
    }
    void decrementWeakRef() {
        weak_ref--;
        if (strong_ref == 0 && weak_ref == 0) {
            delete this;
        }
    }

    size_t getStrongRef() noexcept { return strong_ref; };
    size_t getWeakRef() noexcept { return weak_ref; };

    Deleter getDeleter() noexcept { return deleter; };
};


//shared_ptr
template<typename T, typename Deleter = default_delete<T>>
class MySharedPtr
{
private:
    ControlBlock<T, Deleter>* cb;
    T* ptr;
public:
    //constructor and destructor
    constexpr MySharedPtr() : ptr(nullptr), cb(nullptr) {};
    constexpr explicit MySharedPtr(T* ptr) noexcept : ptr(ptr), cb(new ControlBlock<T, Deleter>()) { };

    template<typename T, typename Deleter>
    constexpr MySharedPtr(T* ptr, Deleter del) noexcept : ptr(ptr), cb(ControlBlock(del)) {};

    constexpr MySharedPtr(const MySharedPtr& other) noexcept : ptr(other.ptr), cb(other.cb) {
        cb->incrementStrongRef();
    }
    constexpr MySharedPtr& operator=(const MySharedPtr& other) noexcept {
        if (this != &other)
        {
            ptr = other.ptr;
            cb = other.cb;
            cb->incrementStrongRef();
        }
        return*this;
    }
    constexpr MySharedPtr(MySharedPtr&& other) noexcept : ptr(other.ptr), cb(other.cb) {
        other.ptr = nullptr;
        other.cb = nullptr;
    }
    constexpr MySharedPtr& operator=(MySharedPtr&& other) noexcept {
        if (this != &other)
        {
            ptr = other.ptr;
            cb = other.cb;
            other.ptr = nullptr;
            other.cb = nullptr;
        }
        return *this;
    }

    ~MySharedPtr() {
        reset();
    }

    //operator and data access methods
    T& operator*() const noexcept {
        return *ptr;
    }
    T* operator->() const noexcept {
        return ptr;
    }
    T* get() const {
        return ptr;
    }
    ControlBlock<T, Deleter>* getCB() const noexcept {
        return cb;
    }
    Deleter getDeleter() noexcept {
        return cb->getDeleter();
    }
    size_t use_count() const noexcept {
        return  cb ? cb->getStrongRef() : 0;
    }
    //other methods
    template< class Y >
    bool owner_before(const MySharedPtr<Y>& other) const noexcept {
        return this->cb->getStrongRef() < other.cb->getStrongRef() ? true : false;
    }
    bool unique() const noexcept {
        return this->cb->getStrongRef() == 1 ? true : false;
    }
    constexpr void reset(T* newPtr = nullptr) noexcept {
        if (cb) {
            if (use_count() - 1 == 0 || cb == nullptr)
                cb->getDeleter()(ptr);
            cb->decrementStrongRef();
        }
    }
    explicit operator bool() const noexcept {
        return get() != nullptr ? true : false;
    }
};

//weak_ptr
template<typename T, typename Deleter = default_delete<T>>
class MyWeakPtr
{
private:
    T* ptr;
    ControlBlock<T, Deleter>* cb;
public:
    MyWeakPtr() : ptr(nullptr), cb(nullptr) {};
    explicit MyWeakPtr(const MySharedPtr<T>& shared_ptr) noexcept : ptr(shared_ptr.get()), cb(shared_ptr.getCB()) {
        cb->incrementWeakRef();
    };
    MyWeakPtr(const MyWeakPtr& other) : ptr(other.ptr), cb(other.cb) {
        cb->incrementWeakRef();
    };
    MyWeakPtr& operator=(const MyWeakPtr& other) {
        if (this != &other) {
            cb->incrementWeakRef();
            ptr = other.ptr;
            cb = other.cb;
        }
        return *this;
    }
    MyWeakPtr& operator=(const MySharedPtr<T>& shared_ptr) noexcept {
        if (shared_ptr.get() == nullptr) {
            reset();
        }
        else {
            ptr = shared_ptr.get();
            cb = shared_ptr.getCB();
            cb->incrementWeakRef();
        }
        return *this;
    }

    MyWeakPtr(MyWeakPtr&& other) = delete;
    MyWeakPtr& operator=(MyWeakPtr&& other) = delete;
    ~MyWeakPtr() {
        reset();
    }

    operator bool() const {
        return !expired();
    }
    bool expired() const {
        return cb->getStrongRef() == 0;
    }
    MySharedPtr<T> lock() const {
        return expired() ? MySharedPtr<T>() : MySharedPtr<T>(*this);
    }
    size_t use_count() const noexcept {
        return  cb ? cb->getStrongRef() : 0;
    }
    void reset() {
        ptr = nullptr;
        cb->decrementWeakRef();
        cb = nullptr;
    }
};

#endif