
#include<iostream>
#include<algorithm>
#include<type_traits>

using namespace std;

/*buffer size set*/
inline size_t _deque_buf_size(size_t n, size_t sz) {
	return n != 0 ? n : (sz<512 ? size_t(512 / sz) : size_t(1));
}

template<class T, class Ref, class Ptr, size_t BufSiz>
class Deque_iterator {
public:
	typedef Deque_iterator<T, T&, T*, BufSiz> iterator;
	typedef Deque_iterator<T, const T&, const T*, BufSiz> const_iterator;
	static size_t buffer_size() { return _deque_buf_size(BufSiz, sizeof(T)); }
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T** map_pointer;

	typedef Deque_iterator self;
	T* cur;
	T* first;
	T* last;
	map_pointer node;//管控中心
	Deque_iterator() :first(0), cur(0), last(0), node(0) {}
	Deque_iterator(T* x, map_pointer y) :cur(x), first(*y), last(*y + buffer_size()), node(y) {}
	Deque_iterator(const Deque_iterator& x) :
		cur(x.cur), first(x.first), last(x.last), node(x.node) {}
		
	//遇到缓冲区边缘，需要调用set_node函数跳一个缓冲区
	void set_node(map_pointer new_node) {
		node = new_node;
		first = *new_node;
		last = first + difference_type(buffer_size());
	}
	//重载各种运算子
	reference operator*()const { return *cur; }
	pointer operator->()const { return &(operator*()); }
	
	difference_type operator -(const self& x)const { 
		return difference_type(buffer_size())*(node - x.node - 1) + (cur - first) + (x.last - x.cur);
	}
	
	//前置
	self& operator++() {
		++cur;//切换至下一个缓冲区
		//如果到达所在缓冲区尾端，调用set_node函数切换至下一个节点
		if (cur == last) {
			set_node(node + 1);
			cur = first;//设置当前对象为下一个缓冲区的第一个节点
		}
		return *this;
	}
	//后置
	self operator++(int) {
		self tmp = *this;
		++*this;
		return tmp;
	}
	//前置
	self& operator--() {
		if (cur == first) {
			set_node(node - 1);
			cur = last;
		}
		--cur;
		return *this;
	}
	//后置
	self operator--(int) {
		self tmp = *this;
		--*this;
		return tmp;
	}
	//重载 +=，实现随机存取，迭代器可以直接跳跃n个距离
	self& operator+=(difference_type n) {
		difference_type offset = n + (cur - first);
		if (offset >= 0 && offset <= difference_type(buffer_size()))
		//目标位置在同一个缓冲区
			cur += n;
		else {
			//不再同一个缓冲区
			difference_type node_offset = offset>0 ? offset / difference_type(buffer_size()) :
				-difference_type((-offset - 1) / buffer_size()) - 1;
			//切换至正确的节点
			set_node(node + node_offset);
			//切换至正确的元素
			cur = first + (offset - node_offset * difference_type(buffer_size()));
		}
		return *this;
	}
	self operator+(difference_type n) {
		self tmp = *this;
		return tmp += n;
	}
	self& operator-=(difference_type n) { return *this += -n; }
	self operator-(difference_type n)const {
		self tmp = *this;
		return tmp += -n;
	}
	bool operator==(const self& x)const { return (cur == x.cur); }
	bool operator!=(const self& x)const { return (cur != x.cur); }
	bool operator<(const self& x)const {
		return node == x.node ? (cur<x.cur) : (node<x.node);
	}
	//重载[]，实现随机访问
	reference operator[](difference_type n) const
    {
        return *(*this + n);
    }
};



