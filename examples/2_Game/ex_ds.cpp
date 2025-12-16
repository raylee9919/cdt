/* ========================================================================

                                                            

                                                                    
                                                                        
                                         

   ======================================================================== */

void swap(Index_Dist *a, Index_Dist *b) {
    Index_Dist tmp = *a;
    *a = *b;
    *b = tmp;
}

void heapifyUp(Priority_Queue *pq, int index) {
    if (index && pq->items[(index - 1) / 2].dist > pq->items[index].dist) {
        swap(&pq->items[(index - 1) / 2], &pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
}

void heapifyDown(Priority_Queue *pq, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < pq->size && pq->items[left].dist < pq->items[smallest].dist) {
        smallest = left;
    }

    if (right < pq->size && pq->items[right].dist < pq->items[smallest].dist) {
        smallest = right;
    }

    if (smallest != index) {
        swap(&pq->items[index], &pq->items[smallest]);
        heapifyDown(pq, smallest);
    }
}

// Priority_Queue
//
void Priority_Queue::push(Index_Dist value) {
    if (size < arrcnt(items)) {
        items[size++] = value;
        heapifyUp(this, size - 1);
    } else {
        assert(!"");
    }
}

Index_Dist Priority_Queue::pop() {
    Index_Dist dummy = {};
    if (size > 0) {
        Index_Dist item = items[0];
        items[0] = items[--size];
        heapifyDown(this, 0);
        return item;
    }
    assert(!"X");
    return dummy;
}

// Array
//
template<typename T>
Array<T>::Array() {
}

template<typename T>
Array<T>::~Array() {
    free(data);
}

template<typename T>
void Array<T>::push(T val) {
    if (count == cap) {
        int new_cap = 4;
        if (cap == 0) {
            data = (T *)malloc(new_cap*sizeof(T));
        } else {
            new_cap = cap + (cap>>1);
            T *new_data = (T *)realloc(data, new_cap*sizeof(T));
            data = new_data;
        }
        cap = new_cap;
    }

    data[count++] = val;
}

template<typename T>
bool Array<T>::empty() {
    return count == 0;
}

template<typename T>
void Array<T>::clear() {
    count = 0;
}

template<typename T>
T& Array<T>::operator[](u64 idx) {
    return data[idx];
}

template<typename T>
const T& Array<T>::operator[](u64 idx) const {
    return data[idx];
}


// Stack
//
template<typename T>
Stack<T>::Stack() {
}

template<typename T>
Stack<T>::~Stack() {
}

template<typename T>
T Stack<T>::pop() {
    assert(top > 0);
    --top;
    return data[top];
}

template<typename T>
void Stack<T>::push(T item) {
    assert(top < arrcnt(data));
    data[top] = item;
    ++top;
}

template<typename T>
int Stack<T>::count() {
    return top;
}

template<typename T>
bool Stack<T>::empty() {
    return top == 0;
}

template<typename T>
void Stack<T>::clear() {
    top = 0;
}

// Queue
//
template<typename T>
Queue<T>::Queue() {
}

template<typename T>
Queue<T>::~Queue() {
}

template<typename T>
void Queue<T>::push(T val) {
    int next = (back + 1)%arrcnt(data);
    assert(next != front);
    data[back] = val;
    back = next;
}

template<typename T>
T Queue<T>::pop() {
    assert(front != back);
    T val = data[front];
    front = (front + 1) % arrcnt(data);
    return val;
}

template<typename T>
int Queue<T>::count() {
    if (back >= front) {
        return back - front;
    } else {
        return back + arrcnt(data) - front;
    }
}

template<typename T>
bool Queue<T>::empty() {
    return front == back;
}

template<typename T>
void Queue<T>::clear() {
    front = 0;
    back = 0;
}
