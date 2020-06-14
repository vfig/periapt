
#include <cstring>

template<typename _T>
cDynArray<_T>::cDynArray(unsigned int _i)
	: cDynArrayBase()
{
	if (_i != 0)
	{
		_resize(&m_data, sizeof(type), _i);
		m_size = _i;
	}
}

template<typename _T>
cDynArray<_T>::cDynArray(const cDynArray<_T>& _cpy)
	: cDynArrayBase()
{
	if (_cpy.m_size != 0)
	{
		_resize(&m_data, sizeof(type), _cpy.m_size);
		::memcpy(m_data, _cpy.m_data, sizeof(type) * _cpy.m_size);
		m_size = _cpy.m_size;
	}
}

template<typename _T>
cDynArray<_T>& cDynArray<_T>::operator=(const cDynArray<_T>& _cpy)
{
	_resize(&m_data, sizeof(type), _cpy.m_size);
	if (_cpy.m_size)
		::memcpy(m_data, _cpy.m_data, sizeof(type) * _cpy.m_size);
	m_size = _cpy.m_size;
}

template<typename _T>
_T& cDynArray<_T>::operator[](int _n) throw (/*std::out_of_range*/)
{
	if (_n >= m_size)
		throw std::out_of_range("cDynArray::operator[]");
	return reinterpret_cast<ptr>(m_data)[_n];
}

template<typename _T>
const _T& cDynArray<_T>::operator[](int _n) const throw (/*std::out_of_range*/)
{
	if (_n >= m_size)
		throw std::out_of_range("cDynArray::operator[]");
	return reinterpret_cast<ptr>(m_data)[_n];
}