template<class T, size_t BufSize = 0>
class Deque {
public:
	typedef T value_type;
	typedef value_type* pointer;
	typedef value_type& reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
public:
	typedef Deque_iterator<T, T&, T*, BufSize> iterator;
protected:
	typedef pointer* map_pointer; 
	static size_type buffer_size() {
		return _deque_buf_size(BufSize, sizeof(value_type));
	}
	static size_type initial_map_size() { return 8; }
protected:
	iterator start;
	iterator finish;
	map_pointer map;
	size_type map_size; // mapÈÝÁ¿
private:
	inline void destroy(iterator first1, iterator last1)
	{
		__destroy(first1, last1, pointer(0));
	}
	inline void __destroy(iterator first, iterator last, T*)
	{
		__destroy_aux(first, last);
	}
	inline void  __destroy_aux(iterator first, iterator last)
	{
		for (; first < last; ++first)
			destroy(&*first);
	}
	inline void destroy(pointer first, pointer last) {
		for (; first<last; ++first)
			destroy(first);
	}
	inline void destroy(T* pointer)
	{
		pointer->~T();
	}
public:
	iterator begin() { return start; }
	iterator end() { return  finish; }
	reference operator[](size_type n) { return start[difference_type(n)]; }//直接调用 deque_iterator<> :: operator[]
	reference front() { return *start; }
	reference back()
	{
		iterator tmp = finish;
		--tmp;
		return *tmp;
	}
	size_type size() const { return finish - start; }
	size_type max_size() const { return size_type(-1); }
	bool empty() const { return finish == start; }
private:
	void construct(pointer p, const value_type& value) { new (p) value_type(value); }
    //当尾端只剩下一个元素备用空间，调用push_back_aux()
    //先配置一整块新的缓冲区，再设置新元素内容，然后更改迭代器finish的状态
	void push_back_aux(const value_type& t) {
		value_type t_copy = t;
		reserve_map_at_back();
		*(finish.node + 1) = allocate_node(buffer_size());
		try {
			construct(finish.cur, t_copy);
			finish.set_node(finish.node + 1);
			finish.cur = finish.first;
		}
		catch (...) {
			deallocate_node(*(finish.node + 1));
			cout << "node add failed." << endl;
			throw;
		}

	}

	void reserve_map_at_back(size_type nodes_to_add = 1) {
		if (nodes_to_add + 1 > map_size - (finish.node - map))
			reallocate_map(nodes_to_add, false);
	}

	void reserve_map_at_front(size_type nodes_to_add = 1) {
		if (nodes_to_add > start.node - map)
			reallocate_map(nodes_to_add, true);
	}
	void new_elements_at_front(size_type new_elements) {
		size_type new_nodes = (new_elements + buffer_size() - 1) / buffer_size();
		reserve_map_at_front(new_nodes);
		size_type i;
		try {
			for (i = 1; i <= new_nodes; ++i)
				*(start.node - i) = allocate_node(buffer_size());
		}
		catch (...) {
			for (size_type j = 1; j < i; ++j)
				deallocate_node(*(start.node - j));
			throw;
		}
	}
	void new_elements_at_back(size_type new_elements) {
		size_type new_nodes = (new_elements + buffer_size() - 1) / buffer_size();
		reserve_map_at_back(new_nodes);
		size_type i;
		try {
			for (i = 1; i <= new_nodes; ++i)
				*(finish.node + i) = allocate_node(buffer_size());
		}
		catch (...) {
			for (size_type j = 1; j < i; ++j)
				deallocate_node(*(finish.node + j));
			throw;
		}
	}
	iterator reserve_elements_at_front(size_type n) {
		size_type vacancies = start.cur - start.first;
		if (n > vacancies)
			new_elements_at_front(n - vacancies);
		return start - difference_type(n);
	}
	iterator reserve_elements_at_back(size_type n) {
		size_type vacancies = (finish.last - finish.cur) - 1;
		if (n > vacancies)
			new_elements_at_back(n - vacancies);
		return finish + difference_type(n);
	}

