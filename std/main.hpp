template <typename T>
class List {
private:
    size_t lsize = 0;
    T* array = nullptr;

public:
    List() = default;

    List(const List<T>& other) {
        resize(other.lsize);
        for (size_t i = 0; i < lsize; i++) {
            array[i] = other.array[i];
        }
    }

    ~List() { 
        delete[] array; 
    }

    T& operator[](const size_t& index) {
        if (index >= lsize) { 
            std::cerr << "Out of bounds.\n"; 
        }
        return array[index];
    }

    const T& operator[](const size_t& index) const {
        if (index >= lsize) { 
            std::cerr << "Out of bounds.\n"; 
        }
        return array[index];
    }

    void resize(const size_t& nsize) {
        if (nsize == lsize) return;

        T* new_array = nullptr;
        if (nsize > 0) {
            new_array = new T[nsize];
            size_t elements_to_copy = (nsize < lsize) ? nsize : lsize;
            for (size_t i = 0; i < elements_to_copy; i++) {
                new_array[i] = array[i];
            }
        }

        delete[] array;
        array = new_array;
        lsize = nsize;
    }

    List<T>& operator=(const List<T>& other) {
        if (this == &other) return *this; // self-assignment guard

        resize(other.lsize);
        for (size_t i = 0; i < other.lsize; i++) {
            this->array[i] = other.array[i];
        }
        return *this;
    }

    List<T> operator+(const T& val) const {
        List<T> out = *this;
        out.append(val);
        return out;
    }

    List<T> operator+(const List<T>& other) const {
        List<T> out;
        out.resize(this->lsize + other.lsize);
        
        for (size_t i = 0; i < this->lsize; i++) {
            out.array[i] = this->array[i];
        }
        for (size_t i = 0; i < other.lsize; i++) {
            out.array[this->lsize + i] = other.array[i];
        }
        return out;
    }

    void append(T val) {
        size_t old_size = lsize;
        resize(lsize + 1);
        array[old_size] = val;
    }

    constexpr size_t size() const {
        return lsize;
    }

    #ifndef __PITI_N_RANGELOOPS
    class Iterator {
    private:
        T* m_ptr;

    public:
        Iterator(T* ptr) : m_ptr(ptr) {}

        T& operator*() const { return *m_ptr; }
        T* operator->() { return m_ptr; }

        Iterator& operator++() {
            m_ptr++;
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            m_ptr++;
            return temp;
        }

        bool operator==(const Iterator& other) const { return m_ptr == other.m_ptr; }
        bool operator!=(const Iterator& other) const { return m_ptr != other.m_ptr; }
    };

    Iterator begin() { return Iterator(array); }
    Iterator end() { return Iterator(array + lsize); }
    
    class ConstIterator {
    private:
        const T* m_ptr;
    public:
        ConstIterator(const T* ptr) : m_ptr(ptr) {}
        const T& operator*() const { return *m_ptr; }
        ConstIterator& operator++() { m_ptr++; return *this; }
        bool operator!=(const ConstIterator& other) const { return m_ptr != other.m_ptr; }
    };
    ConstIterator begin() const { return ConstIterator(array); }
    ConstIterator end() const { return ConstIterator(array + lsize); }
    #endif
};