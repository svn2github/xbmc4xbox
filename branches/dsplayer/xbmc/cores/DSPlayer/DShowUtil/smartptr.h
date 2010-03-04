// SmartPtr.h
//
// Defines a smart pointer class that does not depend on any ATL headers

#pragma once

///////////////////////////////////////////////////////////////////////
// Name: AreCOMObjectsEqual [template]
// Desc: Tests two COM pointers for equality.
///////////////////////////////////////////////////////////////////////

__declspec(nothrow) IUnknown* __stdcall AtlComPtrAssign(_Inout_opt_ _Deref_post_opt_valid_  IUnknown** pp, _In_opt_ IUnknown* lp);
//(IUnknown*) AtlComPtrAssign(_Inout_opt_ _Deref_post_opt_valid_  IUnknown** pp, _In_opt_ IUnknown* lp);

inline __declspec(nothrow) IUnknown* __stdcall AtlComPtrAssign(_Inout_opt_ _Deref_post_opt_valid_  IUnknown** pp, _In_opt_ IUnknown* lp)
{
	if (pp == NULL)
		return NULL;
		
	if (lp != NULL)
		lp->AddRef();
	if (*pp)
		(*pp)->Release();
	*pp = lp;
	return lp;
}

template <class T1, class T2>
bool AreComObjectsEqual(T1 *p1, T2 *p2)
{
    bool bResult = false;
    if (p1 == NULL && p2 == NULL)
    {
        // Both are NULL
        bResult = true;
    }
    else if (p1 == NULL || p2 == NULL)
    {
        // One is NULL and one is not
        bResult = false;
    }
    else 
    {
        // Both are not NULL. Compare IUnknowns.
        IUnknown *pUnk1 = NULL;
        IUnknown *pUnk2 = NULL;
        if (SUCCEEDED(p1->QueryInterface(IID_IUnknown, (void**)&pUnk1)))
        {
            if (SUCCEEDED(p2->QueryInterface(IID_IUnknown, (void**)&pUnk2)))
            {
                bResult = (pUnk1 == pUnk2);
                pUnk2->Release();
            }
            pUnk1->Release();
        }
    }
    return bResult;
}

// _NoAddRefOrRelease:
// This is a version of our COM interface that dis-allows AddRef
// and Release. All ref-counting should be done by the SmartPtr 
// object, so we want to dis-allow calling AddRef or Release 
// directly. The operator-> returns a _NoAddRefOrRelease pointer
// instead of returning the raw COM pointer. (This behavior is the
// same as ATL's SmartPtr class.)
template <class T>
class _NoAddRefOrRelease : public T
{
private:
    STDMETHOD_(ULONG, AddRef)() = 0;
    STDMETHOD_(ULONG, Release)() = 0;
};

template <class T>
class SmartPtr
{
public:

    // Ctor
    SmartPtr() : m_ptr(NULL)
    {
    }

    // Ctor
    SmartPtr(T *ptr)
    {
        m_ptr = ptr;
        if (m_ptr)
        {
            m_ptr->AddRef();
        }
    }

    // Copy ctor
    SmartPtr(const SmartPtr& sptr)
    {
        m_ptr = sptr.m_ptr;
        if (m_ptr)
        {
            m_ptr->AddRef();
        }
    }

    // Dtor
    ~SmartPtr() 
    { 
        if (m_ptr)
        {
            m_ptr->Release();
        }
    }
    T* operator=(_In_opt_ T* lp) throw()
	  {
        if(*this!=lp)
        {
    		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&m_ptr, lp));
        }
        return *this;
	  }
    // Assignment
    SmartPtr& operator=(const SmartPtr& sptr)
    {
        
        if (!AreComObjectsEqual(m_ptr, sptr.m_ptr))
        {
            if (m_ptr)
            {
                m_ptr->Release();
            }

            m_ptr = sptr.m_ptr;
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }
        return *this;
  }

    // address-of operator
	T** operator&()
	{
		return &m_ptr;
	}

    // dereference operator
    _NoAddRefOrRelease<T>* operator->()
    {
        return (_NoAddRefOrRelease<T>*)m_ptr;
    }

    // coerce to underlying pointer type.
    operator T*()
    {
        return m_ptr;
    }

    // Templated version of QueryInterface

    template <class Q> // Q is another interface type
    HRESULT QueryInterface(Q **ppQ)
    {
        return m_ptr->QueryInterface(__uuidof(Q), (void**)ppQ);
    }
 
    HRESULT CreateInstance(REFCLSID clsid, LPUNKNOWN pUnkOuter = NULL,DWORD dwClsContext = CLSCTX_ALL)
    {
	  ASSERT(m_ptr == NULL);
      return CoCreateInstance(clsid,pUnkOuter, dwClsContext, __uuidof(T),(void **)&m_ptr);
    }
    // safe Release() method
    ULONG Release()
    {
        T *ptr = m_ptr;
        ULONG result = 0;
        if (ptr)
        {
            m_ptr = NULL;
            result = ptr->Release();
        }
        return result;
    }

	// Attach to an existing interface (does not AddRef)
	void Attach(T* p) 
	{
		if (m_ptr)
        {
			m_ptr->Release();
        }
		m_ptr = p;
	}


        
	// Detach the interface (does not Release)
    T* Detach()
    {
        T* p = m_ptr;
        m_ptr = NULL;
        return p;
    }


    // equality operator
    bool operator==(T *ptr) const
    {
      return m_ptr == ptr;
        //return AreComObjectsEqual(m_ptr, ptr);
    }

    // inequality operator
    bool operator!=(T *ptr) const
    {
        return !operator==(ptr);
    }

//private:
    T *m_ptr;
};