	void reallocate_map(size_type nodes_to_add, bool add_at_front) {
		size_type old_num_nodes = finish.node - start.node + 1;
		size_type new_num_nodes = old_num_nodes + nodes_to_add;

		map_pointer new_nstart;
		if (map_size > 2 * new_num_nodes) {
			new_nstart = map + (map_size - new_num_nodes) / 2 + (add_at_front ? nodes_to_add : 0);
			if (new_nstart < start.node)
				copy(start.node, finish.node + 1, new_nstart);
			else
				copy_backward(start.node, finish.node + 1, new_nstart + old_num_nodes);
		}
		else {
			size_type new_map_size = map_size + max(map_size, nodes_to_add) + 2;

			map_pointer new_map = allocate_map(new_map_size);
			new_nstart = new_map + (new_map_size - new_num_nodes) / 2 + (add_at_front ? nodes_to_add : 0);

			copy(start.node, finish.node + 1, new_nstart);

			deallocate_map(map, map_size);

			map = new_map;
			map_size = new_map_size;
		}
		start.set_node(new_nstart);
		finish.set_node(new_nstart + old_num_nodes - 1);
	}
	void push_front_aux(const value_type& t) {
		value_type t_copy = t;
		reserve_map_at_front();
		*(start.node - 1) = allocate_node(buffer_size());
		try {
			start.set_node(start.node - 1);
			start.cur = start.last - 1;
			construct(start.cur, t_copy);
		}
		catch (...) {
			start.set_node(start.node + 1);
			start.cur = start.first;
			deallocate_node(*(start.node - 1));
			throw;
		}
	}
	void  pop_back_aux() {
		deallocate_node(finish.first);
		finish.set_node(finish.node - 1);
		finish.cur = finish.last - 1;
		destroy(finish.cur);
	}
	void pop_front_aux() {
		destroy(start.cur);
		deallocate_node(start.first);
		start.set_node(start.node + 1);
		start.cur = start.first;
	}
	iterator insert_aux(iterator pos, const value_type& x) { //Ëæ»ú²åÈëÐèÒªÒÆ¶¯Êý¾Ý£¬Ð§ÂÊºÜµÍ
		difference_type index = pos - start;
		value_type x_copy = x;
		if (index < size() / 2) {
			push_front(front());
			iterator front1 = start;
			++front1;
			iterator front2 = front1;
			++front2;
			pos = start + index;
			iterator pos1 = pos;
			++pos1;
			copy(front2, pos1, front1);
		}
		else {
			push_back(back());
			iterator back1 = finish;
			--back1;
			iterator back2 = back1;
			--back2;
			pos = start + index;
			copy_backward(pos, back2, back1);
		}
		*pos = x_copy;
		return pos;
	}
	void insert_aux(iterator pos, size_type n, const value_type& x) {
		const difference_type elems_before = pos - start;
		size_type length = size();
		value_type x_copy = x;
		if (elems_before < length / 2) {
			iterator new_start = reserve_elements_at_front(n);
			iterator old_start = start;
			pos = start + elems_before;
			try {
				if (elems_before >= difference_type(n)) {
					iterator start_n = start + difference_type(n);
					uninitialized_copy(start, start_n, new_start);
					start = new_start;
					copy(start_n, pos, old_start);
					fill(pos - difference_type(n), pos, x_copy);
				}
				else {
					__uninitialized_copy_fill(start, pos, new_start, start, x_copy);
					start = new_start;
					fill(old_start, pos, x_copy);
				}

			}
			catch (...) {
				destroy_nodes_at_front(new_start);
				throw;
			}
		}
		else {
			iterator new_finish = reserve_elements_at_back(n);
			iterator old_finish = finish;
			const difference_type elems_after = difference_type(length) - elems_before;
			pos = finish - elems_after;
			try {
				if (elems_after > difference_type(n)) {
					iterator finish_n = finish - difference_type(n);
					uninitialized_copy(finish_n, finish, finish);
					finish = new_finish;
					copy_backward(pos, finish_n, old_finish);
					fill(pos, pos + difference_type(n), x_copy);
				}
				else {
					__uninitialized_fill_copy(finish, pos + difference_type(n), x_copy,
						pos, finish);
					finish = new_finish;
					fill(pos, old_finish, x_copy);
				}
			}
			catch (...) {
				destroy_nodes_at_back(new_finish);
				throw;
			}
		}
	}
	iterator __uninitialized_fill_copy(iterator result, iterator mid,
		const value_type& x,
		iterator first, iterator last) {
		uninitialized_fill(result, mid, x);
		try {
			return uninitialized_copy(first, last, mid);
		}
		catch (...) {
			destroy(result, mid);
			throw;
		}
	}
	void __uninitialized_copy_fill(iterator first1, iterator last1,
		iterator first2, iterator last2,
		const value_type& x) {
		iterator mid2 = uninitialized_copy(first1, last1, first2);
		try {
			uninitialized_fill(mid2, last2, x);
		}
		catch (...) {
			destroy(first2, mid2);
			throw;
		}
	}
	void destroy_nodes_at_front(iterator before_start) {
		for (map_pointer n = before_start.node; n < start.node; ++n)
			deallocate_node(*n);
	}
	void destroy_nodes_at_back(iterator after_finish) {
		for (map_pointer n = after_finish.node; n > finish.node; --n)
			deallocate_node(*n);
	}
	void deallocate(pointer p, size_type n) {
		::operator delete(p);
	}

