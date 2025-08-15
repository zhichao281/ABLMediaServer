#pragma once 
#ifdef USE_BOOST

#include <boost/pool/pool.hpp>

namespace simple_pool {

	template <typename T, typename UserAllocator = boost::default_user_allocator_new_delete>
	class unordered_object_pool : protected boost::pool<UserAllocator>
	{
	public:
		typedef T element_type;
		typedef UserAllocator user_allocator;
		typedef typename boost::pool<UserAllocator>::size_type size_type;
		typedef typename boost::pool<UserAllocator>::difference_type difference_type;

	protected:
		boost::pool<UserAllocator>& store()
		{
			return *this;
		}

		const boost::pool<UserAllocator>& store() const
		{
			return *this;
		}

		static void*& nextof(void* const ptr)
		{
			return *(static_cast<void**>(ptr));
		}

	public:
		explicit unordered_object_pool(const size_type arg_next_size = 32, const size_type arg_max_size = 0)
			:
			boost::pool<UserAllocator>(sizeof(T), arg_next_size, arg_max_size)
		{
		}

		~unordered_object_pool();

		element_type* malloc BOOST_PREVENT_MACRO_SUBSTITUTION()
		{
			return static_cast<element_type*>(store().malloc());
		}
		void free BOOST_PREVENT_MACRO_SUBSTITUTION(element_type* const chunk)
		{
			store().free(chunk);
		}
		bool is_from(element_type* const chunk) const
		{
			return store().is_from(chunk);
		}

		element_type* construct()
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0>  element_type* construct(const T0& a0) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0>  element_type* construct(T0& a0)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1>  element_type* construct(const T0& a0, const T1& a1) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1>  element_type* construct(T0& a0, T1& a1)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2>
		element_type* construct(const T0& a0, const T1& a1, const T2& a2) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2>
		element_type* construct(T0& a0, T1& a1, T2& a2)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3>
		element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3>
		element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4>
		element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3,
			const T4& a4) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4>
		element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3,
			T4& a4)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
		element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3,
			const T4& a4, const T5& a5) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
		element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3,
			T4& a4, T5& a5)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6>
			element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3,
				const T4& a4, const T5& a5, const T6& a6) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6>
			element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3,
				T4& a4, T5& a5, T6& a6)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6, typename T7>
			element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3,
				const T4& a4, const T5& a5, const T6& a6, const T7& a7) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6, a7); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6, typename T7>
			element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3,
				T4& a4, T5& a5, T6& a6, T7& a7)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6, a7); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6, typename T7, typename T8>
			element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3,
				const T4& a4, const T5& a5, const T6& a6, const T7& a7,
				const T8& a8) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6, typename T7, typename T8>
			element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3,
				T4& a4, T5& a5, T6& a6, T7& a7,
				T8& a8)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6, typename T7, typename T8, typename T9>
			element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3,
				const T4& a4, const T5& a5, const T6& a6, const T7& a7,
				const T8& a8, const T9& a9) const
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
			typename T6, typename T7, typename T8, typename T9>
			element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3,
				T4& a4, T5& a5, T6& a6, T7& a7,
				T8& a8, T9& a9)
		{
			element_type* const ret = (malloc)();
			if (ret == 0)
				return ret;
			try { new (ret) element_type(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }
			catch (...) { (free)(ret); throw; }
			return ret;
		}

		void destroy(element_type* const chunk)
		{
			chunk->~T();
			(free)(chunk);
		}

		size_type get_next_size() const
		{
			return store().get_next_size();
		}
		void set_next_size(const size_type x)
		{
			store().set_next_size(x);
		}
	};

	template <typename T, typename UserAllocator>
	unordered_object_pool<T, UserAllocator>::~unordered_object_pool()
	{
#ifndef BOOST_POOL_VALGRIND
		if (!this->list.valid())
			return;

		boost::details::PODptr<size_type> iter = this->list;
		boost::details::PODptr<size_type> next;

		do
		{
			next = iter.next();

			(UserAllocator::free)(iter.begin());

			iter = next;
		} while (iter.valid());

		this->list.invalidate();
#else
		for (std::set<void*>::iterator pos = this->used_list.begin(); pos != this->used_list.end(); ++pos)
		{
			static_cast<T*>(*pos)->~T();
		}

#endif
	}

}
#else
#pragma once
#include <vector>
#include <unordered_set>
#include <memory>
#include <algorithm>

