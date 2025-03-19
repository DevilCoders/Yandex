#ifndef DPS_SINGLETON_H_INCLUDED
#define DPS_SINGLETON_H_INCLUDED

#include <stdexcept>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include <pthread.h>

namespace Yandex {

	namespace DPS {

		template <class T> struct CreateUsingNew
		{
			static T* Create()
			{ return new T; }

			static void Destroy(T* p)
			{ delete p; }
		};

		template<typename T>
			class SingletonHolder
			{
				public:
					typedef T ObjectType;
					static T& Instance();
				private:
					static void MakeInstance();
					static void DestroySingleton();
					SingletonHolder();
					static T* pInstance_;
					static bool destroyed_;
					static boost::mutex m_;
			};

		template<class T>
			T* SingletonHolder<T>::pInstance_;

		template<class T>
			bool SingletonHolder<T>::destroyed_;

		template<class T>
			boost::mutex SingletonHolder<T>::m_;

		template<class T>
			inline T& SingletonHolder<T>::Instance()
			{
				if (!pInstance_)
				{
					MakeInstance(); 
				}
				return *pInstance_;
			}

		template<class T>
			void SingletonHolder<T>::MakeInstance()
			{
				boost::mutex::scoped_lock lock(m_);
				if (!pInstance_)
				{
					if (destroyed_)
					{
						destroyed_ = false;
						throw std::logic_error("Dead Reference Detected");
					}
					pInstance_ = CreateUsingNew<T>::Create();
					std::atexit(&DestroySingleton);
				}
			}

		template<class T>
			void SingletonHolder<T>::DestroySingleton()
			{
				assert(!destroyed_);
				CreateUsingNew<T>::Destroy(pInstance_);
				pInstance_ = 0;
				destroyed_ = true;
			} 

	}// namespace DPS

}// namespace Yandex

#endif //DPS_SINGLETON_H_INCLUDED