	void creat_map_and_nodes(size_type num_elements) {
		size_type num_nodes = num_elements / buffer_size() + 1;
		map_size = max(initial_map_size(), num_nodes + 2);
		map = allocate_map(map_size);
		map_pointer nstart = map + (map_size - num_nodes) / 2;
		map_pointer nfinish = nstart + num_nodes - 1;
		map_pointer cur;
		try {
			for (cur = nstart; cur <= nfinish; ++cur)
				*cur = allocate_node(buffer_size());
		}
		catch (...) {
			for (map_pointer ptr = nstart; ptr < cur; ++ptr)
				deallocate_node(*ptr);
			deallocate_map(map, map_size);
			throw;
		}
		start.set_node(nstart);
		finish.set_node(nfinish);
		start.cur = start.first;
		finish.cur = finish.first + num_elements % buffer_size();
	}
	void deallocate_map(map_pointer tmp, size_type n) {
		::operator delete(tmp);
	}
	void deallocate_node(pointer tmp) {
		::operator delete(tmp);
	}
	inline pointer allocate_node(ptrdiff_t size) {
		set_new_handler(0);
		pointer tmp = (pointer)(::operator new((size_t)(size*sizeof(value_type))));
		if (tmp == 0) {
			cout << "out of memory" << endl;
			exit(1);
		}
		return tmp;
	}
	inline map_pointer allocate_map(ptrdiff_t size) {
		set_new_handler(0);
		map_pointer tmp = (map_pointer)(::operator new((size_t)(size*sizeof(pointer))));
		if (tmp == 0) {
			cout << "out of memory" << endl;
			exit(1);
		}
		return tmp;
	}
	void fill_initialize(size_type n, const value_type& value) {
		creat_map_and_nodes(n);
		map_pointer cur;
		try {
			for (cur = start.node; cur<finish.node; ++cur)
				uninitialized_fill(*cur, *cur + buffer_size(), value);
			uninitialized_fill(finish.first, finish.cur, value);
		}
		catch (...) {
			for (map_pointer p = start.node; p<cur; ++p)
				deallocate_node(*p);
			deallocate_map(map, map_size);
			throw;
		}
	}
	void destroy_map_and_nodes() {
		for (map_pointer curr = start.node; curr <= finish.node; ++curr) {
			deallocate_node(*curr);
		}
		deallocate_map(map, map_size);
	}
public:
	Deque() :start(), finish(), map(0), map_size(0) { creat_map_and_nodes(0); }
	Deque(size_type n, const value_type& value) :start(), finish(), map(0), map_size(0) {
		fill_initialize(n, value);
	}
	explicit Deque(size_type n) : start(), finish(), map(0), map_size(0)
	{
		fill_initialize(n, value_type());
	}
	Deque(Deque& x) : start(), finish(), map(0), map_size(0)
	{
		creat_map_and_nodes(x.size());
		try {
			uninitialized_copy(x.begin(), x.end(), start);
		}
		catch (...) {
			cout << "copy failed." << endl;
			destroy_map_and_nodes();
			throw;
		}

	}
	~Deque() {
		destroy(start, finish);
		destroy_map_and_nodes();
	}
	void push_back(const value_type& t) {
		if (finish.cur != finish.last - 1) {
			construct(finish.cur, t);
			++finish.cur;
		}
		else {
			push_back_aux(t);
		}
	}
	void push_front(const value_type& t) {
		if (start.cur != start.first) {
			construct(start.cur - 1, t);
			--start.cur;
		}
		else
			push_front_aux(t);
	}
	void pop_back() {
		if (finish.cur != finish.first) {
			--finish.cur;
			destroy(finish.cur);
		}
		else
			pop_back_aux();
	}
	void pop_front() {
		if (start.cur != start.last - 1) {
			destroy(start.cur);
			++start.cur;
		}
		else
			pop_front_aux();
	}
	iterator insert(iterator position, const value_type& x) {
		if (position.cur == start.cur) { 
			push_front(x);
			return start;
		}
		else if (position.cur == finish.cur) { 
			push_back(x);
			iterator tmp = finish;
			--tmp;
			return tmp;
		}
		else {
			return insert_aux(position, x);
		}
	}
	void  insert(iterator pos, size_type n, const value_type& x) {
		if (pos.cur == start.cur) {
			iterator new_start = reserve_elements_at_front(n);
			uninitialized_fill(new_start, start, x);
			start = new_start;
		}
		else if (pos.cur == finish.cur) {
			iterator new_finish = reserve_elements_at_back(n);
			uninitialized_fill(finish, new_finish, x);
			finish = new_finish;
		}
		else
			insert_aux(pos, n, x);
	}
	void insert(iterator pos, iterator first, iterator last) {
		//insert(pos, first, last, iterator_category(first));
		iterator tmp = pos;
		while (first != last) {
			insert(tmp, (*first));
			++first;
			++tmp;
		}
	}
	iterator erase(iterator pos) {
		iterator next = pos;
		++next;
		difference_type index = pos - start;
		if (index < (size() >> 1)) {
			copy_backward(start, pos, next);
			pop_front();
		}
		else {
			copy(next, finish, pos);
			pop_back();
		}
		return start + index;
	}
	iterator erase(iterator first, iterator last) {
		if (first == start && last == finish) {
			clear();
			return finish;
		}
		else {
			difference_type n = last - first;
			difference_type elems_before = first - start;
			if (elems_before < (size() - n) / 2) {
				copy_backward(start, first, last);
				iterator new_start = start + n;
				destroy(start, new_start);
				for (map_pointer cur = start.node; cur < new_start.node; ++cur)
					deallocate(*cur, buffer_size());
				start = new_start;
			}
			else {
				copy(last, finish, first);
				iterator new_finish = finish - n;
				destroy(new_finish, finish);
				for (map_pointer cur = new_finish.node + 1; cur <= finish.node; ++cur)
					deallocate(*cur, buffer_size());
				finish = new_finish;
			}
			return start + elems_before;
		}
	}
	void clear() {
		for (map_pointer node = start.node + 1; node < finish.node; ++node) {
			destroy(*node, *node + buffer_size());
			deallocate(*node, buffer_size());
		}
		if (start.node != finish.node) {
			destroy(start.cur, start.last);
			destroy(finish.first, finish.cur);
			deallocate(finish.first, buffer_size());
		}
		else
			destroy(start.cur, finish.cur);

		finish = start;
	}
};
