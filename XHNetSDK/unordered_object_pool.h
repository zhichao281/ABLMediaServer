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

#include <set>
namespace simple_pool {
	template <typename T, typename UserAllocator = std::allocator<T>>
	class unordered_object_pool
	{
	public:
		typedef T element_type;
		typedef UserAllocator user_allocator;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		unordered_object_pool(const size_type arg_next_size = 32, const size_type arg_max_size = 0)
			: m_next_size(arg_next_size), m_max_size(arg_max_size), m_used_list()
		{
			// 为内存池分配 m_max_size 个 T 对象的内存
			m_pool = static_cast<element_type*>(m_user_allocator.allocate(m_max_size));
		}

		~unordered_object_pool()
		{
			// 销毁已分配的 T 对象并释放内存
			for (auto p : m_used_list)
			{
				p->~T();
				m_user_allocator.deallocate(p, 1);
			}

			// 释放内存池的内存
			m_user_allocator.deallocate(m_pool, m_max_size);
		}

		element_type* malloc()
		{
			if (m_used_list.size() >= m_max_size)
			{
				return nullptr;
			}

			if (m_free_list.empty())
			{
				// 如果空闲链表为空，就从内存池中分配一些内存并添加到空闲链表中
				size_t new_size = (std::min)(m_next_size, m_max_size - m_used_list.size());
				element_type* p_new = static_cast<element_type*>(m_user_allocator.allocate(new_size * sizeof(T)));

				for (size_t i = 0; i < new_size; ++i)
				{
					m_free_list.push_back(p_new + i);
				}
			}

			// 从空闲链表中取出一个 T 对象
			element_type* const chunk = m_free_list.back();
			m_free_list.pop_back();

			// 将该对象添加到已用集合中
			m_used_list.insert(chunk);
			return chunk;

		}

		void free(element_type* const chunk)
		{
			if (chunk != nullptr)
			{
				// 从已用集合中删除该对象
				m_used_list.erase(chunk);
				// 将该对象添加到空闲链表中
				m_free_list.push_back(chunk);
			}
		}

		bool is_from(element_type* const chunk) const
		{
			// 判断一个对象是否来自于该内存池

			return (reinterpret_cast<char*>(chunk) >= m_pool) &&
				(reinterpret_cast<char*>(chunk) < m_pool + m_max_size);

			//   // 检查指针是否在内存池中
	/*		const void* const p = static_cast<const void*>(chunk);
			return m_used_list.find(p) != m_used_list.end();*/
		}

		

		element_type* construct()
		{
			// 分配一个 T 对象并调用其默认构造函数
			element_type* const ret = malloc();
			if (ret != nullptr)
			{
				try { new (ret) element_type(); }
				catch (...) { free(ret); throw; }
			}
			return ret;
		}
		template <typename... Args>
		element_type* construct(Args&&... args)
		{
			element_type* const chunk = malloc();
			if (chunk == nullptr) {
				return nullptr;
			}
			try {
				new (chunk) element_type(std::forward<Args>(args)...);
			}
			catch (...) {
				free(chunk);
				throw;
			}
			return chunk;
		}


		template<typename T0, typename T1>
		element_type* construct(const T0& a0, const T1& a1) const
		{
			element_type* const ret = malloc();
			if (ret != nullptr)
			{
				try { new (ret) element_type(a0, a1); }
				catch (...) { free(ret); throw; }
			}
			return ret;
		}

		template<typename T0, typename T1>  element_type* construct(T0& a0, T1& a1)
		{
			element_type* const ret = (malloc)();
			if (ret != nullptr)
			{
				try { new (ret) element_type(a0, a1); }
				catch (...) { (free)(ret); throw; }
			}
			return ret;
		}

		template<typename T0, typename T1, typename T2>
		element_type* construct(const T0& a0, const T1& a1, const T2& a2) const
		{
			element_type* const ret = (malloc)();
			if (ret != nullptr)
			{
				try { new (ret) element_type(a0, a1, a2); }
				catch (...) { (free)(ret); throw; }
			}
			return ret;
		}

		template<typename T0, typename T1, typename T2>
		element_type* construct(T0& a0, T1& a1, T2& a2)
		{
			element_type* const ret = (malloc)();
			if (ret != nullptr)
			{
				try { new (ret) element_type(a0, a1, a2); }
				catch (...) { (free)(ret); throw; }
			}
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3>
		element_type* construct(const T0& a0, const T1& a1, const T2& a2, const T3& a3) const
		{
			element_type* const ret = (malloc)();
			if (ret != nullptr)
			{
				try { new (ret) element_type(a0, a1, a2, a3); }
				catch (...) { (free)(ret); throw; }
			}
			return ret;
		}

		template<typename T0, typename T1, typename T2, typename T3>
		element_type* construct(T0& a0, T1& a1, T2& a2, T3& a3)
		{
			element_type* const ret = (malloc)();
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (ret == nullptr)
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
			if (chunk != nullptr)
			{
				chunk->~T();
				free(chunk);
			}
		}

		size_type get_next_size() const
		{
			return m_next_size;
		}

		void set_next_size(const size_type x)
		{
			m_next_size = x;
		}

	private:
		const size_type m_max_size;
		size_type m_next_size;
		user_allocator m_user_allocator;
		std::set<element_type*> m_used_list;
		std::vector<element_type*> m_free_list;
		element_type* m_pool;
	};
}




#endif
