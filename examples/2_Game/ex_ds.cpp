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

void enqueue(Priority_Queue *pq, Index_Dist value) {
    if (pq->size < arrcnt(pq->items)) {
        pq->items[pq->size++] = value;
        heapifyUp(pq, pq->size - 1);
    } else {
        assert(!"");
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

Index_Dist dequeue(Priority_Queue *pq)
{
    Index_Dist dummy = {0};
    if (pq->size) {
        Index_Dist item = pq->items[0];
        pq->items[0] = pq->items[--pq->size];
        heapifyDown(pq, 0);
        return item;
    }
    assert(!"X");
    return dummy;
}

Index_Dist peek(Priority_Queue *pq) {
    return pq->items[0];
}

// Array
//
template<typename T>
void Array<T>::push(T val) {
    if (count == cap) {
        u64 new_cap = 4;
        if (cap == 0) {
            data = (T *)malloc(new_cap*sizeof(T));
        } else {
            new_cap = cap<<1;
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
T stack_pop(Stack<T> *s) {
    assert(s->top > 0);
    --s->top;
    return s->data[s->top];
}

template<typename T>
void stack_push(Stack<T> *s, T item) {
    assert(s->top < arrcnt(s->data));
    s->data[s->top] = item;
    ++s->top;
}

template<typename T>
int stack_empty(Stack<T> *s) {
    return (s->top == 0);
}

template<typename T>
void stack_clear(Stack<T> *s) {
    s->top = 0;
}
