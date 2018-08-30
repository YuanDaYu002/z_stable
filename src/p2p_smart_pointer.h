#ifndef _P2P_SMART_POINTER_H_2015_11_24_
#define _P2P_SMART_POINTER_H_2015_11_24_
#include <tr1/memory>

#define p2p_shared_ptr std::tr1::shared_ptr
/*
	ֻ�ܹ���new�������ڴ棬��֧��mallocϵ��
	
	����ָ�벻��ָ�����顣��Ϊ�ͷ��ڴ���õ���delete����delete[]
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

