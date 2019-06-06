#pragma once

#include <string>
#include <vector>
#include "file.h"

class Logger {
public:
  struct Task;
  static Task* root;
  File* logfile;
  Logger();
  ~Logger();
  static Logger instance;
private:
  friend struct Task;
  static Task* top;
public:
  static void* begin(size_t count, char const* name = nullptr, void* task = nullptr);
  static void item(char const* name, void* task = nullptr);
  static void progress(size_t count, bool add = true, void* task = nullptr);
  static void end(bool pop = false, void* task = nullptr);

  static int menu(char const* title, std::vector<std::string> const& options);
  static int menu(char const* title, std::map<char, std::string> const& options);

  static void log(char const* fmt, ...);

  template<class List>
  class Loop {
  public:
    class Iterator {
    public:
      Iterator& operator++() {
        if (++iter_ != list_.end()) {
          Logger::item(iter_->c_str(), task_);
        }
        return *this;
      }
      bool operator!=(Iterator const& rhs) const {
        return iter_ != rhs.iter_;
      }
      typename List::value_type const& operator*() {
        return *iter_;
      }
    private:
      friend class Loop;
      Iterator(List const& list, typename List::const_iterator iter, void* task)
        : list_(list)
        , iter_(iter)
        , task_(task)
      {
        if (iter_ != list.end()) {
          Logger::item(iter_->c_str(), task_);
        }
      }
      List const& list_;
      typename List::const_iterator iter_;
      void* task_;
    };

    Loop(List const& list, char const* task = nullptr)
      : list_(std::move(list))
    {
      task_ = Logger::begin(list_.size(), task);
    }
    ~Loop() {
      Logger::end(false, task_);
    }
    Iterator begin() const {
      return Iterator(list_, list_.begin(), task_);
    }
    Iterator end() const {
      return Iterator(list_, list_.end(), task_);
    }
  private:
    List const& list_;
    void* task_;
  };

  template<class List>
  static Loop<List> loop(List const& list) {
    return Loop<List>(list);
  }
};
