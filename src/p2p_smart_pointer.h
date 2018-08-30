#ifndef _P2P_SMART_POINTER_H_2015_11_24_
#define _P2P_SMART_POINTER_H_2015_11_24_
#include <tr1/memory>

#define p2p_shared_ptr std::tr1::shared_ptr
/*
	只能管理new出来的内存，不支持malloc系列
	
	智能指针不能指向数组。因为释放内存调用的是delete而非delete[]
*/

template<typename T>
class p2p_scoped_ptr
{
public:
	explicit p2p_scoped_ptr(T* p = 0 ):m_ptr(p){}

	~p2p_scoped_ptr(){	if(m_ptr)delete m_ptr;}

//	void reset(T* p = 0){if(m_ptr)delete m_ptr;m_ptr = p;}

	T& operator*() const {return *m_ptr;}
	
	T* operator->() const {return m_ptr;}
	
	T* get() const {return m_ptr;}
private:
	T* m_ptr;
private:
	const p2p_scoped_ptr & operator=(const p2p_scoped_ptr&);
	p2p_scoped_ptr(p2p_scoped_ptr&);	
};

#endif

