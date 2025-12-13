/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

struct Index_Dist {
    int index;
    f32 dist;
};

struct Priority_Queue {
    // @Temporary
    Index_Dist items[4096];
    int size;
};

template<typename T>
struct Array {
    void push(T val);
    void clear();
    bool empty();

    T &operator[](u64 idx);
    const T& operator[](u64 idx) const;

    T *data;
    u64 count;
    u64 cap;
};

template<typename T>
struct Stack {
    T data[256];
    int top;
};
