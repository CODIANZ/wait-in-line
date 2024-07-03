#if !defined(__h_WaitInLine__)
#define __h_WaitInLine__

#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <must_be_shared.h>
#include <vector>
#include <iostream>

class WaitInLine : public must_be_shared<WaitInLine>
{
public:
using function_t = std::function<void()>;
using sp = std::shared_ptr<WaitInLine>;

private:
  std::mutex              m_mtx;
  std::queue<function_t>  m_queue;
  std::condition_variable m_cond;
  std::atomic_bool        m_done;
    
public:
  WaitInLine(const magic_t& _):
  must_be_shared<WaitInLine>(_) {}
  virtual ~WaitInLine() = default;
  
  void initialize() {
    auto THIS = shared_from_this();
    std::thread([THIS](){
      std::cout << "WaitInLine: start run loop" << std::endl;
      while(!THIS->m_done){
        auto f = [THIS]() {
          std::unique_lock<std::mutex> lock(THIS->m_mtx);
          THIS->m_cond.wait(lock, [THIS]{ return !THIS->m_queue.empty() || THIS->m_done; });
          if(THIS->m_done){
            return function_t();
          }
          else{
            auto f = THIS->m_queue.front();
            THIS->m_queue.pop();
            return f;
          }
        }();
        if(f) f();
      }
      std::cout << "WaitInLine: finish run loop" << std::endl;
    }).detach();
  }
  
  void finalize() {
    m_done = true;
    m_cond.notify_one();
  }

  void registerFunction(function_t f){
    {
      std::unique_lock<std::mutex> lock(m_mtx);
      m_queue.push(f);
    }
    m_cond.notify_one();
  }
};

#endif /** !defined(__h_WaitInLine__) */
