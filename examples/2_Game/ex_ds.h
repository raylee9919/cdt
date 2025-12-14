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
    Array();
    ~Array();


    void push(T val);
    void clear();
    bool empty();

    T &operator[](u64 idx);
    const T& operator[](u64 idx) const;


    T *data = 0;
    int count = 0;
    int cap = 0;
};

template<typename T>
struct Stack {
    Stack();
    ~Stack();

    void push(T val);
    T pop();
    int count();
    bool empty();
    void clear();

    T data[256];
    int top = 0;
};

template<typename T>
struct Queue {
    Queue();
    ~Queue();

    void push(T val);
    T pop();
    int count();
    bool empty();
    void clear();

    T data[256];
    int front = 0;
    int back  = 0;
};
