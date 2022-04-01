#ifndef SIMPLE_VEC_HPP_INCLUDE_GUARD
#define SIMPLE_VEC_HPP_INCLUDE_GUARD

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

template<typename T>
struct simple_vec
{
private:

	T* m_data;

	uint32_t m_used;

	uint32_t m_capacity;

	[[nodiscard]] bool grow_by_one() noexcept
	{
		if (m_capacity < m_used + 1)
		{
			T* tmp = static_cast<T*>(realloc(m_data, m_capacity * 2 * sizeof(T)));

			if (tmp == nullptr)
				return false;

			m_data = tmp;

			m_capacity *= 2;
		}

		return true;
	}

public:

	simple_vec() noexcept : m_data{ nullptr } {}

	~simple_vec() noexcept
	{
		free(m_data);
	}

	[[nodiscard]] bool initialize(uint32_t initial_capacity) noexcept
	{
		m_data = static_cast<T*>(malloc(initial_capacity * sizeof(T)));

		if (m_data == nullptr)
			return false;

		m_used = 0;

		m_capacity = initial_capacity;

		return true;
	}

	[[nodiscard]] bool reserve(uint32_t new_capacity) noexcept
	{
		T* tmp = static_cast<T*>(realloc(m_data, new_capacity * sizeof(T)));

		if (tmp == nullptr)
			return false;

		m_data = tmp;

		m_capacity = new_capacity;

		return true;
	}

	[[nodiscard]] bool append(const T& t) noexcept
	{
		if (!grow_by_one())
			return false;

		m_data[m_used++] = t;

		return true;
	}

	[[nodiscard]] bool append(T&& t) noexcept
	{
		if (!grow_by_one())
			return false;

		m_data[m_used++] = std::move(t);

		return true;
	}

	uint32_t size() const noexcept
	{
		return m_used;
	}

	uint32_t capacity() const noexcept
	{
		return m_capacity;
	}

	const T* data() const noexcept
	{
		return m_data;
	}

	T* data() noexcept
	{
		return m_data;
	}

	const T& operator[](uint32_t i) const noexcept
	{
		return m_data[i];
	}

	T& operator[](uint32_t i) noexcept
	{
		return m_data[i];
	}

	void clear() noexcept
	{
		for (uint32_t i = 0; i != m_used; ++i)
			m_data[i].~TYPE();

		m_used = 0;
	}

	T* steal() noexcept
	{
		T* tmp = m_data;

		m_data = nullptr;

		m_used = 0;

		m_capacity = 0;

		return tmp;
	}

	void memset(uint8_t val) noexcept
	{
		::memset(m_data, 0xFF, m_capacity * sizeof(T));
	}
};

template<typename T>
struct simple_table
{
private:

	T* m_data;

	uint32_t m_size;

public:

	simple_table() noexcept : m_data{ nullptr } {}

	~simple_table() noexcept
	{
		free(m_data);
	}

	[[nodiscard]] bool initialize(uint32_t initial_size) noexcept
	{
		m_data = static_cast<T*>(malloc(initial_size * sizeof(T)));

		if (m_data == nullptr)
			return false;

		m_size = initial_size;

		return true;
	}

	[[nodiscard]] bool resize(uint32_t new_size) noexcept
	{
		T* tmp = static_cast<T*>(realloc(m_data, new_size));

		if (tmp == nullptr)
			return false;

		m_data = tmp;

		m_size = new_size;

		return true;
	}

	uint32_t size() const noexcept
	{
		return m_size;
	}

	const T* data() const noexcept
	{
		return m_data;
	}

	T* data() noexcept
	{
		return m_data;
	}

	const T& operator[](uint32_t i) const noexcept
	{
		return m_data[i];
	}

	T& operator[](uint32_t i) noexcept
	{
		return m_data[i];
	}

	T* steal() noexcept
	{
		T* tmp = m_data;

		m_data = nullptr;

		m_size = 0;

		return tmp;
	}

	void memset(uint8_t val) noexcept
	{
		::memset(m_data, 0xFF, m_size * sizeof(T));
	}
};

#endif // SIMPLE_VEC_HPP_INCLUDE_GUARD