namespace simple_pool {

	template <typename T, typename UserAllocator = std::allocator<T>>
	class unordered_object_pool
	{
	public:
		typedef T element_type;
		typedef UserAllocator user_allocator;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		explicit unordered_object_pool(const size_type arg_next_size = 32,
			const size_type arg_max_size = 0)
			: m_next_size(arg_next_size),
			m_max_size(arg_max_size),
			m_total_allocated(0),
			m_allocator()
		{
		}

		~unordered_object_pool()
		{
			// 销毁所有已构造的对象
			for (auto chunk : m_used_list) {
				static_cast<T*>(chunk)->~T();
			}

			// 释放所有内存块
			for (auto& block : m_blocks) {
				m_allocator.deallocate(block.first, block.second);
			}
		}

		element_type* malloc()
		{
			if (m_max_size > 0 && m_total_allocated >= m_max_size) {
				return nullptr;
			}

			if (m_free_list.empty()) {
				allocate_block();
				if (m_free_list.empty()) {
					return nullptr;
				}
			}

			void* chunk = m_free_list.back();
			m_free_list.pop_back();
			m_used_list.insert(chunk);
			m_total_allocated++;

			return static_cast<element_type*>(chunk);
		}

		void free(element_type* const chunk)
		{
			if (chunk && m_used_list.erase(chunk)) {
				m_free_list.push_back(chunk);
				m_total_allocated--;
			}
		}

		bool is_from(element_type* const chunk) const
		{
			for (const auto& block : m_blocks) {
				if (chunk >= block.first &&
					chunk < (block.first + block.second)) {
					return true;
				}
			}
			return false;
		}

		// 构造对象的各种重载版本
		element_type* construct()
		{
			element_type* const ret = malloc();
			if (ret == nullptr) return ret;
			try { new (ret) element_type(); }
			catch (...) { free(ret); throw; }
			return ret;
		}

		template<typename... Args>
		element_type* construct(Args&&... args)
		{
			element_type* const ret = malloc();
			if (ret == nullptr) return ret;
			try { new (ret) element_type(std::forward<Args>(args)...); }
			catch (...) { free(ret); throw; }
			return ret;
		}

		void destroy(element_type* const chunk)
		{
			if (chunk) {
				chunk->~T();
				free(chunk);
			}
		}

		size_type get_next_size() const { return m_next_size; }
		void set_next_size(const size_type x) { m_next_size = x; }

	private:
		void allocate_block()
		{
			size_type block_size = m_next_size;
			if (m_max_size > 0) {
				size_type remaining = m_max_size - m_total_allocated;
				if (remaining == 0) return;
				block_size = std::min(block_size, remaining);
			}

			T* new_block = m_allocator.allocate(block_size);
			if (!new_block) return;

			m_blocks.emplace_back(new_block, block_size);

			// 将新块中的对象添加到空闲列表
			for (size_type i = 0; i < block_size; ++i) {
				m_free_list.push_back(new_block + i);
			}
		}

		size_type m_next_size;          // 每次分配的块大小
		size_type m_max_size;           // 最大分配数量(0表示无限制)
		size_type m_total_allocated;    // 当前已分配总数
		user_allocator m_allocator;     // 内存分配器

		// 已分配的内存块列表(指针+大小)
		std::vector<std::pair<T*, size_type>> m_blocks;

		// 空闲对象列表
		std::vector<void*> m_free_list;

		// 正在使用的对象集合
		std::unordered_set<void*> m_used_list;
	};

} // namespace simple_pool




#endif
